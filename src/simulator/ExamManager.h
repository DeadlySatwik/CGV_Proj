#ifndef EXAMMANAGER_H
#define EXAMMANAGER_H

#include <string>
#include <vector>
#include <map>
#include "EngineCore/Vec3.h"

class PlayerCar;
class TrainingManager;

class ExamManager
{
public:
    enum ExamState {
        NOT_STARTED,
        IN_PROGRESS,
        PASSED,
        FAILED
    };

    struct Checkpoint {
        Vec3 pos;
        float radius;
    };

    struct ViolationRecord {
        std::string type;
        int count;
    };

    ExamManager();

    void loadExam(const std::string& filename);
    void startExam(const TrainingManager& tm);
    void update(PlayerCar* player, const TrainingManager& tm, float delta);
    void draw() const;

    ExamState getState() const { return currentState; }
    float getTimeRemaining() const { return timeRemaining; }
    int getExamScore() const { return currentScore; }
    int getPassScore() const { return passScore; }
    size_t getCurrentCheckpointIndex() const { return currentCheckpointIndex; }
    size_t getTotalCheckpoints() const { return checkpoints.size(); }
    const std::vector<Checkpoint>& getCheckpoints() const { return checkpoints; }
    void cancelExam();

private:
    ExamState currentState;
    
    // Config
    float timeLimit;
    int passScore;
    int startScore;
    std::vector<Checkpoint> checkpoints;

    // State
    float timeRemaining;
    int currentScore;
    int lastObservedScore;
    size_t currentCheckpointIndex;
    std::string failureReason;
    
    // Stats
    std::map<std::string, int> violationCounts;
    int totalViolations;

    void recordViolation(const std::string& violationMsg);
    void failExam(const std::string& reason);
    void passExam();
    void generateReport() const;
};

#endif // EXAMMANAGER_H
