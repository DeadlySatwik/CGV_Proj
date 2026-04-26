#ifndef RULEMANAGER_H
#define RULEMANAGER_H

#include <string>
#include <vector>

// Forward declarations
class PlayerCar;
class GameObject;
class Cross;
class Driveable;

class RuleManager
{
public:
    RuleManager();

    // Core update loop for rules
    void update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad);

    // Getters for HUD / Telemetry
    int getScore() const { return currentScore; }
    std::string getLastViolation() const { return lastViolation; }
    std::string getCurrentWarning() const { return currentWarning; }

private:
    int currentScore;
    std::string lastViolation;
    std::string currentWarning;
    
    // Violation tracking & timers
    float speedingTimer;
    float wrongWayTimer;
    
    // Stop sign specific
    float stopSignTimer;
    bool  isStopped;

    // Yield logic state machine
    enum YieldState {
        YIELD_NONE,
        YIELD_APPROACHING,
        YIELD_WAITING,
        YIELD_SAFE_TO_ENTER,
        YIELD_VIOLATION
    };
    YieldState currentYieldState;

    // Last crossed intersection tracking (to avoid multiple penalties for one crossing)
    Cross* lastCrossChecked;

    void applyPenalty(int points, const std::string& reason);
    void setWarning(const std::string& warningMsg);

    void checkSpeedLimit(PlayerCar* player, float delta);
    void checkWrongWay(PlayerCar* player, Driveable* currentRoad, float delta);
    void checkIntersectionRules(PlayerCar* player, const std::vector<GameObject*>& objects, float delta);
};

#endif // RULEMANAGER_H
