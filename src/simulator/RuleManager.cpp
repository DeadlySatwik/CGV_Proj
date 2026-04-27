#include "RuleManager.h"
#include "PlayerCar.h"
#include "Road.h"
#include <iostream>
#include <cmath>

using namespace std;

RuleManager::RuleManager()
{
    currentScore = 100;
    lastViolation = "None";
    
    speedingTimer = 0.0f;
    wrongWayTimer = 0.0f;
    
    stopSignTimer = 0.0f;
    isStopped = false;
    currentYieldState = YIELD_NONE;

    lastCrossChecked = nullptr;
}

void RuleManager::setWarning(const std::string& warningMsg)
{
    currentWarning = warningMsg;
}

void RuleManager::applyPenalty(int points, const std::string& reason)
{
    currentScore -= points;
    lastViolation = reason + " (-" + std::to_string(points) + " pts)";
    // Console log the violation immediately as a HUD equivalent
    cout << "\n[RULE VIOLATION] " << lastViolation << " | Current Score: " << currentScore << "\n" << endl;
}

void RuleManager::update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad)
{
    if (!player) return;

    checkSpeedLimit(player, delta);
    
    if (currentRoad)
    {
        checkWrongWay(player, currentRoad, delta);
    }
    
    checkIntersectionRules(player, objects, delta);
}

void RuleManager::checkSpeedLimit(PlayerCar* player, float delta)
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

void RuleManager::checkWrongWay(PlayerCar* player, Driveable* currentRoad, float delta)
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

void RuleManager::checkIntersectionRules(PlayerCar* player, const std::vector<GameObject*>& objects, float delta)
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
