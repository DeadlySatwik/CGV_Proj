#include "ModeManager.h"
#include <iostream>

using namespace std;

ModeManager::ModeManager()
{
    currentMode = BASIC_DRIVING;
}

void ModeManager::toggleMode(int currentVehicleIdx)
{
    if (currentMode == BASIC_DRIVING)
    {
        currentMode = TRAINING;
        trainingManager.reset();
        cout << "\n=============================================" << endl;
        cout << "   [MODE SWITCH] Entering TRAINING MODE" << endl;
        cout << "   Rule enforcement and scoring ACTIVE" << endl;
        cout << "=============================================\n" << endl;
    }
    else if (currentMode == TRAINING)
    {
        currentMode = EXAM;
        trainingManager.reset();
        
        // Resolve vehicle type from index
        std::string vehType = "CAR";
        if (currentVehicleIdx == 1) vehType = "BUS";
        else if (currentVehicleIdx == 2) vehType = "BIKE";
        
        examManager.loadExam("exampleExam.txt");
        examManager.startExam(trainingManager, vehType);
    }
    else
    {
        currentMode = BASIC_DRIVING;
        examManager.cancelExam();
        cout << "\n=============================================" << endl;
        cout << "   [MODE SWITCH] Entering BASIC DRIVING MODE" << endl;
        cout << "   Rules disabled. Free roam ACTIVE" << endl;
        cout << "=============================================\n" << endl;
    }
}

void ModeManager::update(PlayerCar* player, const std::vector<GameObject*>& objects, float delta, Driveable* currentRoad)
{
    if (currentMode == TRAINING || currentMode == EXAM)
    {
        trainingManager.update(player, objects, delta, currentRoad);
    }
    
    if (currentMode == EXAM)
    {
        examManager.update(player, trainingManager, delta);
    }
}

void ModeManager::draw() const
{
    if (currentMode == TRAINING || currentMode == EXAM)
    {
        trainingManager.draw();
    }
    
    if (currentMode == EXAM)
    {
        examManager.draw();
    }
}
