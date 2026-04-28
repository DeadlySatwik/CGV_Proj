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
    // Create reports dir
#ifdef _WIN32
    mkdir("reports");
#else
    mkdir("reports", 0777);
#endif

    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", ltm);
    
    string filename = "reports/exam_" + string(buf) + ".md";
    ofstream out(filename);
    
    if (!out.is_open()) {
        cout << "[EXAM] Error: Could not write report to " << filename << endl;
        return;
    }

    out << "# Driving Exam Report\n\n";
    
    out << "## Candidate Information\n";
    out << "- **Vehicle Type:** " << activeVehicleType << "\n";
    out << "- **Date:** " << buf << "\n\n";
    
    out << "## Result\n";
    out << "**" << (currentState == PASSED ? "PASS" : "FAIL") << "**\n\n";
    
    out << "## Score & Duration\n";
    out << "- **Score:** " << currentScore << " / " << startScore << " (Passing: " << passScore << ")\n";
    float timeTaken = timeLimit - timeRemaining;
    out << "- **Time:** " << (int)timeTaken << "s / " << (int)timeLimit << "s\n\n";
    
    if (currentState == FAILED) {
        out << "## Failure Reason\n";
        out << "> " << failureReason << "\n\n";
    }

    out << "## Checkpoints & Objectives\n";
    out << "| Progress | Objective |\n";
    out << "| :--- | :--- |\n";
    for (size_t i = 0; i < checkpoints.size(); i++) {
        if (i < currentCheckpointIndex) {
            out << "| [x] | " << checkpoints[i].objective << " |\n";
        } else {
            out << "| [ ] | " << checkpoints[i].objective << " |\n";
        }
    }
    out << "\n";

    out << "## Violations Summary (" << totalViolations << " total)\n";
    if (totalViolations == 0) {
        out << "No violations recorded. Perfect driving!\n\n";
    } else {
        out << "| Violation Type | Count |\n";
        out << "| :--- | :--- |\n";
        for (auto const& pair : violationCounts) {
            out << "| " << pair.first << " | " << pair.second << " |\n";
        }
        out << "\n";
    }
    
    out << "## Remarks\n";
    if (currentState == PASSED) {
        out << "The candidate has demonstrated sufficient skill in operating the " << activeVehicleType 
            << " and adhering to traffic rules.\n\n";
    } else {
        out << "The candidate failed to meet the required standard. Please review the violations above and practice further in Training Mode.\n\n";
    }

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
    
    out << "# OFFICIAL DRIVING LICENSE\n\n";
    
    out << "<div style=\"border: 2px solid #2ecc71; padding: 20px; border-radius: 10px; max-width: 400px; font-family: sans-serif;\">\n\n";
    
    out << "### STATE OF AWAS - DEPT OF MOTOR VEHICLES\n";
    out << "--- \n\n";
    out << "**LICENSE CLASS:** " << activeVehicleType << "\n\n";
    out << "**CANDIDATE:** Player 1\n\n";
    
    time_t now = time(0);
    char dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%B %d, %Y", localtime(&now));
    out << "**ISSUE DATE:** " << dateBuf << "\n\n";
    
    out << "**STATUS:** VALID\n\n";
    out << "**EXAM REF:** " << timestamp << "\n\n";
    
    out << "--- \n";
    out << "*This document certifies that the candidate has successfully passed the required examinations to operate a " << activeVehicleType << ".* \n\n";
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
