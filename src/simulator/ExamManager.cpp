#include "ExamManager.h"
#include "TrainingManager.h"
#include "PlayerCar.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <sys/stat.h>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

ExamManager::ExamManager()
{
    currentState = NOT_STARTED;
    timeLimit = 180.0f;
    passScore = 70;
    startScore = 100;
    timeRemaining = 0.0f;
    currentScore = 0;
    lastObservedScore = 0;
    currentCheckpointIndex = 0;
    totalViolations = 0;
}

void ExamManager::loadExam(const std::string& filename)
{
    checkpoints.clear();
    
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "[EXAM] Warning: Could not open " << filename << ". Using default exam config." << endl;
        // Default config
        timeLimit = 180.0f;
        passScore = 70;
        startScore = 100;
        checkpoints.push_back({Vec3(-4.0f, 0.0f, 8.0f), 1.5f, "Navigate_Intersection"});
        checkpoints.push_back({Vec3(4.0f, 0.0f, -5.0f), 1.5f, "Stop_at_Red_Light"});
        checkpoints.push_back({Vec3(8.0f, 0.0f, 8.0f), 1.5f, "Park_Safely"});
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        ss >> token;
        
        if (token == "TIME_LIMIT") {
            ss >> timeLimit;
        } else if (token == "PASS_SCORE") {
            ss >> passScore;
        } else if (token == "START_SCORE") {
            ss >> startScore;
        } else if (token == "CHECKPOINT") {
            float x, y, z;
            if (ss >> x >> y >> z) {
                std::string objective = "Reach_Checkpoint";
                ss >> objective; // Will naturally stay default if no 4th token
                checkpoints.push_back({Vec3(x, y, z), 1.5f, objective});
            }
        }
    }
    
    cout << "[EXAM] Loaded config: " << timeLimit << "s limit, " << passScore << " pass score, " << checkpoints.size() << " checkpoints." << endl;
}

void ExamManager::startExam(const TrainingManager& tm, const std::string& vehicleType)
{
    currentState = IN_PROGRESS;
    activeVehicleType = vehicleType;
    timeRemaining = timeLimit;
    currentScore = startScore;
    lastObservedScore = tm.getScore();
    currentCheckpointIndex = 0;
    failureReason = "";
    violationCounts.clear();
    totalViolations = 0;
    
    cout << "\n=============================================" << endl;
    cout << "   [EXAM STARTED] Good luck!" << endl;
    cout << "   Vehicle: " << activeVehicleType << endl;
    cout << "   Time Limit: " << timeLimit << "s | Pass Score: " << passScore << endl;
    cout << "=============================================\n" << endl;
}

void ExamManager::cancelExam()
{
    if (currentState == IN_PROGRESS) {
        failExam("Exam cancelled by user.");
    }
}

void ExamManager::recordViolation(const std::string& violationMsg)
{
    totalViolations++;
    
    // Attempt to extract the reason before the parenthesis e.g. "Red Light Violation (-20 pts)"
    size_t pos = violationMsg.find(" (");
    string type = (pos != string::npos) ? violationMsg.substr(0, pos) : violationMsg;
    
    violationCounts[type]++;
}

void ExamManager::update(PlayerCar* player, const TrainingManager& tm, float delta)
{
    if (currentState != IN_PROGRESS || !player) return;

    // Time tracking
    timeRemaining -= delta;
    if (timeRemaining <= 0) {
        timeRemaining = 0;
        failExam("Time limit exceeded.");
        return;
    }

    // Score tracking
    int tmScore = tm.getScore();
    if (tmScore < lastObservedScore) {
        int diff = lastObservedScore - tmScore;
        currentScore -= diff;
        recordViolation(tm.getLastViolation());
        cout << "[EXAM] Violation recorded: " << tm.getLastViolation() << " | Exam Score: " << currentScore << endl;
    } else if (tmScore > lastObservedScore) {
        int diff = tmScore - lastObservedScore;
        currentScore += diff; // e.g. perfect parking
        cout << "[EXAM] Bonus recorded! | Exam Score: " << currentScore << endl;
    }
    lastObservedScore = tmScore;

    if (currentScore < passScore) {
        failExam("Score dropped below passing threshold (" + to_string(passScore) + ").");
        return;
    }

    // Checkpoint tracking
    if (currentCheckpointIndex < checkpoints.size()) {
        const Checkpoint& cp = checkpoints[currentCheckpointIndex];
        float dx = player->getPos().x - cp.pos.x;
        float dz = player->getPos().z - cp.pos.z;
        float dist = sqrt(dx*dx + dz*dz);
        
        if (dist <= cp.radius) {
            currentCheckpointIndex++;
            cout << "[EXAM] Checkpoint " << currentCheckpointIndex << "/" << checkpoints.size() << " reached!" << endl;
            
            if (currentCheckpointIndex >= checkpoints.size()) {
                passExam();
            }
        }
    }
}

