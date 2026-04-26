///   File: Simulator.h

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "EngineCore/EngineCore.h"
#include "EngineCore/Graphics.h"
#include "ObjectsLoader.h"
#include "PlayerCar.h"

class GameObject;

class Simulator : private EngineCore, private Graphics, public ObjectsLoader
{
    friend GameObject;

public:
    static Simulator &getInstance();

    Vec3 cameraPos;
    Vec3 cameraRot;

    void run();

    // Player car/bus/bike
    std::vector<PlayerCar*> playerVehicles;
    PlayerCar *activePlayerVeh;
    int currentVehicleIdx;
    
    bool thirdPersonMode;
    unsigned int playerInputMap;

    // Vehicle cap
    int globalMaxVehicles;
    int getActiveVehicleCount() const;

    // Day/Night cycle — public so Environment/Vehicle/Garage can query
    int getDayPhase() const;
    float getWorldTime() const;

protected:
    GameObject *findObjectByName(const std::string objectName) const;
    void loadedNewObject(GameObject *newGameObject);
    void loadedNewFactory(Garage *newFactory);

private:
    Simulator();

    void registerObject(GameObject *go);
    void destroyObject(GameObject *go);

    void cleanSimulation();

    std::vector<GameObject *> objects;
    std::vector<Garage *> spots;

    void keyHeld(char k);
    void keyPressed(char k);
    void update(const float delta);
    void singleUpdate(const float delta);
    void redraw();
    void mouseMove(const int dx, const int dy);

    enum DirectionMove
    {
        FORWARD,
        BACK,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    int maxNumberOfObjects;

    unsigned int cameraDirection;
    float cameraVelocity;

    // Derived camera vectors (recomputed from yaw/pitch)
    Vec3 cameraFront;
    Vec3 cameraRight;
    Vec3 cameraUp;

    float moveSpeed;
    float mouseSensitivity;

    // Projection mode
    bool projectionOrtho;
    float orthoZoom;

    void cameraMove(const float delta);
    void updateCameraVectors();
    void setupProjection();
    bool isOnRoad(Vec3 testPos) const; // road constraint for player car
    bool sampleRoadHeight(const Vec3 &testPos, float &outHeight) const;
    bool isPlayerBlocked(const Vec3 &testPos) const;
    void printTelemetry() const;

    const float CAMERA_VELOCITY;

    // ===== Day/Night Cycle =====
    enum DayPhase
    {
        PHASE_NIGHT = 0,
        PHASE_DAWN,
        PHASE_MORNING,
        PHASE_NOON,
        PHASE_AFTERNOON,
        PHASE_EVENING,
        PHASE_DUSK,
        PHASE_COUNT
    };

    float worldTime;  // 0.0-24.0 simulated hours
    float timeSpeed;  // sim-minutes per real-second
    bool timeFlowing; // M key toggle
    int dayPhase;

    void updateWorldTime(float delta);
    int computeDayPhase() const;
    float getDayPhaseProgress() const; // 0.0-1.0 within current phase

    // Dynamic lighting helpers
    void updateSkyAndLighting();

    static const char *phaseNames[PHASE_COUNT];
    static float phaseBoundaries[PHASE_COUNT + 1];
};

#endif // SIMULTOR_H
