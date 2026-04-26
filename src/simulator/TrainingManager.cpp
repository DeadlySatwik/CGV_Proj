#include "TrainingManager.h"
#include "PlayerCar.h"
#include "Road.h"
#include <iostream>
#include <cmath>
#include <GL/gl.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

TrainingManager::TrainingManager()
{
    reset();
}

void TrainingManager::reset()
{
    activeLesson = LESSON_GENERAL_DRIVING;
    currentScore = 100;
    lastViolation = "None";
    currentWarning = "";
    currentObjective = "Free Driving (Training)";
    
    speedingTimer = 0.0f;
    wrongWayTimer = 0.0f;
    stopSignTimer = 0.0f;
    isStopped = false;
    currentYieldState = YIELD_NONE;
    lastCrossChecked = nullptr;

    parkingTarget = Vec3(4.2f, 0.0f, -2.5f);
    parkingTimer = 0.0f;
    parkingCompleted = false;

    quietZoneCenter = Vec3(0,0,0);
    quietZoneRadius = 5.0f;
}

void TrainingManager::cycleLesson()
{
    if (activeLesson == LESSON_GENERAL_DRIVING) {
        activeLesson = LESSON_QUIET_ZONE;
        setObjective("Quiet Zone Driving");
        cout << "\n[LESSON] Switched to Quiet Zone Driving" << endl;
    } else if (activeLesson == LESSON_QUIET_ZONE) {
        activeLesson = LESSON_PARKING;
        setObjective("Parking Challenge");
        cout << "\n[LESSON] Switched to Parking Challenge" << endl;
    } else {
        activeLesson = LESSON_GENERAL_DRIVING;
        setObjective("Free Driving (Training)");
        cout << "\n[LESSON] Switched to Free Driving (Training)" << endl;
    }
}

void TrainingManager::draw() const
{
    if (activeLesson == LESSON_QUIET_ZONE) {
        // Red zone
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_LINE_LOOP);
        for(int i=0; i<36; ++i) {
            float theta = i * 2.0f * M_PI / 36.0f;
            glVertex3f(quietZoneCenter.x + cosf(theta)*quietZoneRadius, 0.1f, quietZoneCenter.z + sinf(theta)*quietZoneRadius);
        }
        glEnd();
        glEnable(GL_LIGHTING);
    } else if (activeLesson == LESSON_PARKING) {
        // Green zone - solid colored quad
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.8f, 0.2f);
        glBegin(GL_QUADS);
        float sx = 0.2f;
        float sz = 0.35f;
        glVertex3f(parkingTarget.x - sx, 0.05f, parkingTarget.z - sz);
        glVertex3f(parkingTarget.x + sx, 0.05f, parkingTarget.z - sz);
        glVertex3f(parkingTarget.x + sx, 0.05f, parkingTarget.z + sz);
        glVertex3f(parkingTarget.x - sx, 0.05f, parkingTarget.z + sz);
        glEnd();
        
        // White border
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(parkingTarget.x - sx, 0.06f, parkingTarget.z - sz);
        glVertex3f(parkingTarget.x + sx, 0.06f, parkingTarget.z - sz);
        glVertex3f(parkingTarget.x + sx, 0.06f, parkingTarget.z + sz);
        glVertex3f(parkingTarget.x - sx, 0.06f, parkingTarget.z + sz);
        glEnd();
        glLineWidth(1.0f);
        glEnable(GL_LIGHTING);
    }
}

void TrainingManager::setWarning(const std::string& warningMsg)
{
    currentWarning = warningMsg;
}

void TrainingManager::setObjective(const std::string& objectiveMsg)
{
    currentObjective = objectiveMsg;
}

void TrainingManager::applyPenalty(int points, const std::string& reason)
{
    currentScore -= points;
    lastViolation = reason + " (-" + std::to_string(points) + " pts)";
    cout << "\n[RULE VIOLATION] " << lastViolation << " | Current Score: " << currentScore << "\n" << endl;
}

void TrainingManager::addScore(int points, const std::string& reason)
{
    currentScore += points;
    lastViolation = reason + " (+" + std::to_string(points) + " pts)";
    cout << "\n[SUCCESS] " << lastViolation << " | Current Score: " << currentScore << "\n" << endl;
}