void ExamManager::failExam(const std::string& reason)
{
    currentState = FAILED;
    failureReason = reason;
    cout << "\n=============================================" << endl;
    cout << "   [EXAM FAILED] " << reason << endl;
    cout << "=============================================\n" << endl;
    generateReport();
}

void ExamManager::passExam()
{
    currentState = PASSED;
    cout << "\n=============================================" << endl;
    cout << "   [EXAM PASSED] Congratulations! All checkpoints reached." << endl;
    cout << "   Final Score: " << currentScore << endl;
    cout << "=============================================\n" << endl;
    generateReport();
    
    char buf[64];
    time_t now = time(0);
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", localtime(&now));
    generateLicense(string(buf));
}

void ExamManager::generateReport() const
{
#ifdef _WIN32
    mkdir("reports");
#else
    mkdir("reports", 0777);
#endif

    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", ltm);

    char dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%B %d, %Y", ltm);

    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", ltm);

    string filename = "reports/exam_" + string(buf) + ".md";
    ofstream out(filename);

    if (!out.is_open()) {
        cout << "[EXAM] Error: Could not write report to " << filename << endl;
        return;
    }

    bool passed = (currentState == PASSED);
    float timeTaken = timeLimit - timeRemaining;
    float scorePercent = (startScore > 0) ? ((float)currentScore / startScore * 100.0f) : 0.0f;
    int checkpointsDone = (int)currentCheckpointIndex;
    int totalCheckpoints = (int)checkpoints.size();

    string grade;
    if      (scorePercent >= 90) grade = "A+";
    else if (scorePercent >= 80) grade = "A";
    else if (scorePercent >= 70) grade = "B";
    else if (scorePercent >= 60) grade = "C";
    else if (scorePercent >= 50) grade = "D";
    else                          grade = "F";

    int filled = (int)(scorePercent / 5.0f);
    if (filled > 20) filled = 20;
    string scoreBar;
    for (int i = 0; i < 20; i++)
        scoreBar += (i < filled) ? "\xe2\x96\x88" : "\xe2\x96\x91";

    string rc  = passed ? "#2ecc71" : "#e74c3c";
    string rbg = passed ? "#0d2818" : "#2d0a0a";
    string resultText = passed ? "PASS" : "FAIL";

    out << "<div style=\"font-family:'Courier New',monospace;max-width:720px;margin:0 auto;"
           "background:#111;color:#ddd;border:1px solid #2c2c2c;border-radius:6px;overflow:hidden;\">\n\n";

    // Header
    out << "<div style=\"background:linear-gradient(135deg,#0f0f23,#1a1a3e);padding:24px 32px;"
           "border-bottom:3px solid " << rc << ";\">\n"
        << "<div style=\"display:flex;justify-content:space-between;align-items:flex-start;\">\n"
        << "<div>\n"
        << "<div style=\"font-size:10px;color:#888;letter-spacing:3px;margin-bottom:6px;\">AWAS TRANSPORT AUTHORITY</div>\n"
        << "<div style=\"font-size:20px;font-weight:bold;color:#fff;letter-spacing:2px;\">DRIVING EXAMINATION REPORT</div>\n"
        << "</div>\n"
        << "<div style=\"text-align:right;font-size:11px;color:#888;\">\n"
        << "<div>REF: EX-" << buf << "</div>\n"
        << "<div style=\"margin-top:4px;\">" << dateBuf << "</div>\n"
        << "<div style=\"margin-top:4px;\">" << timeBuf << "</div>\n"
        << "</div>\n</div>\n</div>\n\n";

    // Result banner
    out << "<div style=\"background:" << rbg << ";border-bottom:1px solid " << rc << ";"
           "padding:16px 32px;display:flex;align-items:center;gap:24px;\">\n"
        << "<div style=\"font-size:36px;font-weight:bold;color:" << rc << ";letter-spacing:4px;\">"
        << resultText << "</div>\n"
        << "<div>\n"
        << "<div style=\"font-size:28px;font-weight:bold;color:" << rc << ";\">" << grade << "</div>\n"
        << "<div style=\"font-size:10px;color:#888;letter-spacing:1px;\">GRADE</div>\n"
        << "</div>\n"
        << "<div style=\"margin-left:auto;text-align:right;\">\n"
        << "<div style=\"font-size:28px;font-weight:bold;color:" << rc << ";\">"
        << currentScore << "<span style=\"font-size:14px;color:#888;\">/" << startScore << "</span></div>\n"
        << "<div style=\"font-size:10px;color:#888;letter-spacing:1px;\">EXAM SCORE</div>\n"
        << "</div>\n</div>\n\n";

    // Score bar
    out << "<div style=\"padding:16px 32px;border-bottom:1px solid #222;\">\n"
        << "<div style=\"display:flex;justify-content:space-between;margin-bottom:6px;\">\n"
        << "<span style=\"font-size:10px;color:#888;letter-spacing:2px;\">SCORE BREAKDOWN</span>\n"
        << "<span style=\"font-size:10px;color:#888;\">PASSING THRESHOLD: " << passScore << " / " << startScore << "</span>\n"
        << "</div>\n"
        << "<div style=\"font-family:monospace;font-size:14px;color:" << rc << ";letter-spacing:1px;\">"
        << "[" << scoreBar << "] " << (int)scorePercent << "%</div>\n"
        << "</div>\n\n";

    // Candidate + stats (two columns)
    out << "<div style=\"display:grid;grid-template-columns:1fr 1fr;border-bottom:1px solid #222;\">\n"
        << "<div style=\"padding:20px 32px;border-right:1px solid #222;\">\n"
        << "<div style=\"font-size:10px;color:#666;letter-spacing:2px;margin-bottom:10px;\">CANDIDATE DETAILS</div>\n"
        << "<table style=\"width:100%;font-size:13px;border-collapse:collapse;\">\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Candidate</td><td style=\"color:#ddd;text-align:right;\">Player 1</td></tr>\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Vehicle Class</td><td style=\"color:#ddd;text-align:right;\">" << activeVehicleType << "</td></tr>\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Examiner</td><td style=\"color:#ddd;text-align:right;\">AWAS System</td></tr>\n"
        << "</table>\n</div>\n"
        << "<div style=\"padding:20px 32px;\">\n"
        << "<div style=\"font-size:10px;color:#666;letter-spacing:2px;margin-bottom:10px;\">EXAM STATISTICS</div>\n"
        << "<table style=\"width:100%;font-size:13px;border-collapse:collapse;\">\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Time Used</td>"
        << "<td style=\"color:#ddd;text-align:right;\">" << (int)timeTaken << "s / " << (int)timeLimit << "s</td></tr>\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Checkpoints</td>"
        << "<td style=\"color:#ddd;text-align:right;\">" << checkpointsDone << " / " << totalCheckpoints << "</td></tr>\n"
        << "<tr><td style=\"color:#888;padding:3px 0;\">Total Violations</td>"
        << "<td style=\"color:" << (totalViolations == 0 ? "#2ecc71" : "#e74c3c") << ";text-align:right;font-weight:bold;\">"
        << totalViolations << "</td></tr>\n"
        << "</table>\n</div>\n</div>\n\n";

    // Failure reason
    if (!passed && !failureReason.empty()) {
        out << "<div style=\"padding:12px 32px;background:#1a0a0a;border-bottom:1px solid #3d1515;"
               "border-left:4px solid #e74c3c;\">\n"
            << "<div style=\"font-size:10px;color:#e74c3c;letter-spacing:2px;margin-bottom:4px;\">FAILURE REASON</div>\n"
            << "<div style=\"font-size:13px;color:#ff8888;\">" << failureReason << "</div>\n"
            << "</div>\n\n";
    }

    // Checkpoints
    out << "<div style=\"padding:20px 32px;border-bottom:1px solid #222;\">\n"
        << "<div style=\"font-size:10px;color:#666;letter-spacing:2px;margin-bottom:10px;\">CHECKPOINT PROGRESS</div>\n"
        << "<table style=\"width:100%;font-size:13px;border-collapse:collapse;\">\n"
        << "<tr style=\"border-bottom:1px solid #222;\">\n"
        << "<th style=\"text-align:left;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;width:30px;\">#</th>\n"
        << "<th style=\"text-align:left;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;\">OBJECTIVE</th>\n"
        << "<th style=\"text-align:right;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;\">STATUS</th>\n"
        << "</tr>\n";
    for (size_t i = 0; i < checkpoints.size(); i++) {
        bool done = (i < currentCheckpointIndex);
        out << "<tr style=\"border-bottom:1px solid #1a1a1a;\">\n"
            << "<td style=\"padding:8px 0;color:#555;\">" << (i+1) << "</td>\n"
            << "<td style=\"padding:8px 0;color:#ccc;\">" << checkpoints[i].objective << "</td>\n"
            << "<td style=\"padding:8px 0;text-align:right;color:" << (done ? "#2ecc71" : "#555") << ";font-weight:bold;\">"
            << (done ? "&#10003; COMPLETE" : "INCOMPLETE") << "</td>\n"
            << "</tr>\n";
    }
    out << "</table>\n</div>\n\n";

    // Violations
    out << "<div style=\"padding:20px 32px;border-bottom:1px solid #222;\">\n"
        << "<div style=\"font-size:10px;color:#666;letter-spacing:2px;margin-bottom:10px;\">"
        << "VIOLATIONS RECORD &nbsp;<span style=\"color:" << (totalViolations == 0 ? "#2ecc71" : "#e74c3c") << ";\">"
        << totalViolations << " TOTAL</span></div>\n";
    if (totalViolations == 0) {
        out << "<div style=\"color:#2ecc71;font-size:13px;\">&#10003; No violations recorded &#8212; perfect driving conduct</div>\n";
    } else {
        out << "<table style=\"width:100%;font-size:13px;border-collapse:collapse;\">\n"
            << "<tr style=\"border-bottom:1px solid #222;\">\n"
            << "<th style=\"text-align:left;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;\">VIOLATION TYPE</th>\n"
            << "<th style=\"text-align:center;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;\">COUNT</th>\n"
            << "<th style=\"text-align:right;color:#555;font-weight:normal;padding:5px 0;font-size:10px;letter-spacing:1px;\">SEVERITY</th>\n"
            << "</tr>\n";
        for (auto const& p : violationCounts) {
            string sev = "MINOR";
            string sevColor = "#f39c12";
            if (p.first.find("Wrong-Way") != string::npos || p.first.find("Red Light") != string::npos) {
                sev = "MAJOR"; sevColor = "#e74c3c";
            } else if (p.first.find("Yield") != string::npos || p.first.find("Stop") != string::npos) {
                sev = "MODERATE"; sevColor = "#e67e22";
            }
            out << "<tr style=\"border-bottom:1px solid #1a1a1a;\">\n"
                << "<td style=\"padding:8px 0;color:#ccc;\">" << p.first << "</td>\n"
                << "<td style=\"padding:8px 0;text-align:center;color:#e74c3c;font-weight:bold;\">" << p.second << "</td>\n"
                << "<td style=\"padding:8px 0;text-align:right;font-size:11px;letter-spacing:1px;color:" << sevColor << ";\">" << sev << "</td>\n"
                << "</tr>\n";
        }
        out << "</table>\n";
    }
    out << "</div>\n\n";

    // Remarks
    out << "<div style=\"padding:20px 32px;border-bottom:1px solid #222;\">\n"
        << "<div style=\"font-size:10px;color:#666;letter-spacing:2px;margin-bottom:8px;\">OFFICIAL REMARKS</div>\n"
        << "<div style=\"font-size:13px;color:#aaa;line-height:1.7;\">";
    if (passed) {
        out << "The candidate has demonstrated sufficient skill in operating the " << activeVehicleType
            << " and adhering to all applicable traffic regulations. "
            << "This result is certified by the AWAS Transport Authority.";
    } else {
        out << "The candidate did not meet the required standard for this examination. "
            << "A review of the recorded violations is strongly advised. "
            << "The candidate may re-attempt after completing additional practice in AWAS Training Mode.";
    }
    out << "</div>\n</div>\n\n";

    // Footer
    out << "<div style=\"padding:14px 32px;background:#0a0a0a;display:flex;"
           "justify-content:space-between;align-items:center;border-top:1px solid #1a1a1a;\">\n"
        << "<div style=\"font-size:9px;color:#444;letter-spacing:1px;\">AWAS &#8212; AUTOMATED WORLD ASSESSMENT SYSTEM</div>\n"
        << "<div style=\"font-size:9px;color:#444;\">DOC REF: EX-" << buf << "</div>\n"
        << "</div>\n\n";

    out << "</div>\n";

    out.close();
    cout << "[EXAM] Report generated at: " << filename << endl;
}

