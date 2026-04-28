#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include "TrainingManager.h"
#include "ExamManager.h"
#include <string>

class PlayerCar;
class GameObject;
class Driveable;

class ModeManager
{
public:
    enum GameMode {
        BASIC_DRIVING,
        TRAINING,
        EXAM
    };

    ModeManager();

    GameMode getMode() const { return currentMode; }
    void toggleMode(int currentVehicleIdx = 0);

    // Returns a reference to the training manager to allow HUD access
    TrainingManager& getTrainingManager() { return trainingManager; }
    const TrainingManager& getTrainingManager() const { return trainingManager; }

    ExamManager& getExamManager() { return examManager; }
    const ExamManager& getExamManager() const { return examManager; }

    void update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad);
    void draw() const;

private:
    GameMode currentMode;
    TrainingManager trainingManager;
    ExamManager examManager;
};

#endif // MODEMANAGER_H