void TrainingManager::update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad)
{
    if (!player) return;

    std::string oldWarning = currentWarning;

    checkSpeedLimit(player, delta);
    
    if (currentRoad)
    {
        checkWrongWay(player, currentRoad, delta);
    }
    
    checkIntersectionRules(player, objects, delta);

    if (activeLesson == LESSON_QUIET_ZONE) {
        checkQuietZone(player, delta);
    } else if (activeLesson == LESSON_PARKING) {
        checkParking(player, delta);
    }

    if (currentWarning != oldWarning && !currentWarning.empty()) {
        cout << "\n[WARNING] " << currentWarning << endl;
    }
}

void TrainingManager::checkQuietZone(PlayerCar* player, float delta)
{
    float dx = player->getPos().x - quietZoneCenter.x;
    float dz = player->getPos().z - quietZoneCenter.z;
    float dist = sqrt(dx*dx + dz*dz);

    if (dist <= quietZoneRadius) {
        if (currentWarning != "Quiet Zone. No Honking!") {
            setWarning("Quiet Zone. No Honking!");
        }
        if (player->isHonking) {
            applyPenalty(25, "Honking in Quiet Zone");
            player->isHonking = false; // Prevent spamming
        }
    } else {
        if (currentWarning == "Quiet Zone. No Honking!") {
            setWarning("");
        }
    }
}

void TrainingManager::checkParking(PlayerCar* player, float delta)
{
    if (parkingCompleted) return;

    float dx = player->getPos().x - parkingTarget.x;
    float dz = player->getPos().z - parkingTarget.z;
    float dist = sqrt(dx*dx + dz*dz);

    if (dist < 0.25f && fabs(player->speed) < 0.05f) {
        setWarning("Press 'P' to Park!");
    } else {
        if (currentWarning == "Press 'P' to Park!") setWarning("");
    }
}

void TrainingManager::tryPark(PlayerCar* player)
{
    if (activeLesson != LESSON_PARKING || parkingCompleted) return;
    
    float dx = player->getPos().x - parkingTarget.x;
    float dz = player->getPos().z - parkingTarget.z;
    float dist = sqrt(dx*dx + dz*dz);

    if (dist < 0.25f && fabs(player->speed) < 0.05f) {
        parkingCompleted = true;
        setObjective("Parking Challenge Completed!");
        addScore(50, "Perfect Parking!");
        if (currentWarning == "Press 'P' to Park!") setWarning("");
    }
}

void TrainingManager::checkSpeedLimit(PlayerCar* player, float delta)
{
    const float SPEED_LIMIT = 1.5f;
    const float SPEEDING_TIME_LIMIT = 3.0f;

    if (fabs(player->speed) > SPEED_LIMIT)
    {
        speedingTimer += delta;
        if (speedingTimer >= SPEEDING_TIME_LIMIT)
        {
            applyPenalty(5, "Speeding > 1.5 units");
            speedingTimer = 0.0f; // Reset to allow multiple penalties if sustained
        }
    }
    else
    {
        speedingTimer = 0.0f;
    }
}

void TrainingManager::checkWrongWay(PlayerCar* player, Driveable* currentRoad, float delta)
{
    const float WRONG_WAY_TIME_LIMIT = 2.0f;
    
    // Only check if player is moving
    if (fabs(player->speed) < 0.1f) 
    {
        wrongWayTimer = 0.0f;
        if (currentWarning == "Wrong-way driving detected!") {
            setWarning("");
        }
        return;
    }

    Vec3 forward = player->getForward();
    Vec3 roadDir = currentRoad->getDirection();
    
    float dot = forward.x * roadDir.x + forward.z * roadDir.z;
    
    if (dot < -0.5f)
    {
        wrongWayTimer += delta;
        if (wrongWayTimer >= WRONG_WAY_TIME_LIMIT)
        {
            applyPenalty(10, "Wrong-Way Driving");
            wrongWayTimer = 0.0f;
            setWarning("");
        }
        else
        {
            setWarning("Wrong-way driving detected!");
        }
    }
    else
    {
        wrongWayTimer = 0.0f;
        if (currentWarning == "Wrong-way driving detected!") {
            setWarning("");
        }
    }
}

