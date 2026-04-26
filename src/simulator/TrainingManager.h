#ifndef TRAININGMANAGER_H
#define TRAININGMANAGER_H

#include <string>
#include <vector>
#include "EngineCore/Vec3.h"

// Forward declarations
class PlayerCar;
class GameObject;
class Cross;
class Driveable;

class TrainingManager
{
public:
    enum LessonType {
        LESSON_GENERAL_DRIVING,
        LESSON_QUIET_ZONE,
        LESSON_PARKING
    };

    TrainingManager();

    // Reset state when entering training mode
    void reset();

    void setLesson(LessonType lesson) { activeLesson = lesson; }
    void cycleLesson();
    LessonType getLesson() const { return activeLesson; }

    void draw() const;
    void tryPark(PlayerCar* player);

    // Core update loop for rules
    void update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad);

    // Getters for HUD / Telemetry
    int getScore() const { return currentScore; }
    std::string getLastViolation() const { return lastViolation; }
    std::string getCurrentWarning() const { return currentWarning; }
    std::string getObjective() const { return currentObjective; }

private:
    LessonType activeLesson;
    int currentScore;
    std::string lastViolation;
    std::string currentWarning;
    std::string currentObjective;
    
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

    // Lesson specific state
    Vec3 parkingTarget;
    float parkingTimer;
    bool parkingCompleted;
    
    Vec3 quietZoneCenter;
    float quietZoneRadius;

    void applyPenalty(int points, const std::string& reason);
    void addScore(int points, const std::string& reason);
    void setWarning(const std::string& warningMsg);
    void setObjective(const std::string& objectiveMsg);

    void checkSpeedLimit(PlayerCar* player, float delta);
    void checkWrongWay(PlayerCar* player, Driveable* currentRoad, float delta);
    void checkIntersectionRules(PlayerCar* player, const std::vector<GameObject*>& objects, float delta);
    void checkQuietZone(PlayerCar* player, float delta);
    void checkParking(PlayerCar* player, float delta);
};

#endif // TRAININGMANAGER_H