void ExamManager::generateLicense(const std::string& timestamp) const
{
    string filename = "reports/driving_license_" + timestamp + ".md";
    ofstream out(filename);

    if (!out.is_open()) {
        cout << "[EXAM] Error: Could not write license to " << filename << endl;
        return;
    }

    time_t now = time(0);
    tm* ltm = localtime(&now);

    char dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%d %b %Y", ltm);

    tm expiryTm = *ltm;
    expiryTm.tm_year += 4;
    mktime(&expiryTm);
    char expiryBuf[64];
    strftime(expiryBuf, sizeof(expiryBuf), "%d %b %Y", &expiryTm);

    string licNum = "AWAS-" + timestamp.substr(0, 8) + "-" + timestamp.substr(9, 6);

    string classDesc = "Standard Motor Vehicle";
    if      (activeVehicleType == "TRUCK") classDesc = "Heavy Goods Vehicle";
    else if (activeVehicleType == "BUS")   classDesc = "Passenger Service Vehicle";
    else if (activeVehicleType == "BIKE")  classDesc = "Motorcycle / Light Vehicle";

    // Outer card
    out << "<div style=\"font-family:'Courier New',monospace;max-width:620px;margin:0 auto;"
           "background:linear-gradient(145deg,#0a1628,#0d1f3c);color:#cce0ff;"
           "border:1px solid #1e3a5f;border-radius:12px;overflow:hidden;"
           "box-shadow:0 8px 32px rgba(0,0,0,0.6);\">\n\n";

    // Top security strip
    out << "<div style=\"background:repeating-linear-gradient(90deg,"
           "#1a4a7a 0px,#1a4a7a 6px,#0e2d4d 6px,#0e2d4d 12px);height:6px;\"></div>\n\n";

    // Header row
    out << "<div style=\"padding:16px 24px 12px;border-bottom:1px solid #1e3a5f;"
           "display:flex;justify-content:space-between;align-items:center;\">\n"
        << "<div>\n"
        << "<div style=\"font-size:8px;color:#4a90d9;letter-spacing:4px;\">STATE OF AWAS</div>\n"
        << "<div style=\"font-size:15px;font-weight:bold;color:#fff;letter-spacing:3px;margin-top:3px;\">DEPT. OF MOTOR VEHICLES</div>\n"
        << "</div>\n"
        << "<div style=\"text-align:right;\">\n"
        << "<div style=\"font-size:8px;color:#4a90d9;letter-spacing:2px;\">OFFICIAL DOCUMENT</div>\n"
        << "<div style=\"font-size:11px;color:#2ecc71;font-weight:bold;margin-top:4px;letter-spacing:2px;\">&#10003; VERIFIED</div>\n"
        << "</div>\n</div>\n\n";

    // Body: photo col + info col
    out << "<div style=\"display:flex;\">\n"

        // Photo column
        << "<div style=\"padding:20px 16px 20px 24px;\">\n"
        << "<div style=\"width:88px;height:108px;background:#071224;border:1px solid #2a5080;"
           "border-radius:4px;display:flex;align-items:center;justify-content:center;flex-direction:column;\">\n"
        << "<div style=\"font-size:32px;\">&#128100;</div>\n"
        << "<div style=\"font-size:7px;color:#2a5080;margin-top:5px;letter-spacing:2px;\">PHOTO</div>\n"
        << "</div>\n"
        << "<div style=\"margin-top:8px;width:88px;background:#2ecc71;border-radius:3px;"
           "text-align:center;padding:5px 0;\">\n"
        << "<div style=\"font-size:8px;color:#000;font-weight:bold;letter-spacing:2px;\">CLASS</div>\n"
        << "<div style=\"font-size:16px;color:#000;font-weight:bold;\">" << activeVehicleType << "</div>\n"
        << "</div>\n</div>\n"

        // Info column
        << "<div style=\"padding:20px 24px 20px 8px;flex:1;\">\n"
        << "<div style=\"font-size:15px;font-weight:bold;color:#fff;letter-spacing:2px;"
           "border-bottom:1px solid #1e3a5f;padding-bottom:8px;margin-bottom:14px;\">"
           "OFFICIAL DRIVING LICENCE</div>\n"
        << "<table style=\"width:100%;font-size:12px;border-collapse:collapse;\">\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;width:42%;\">FULL NAME</td>"
           "<td style=\"color:#fff;font-weight:bold;\">PLAYER ONE</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">LICENCE NO.</td>"
           "<td style=\"color:#2ecc71;font-weight:bold;font-size:11px;letter-spacing:1px;\">" << licNum << "</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">DATE OF ISSUE</td>"
           "<td style=\"color:#ddd;\">" << dateBuf << "</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">VALID UNTIL</td>"
           "<td style=\"color:#f39c12;font-weight:bold;\">" << expiryBuf << "</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">VEHICLE CLASS</td>"
           "<td style=\"color:#ddd;\">" << classDesc << "</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">RESTRICTIONS</td>"
           "<td style=\"color:#888;\">NONE</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">ENDORSEMENTS</td>"
           "<td style=\"color:#888;\">NONE</td></tr>\n"
        << "<tr><td style=\"color:#4a90d9;padding:4px 0;font-size:9px;letter-spacing:1px;\">STATUS</td>"
           "<td style=\"color:#2ecc71;font-weight:bold;letter-spacing:2px;\">&#9679; VALID</td></tr>\n"
        << "</table>\n</div>\n</div>\n\n";

    // Barcode strip
    out << "<div style=\"padding:8px 24px 12px;border-top:1px solid #1e3a5f;\">\n"
        << "<div style=\"background:#fff;border-radius:3px;padding:6px 8px;text-align:center;"
           "font-size:14px;letter-spacing:5px;color:#000;font-family:monospace;\">"
           "|| ||| || | ||| | || ||| | | || ||| || | ||| || | |</div>\n"
        << "<div style=\"font-size:8px;color:#2a4a6a;text-align:center;margin-top:3px;"
           "letter-spacing:3px;\">" << licNum << "</div>\n"
        << "</div>\n\n";

    // Certification text
    out << "<div style=\"padding:10px 24px;border-top:1px solid #1e3a5f;"
           "border-bottom:1px solid #1e3a5f;background:rgba(0,0,0,0.2);\">\n"
        << "<div style=\"font-size:10px;color:#6a8aaa;font-style:italic;line-height:1.6;\">"
           "This document certifies that the named candidate has successfully passed all required "
           "examinations to operate a " << activeVehicleType << " within the State of AWAS. "
           "This licence is valid until the date shown above.</div>\n"
        << "</div>\n\n";

    // Footer
    out << "<div style=\"padding:10px 24px;display:flex;justify-content:space-between;align-items:center;\">\n"
        << "<div style=\"font-size:8px;color:#1e3a5f;letter-spacing:1px;\">AWAS AUTOMATED WORLD ASSESSMENT SYSTEM</div>\n"
        << "<div style=\"font-size:8px;color:#1e3a5f;\">EXAM REF: " << timestamp << "</div>\n"
        << "</div>\n\n";

    // Bottom security strip
    out << "<div style=\"background:repeating-linear-gradient(90deg,"
           "#1a4a7a 0px,#1a4a7a 6px,#0e2d4d 6px,#0e2d4d 12px);height:6px;\"></div>\n\n";

    out << "</div>\n";

    out.close();
    cout << "[EXAM] License generated at: " << filename << endl;
}

void ExamManager::draw() const
{
    if (currentState != IN_PROGRESS || currentCheckpointIndex >= checkpoints.size()) return;

    // Draw active checkpoint as a golden circle
    const Checkpoint& cp = checkpoints[currentCheckpointIndex];
    
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 0.84f, 0.0f); // Gold
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    for(int i=0; i<36; ++i) {
        float theta = i * 2.0f * M_PI / 36.0f;
        glVertex3f(cp.pos.x + cosf(theta)*cp.radius, 0.2f, cp.pos.z + sinf(theta)*cp.radius);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}