void TrainingManager::checkIntersectionRules(PlayerCar* player, const std::vector<GameObject*>& objects, float delta)
{
    // Find nearest cross
    Cross* nearestCross = nullptr;
    float minDist = 1e9f;
    Vec3 pPos = player->getPos();

    for (auto obj : objects)
    {
        Cross* cross = dynamic_cast<Cross*>(obj);
        if (cross)
        {
            Vec3 cPos = cross->getPos();
            float dx = pPos.x - cPos.x;
            float dz = pPos.z - cPos.z;
            float dist = sqrt(dx*dx + dz*dz);
            if (dist < minDist)
            {
                minDist = dist;
                nearestCross = cross;
            }
        }
    }

    const float APPROACH_DIST = 4.0f;
    const float DECISION_DIST = 1.5f;
    const float CONFLICT_DIST = 0.5f;

    if (nearestCross && minDist <= APPROACH_DIST)
    {
        int playerStreetIdx = nearestCross->getClosestStreetIndex(pPos);
        if (playerStreetIdx != -1)
        {
            CrossLights* crossLights = dynamic_cast<CrossLights*>(nearestCross);

            // Determine if another vehicle has priority
            bool shouldYield = false;
            for (int i = 0; i < 4; ++i)
            {
                if (i != playerStreetIdx && nearestCross->doesYieldTo(playerStreetIdx, i))
                {
                    if (nearestCross->hasVehiclesApproaching(i))
                    {
                        shouldYield = true;
                        break;
                    }
                }
            }

            // --- Yield Zone Logic ---
            if (minDist <= APPROACH_DIST && minDist > DECISION_DIST)
            {
                currentYieldState = YIELD_APPROACHING;
                if (shouldYield && currentWarning != "Yield to higher-priority traffic")
                {
                    setWarning("Yield to higher-priority traffic");
                }
                else if (!shouldYield && currentWarning == "Yield to higher-priority traffic")
                {
                    setWarning("");
                }
            }
            else if (minDist <= DECISION_DIST && minDist > CONFLICT_DIST)
            {
                currentYieldState = YIELD_WAITING;
                if (!shouldYield)
                {
                    currentYieldState = YIELD_SAFE_TO_ENTER;
                    if (currentWarning == "Yield to higher-priority traffic") setWarning("");
                }
                else
                {
                    if (currentWarning != "Yield to higher-priority traffic") setWarning("Yield to higher-priority traffic");
                }
            }
            else if (minDist <= CONFLICT_DIST)
            {
                if (shouldYield && currentYieldState != YIELD_VIOLATION && nearestCross != lastCrossChecked)
                {
                    applyPenalty(15, "Failure to Yield");
                    setWarning("");
                    currentYieldState = YIELD_VIOLATION;
                    lastCrossChecked = nearestCross;
                }
                else if (!shouldYield)
                {
                    currentYieldState = YIELD_SAFE_TO_ENTER;
                }
            }

            // --- Traffic Light Rule ---
            if (crossLights)
            {
                if (nearestCross != lastCrossChecked)
                {
                    if (!crossLights->isGreenLight(playerStreetIdx))
                    {
                        if (minDist < CONFLICT_DIST && fabs(player->speed) > 0.05f)
                        {
                            applyPenalty(20, "Red Light Violation");
                            lastCrossChecked = nearestCross;
                        }
                    }
                    else
                    {
                        if (minDist < CONFLICT_DIST) lastCrossChecked = nearestCross;
                    }
                }
            }
            else // --- Stop Sign Rule ---
            {
                if (minDist <= DECISION_DIST)
                {
                    if (fabs(player->speed) < 0.05f)
                    {
                        stopSignTimer += delta;
                        if (stopSignTimer >= 1.5f) isStopped = true;
                    }
                }
                
                if (minDist < CONFLICT_DIST && nearestCross != lastCrossChecked)
                {
                    if (!isStopped)
                    {
                        applyPenalty(20, "Ran Stop Sign");
                    }
                    lastCrossChecked = nearestCross;
                    isStopped = false;
                    stopSignTimer = 0.0f;
                }
            }
        }
    }
    else
    {
        // Reset state when out of all zones
        if (nearestCross == nullptr || minDist > APPROACH_DIST + 1.0f)
        {
            lastCrossChecked = nullptr;
            stopSignTimer = 0.0f;
            isStopped = false;
            currentYieldState = YIELD_NONE;
            if (currentWarning == "Yield to higher-priority traffic") setWarning("");
        }
    }
}
