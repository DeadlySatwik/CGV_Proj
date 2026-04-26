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

class GameObject;

class Simulator : private EngineCore, private Graphics, public ObjectsLoader
{
    friend GameObject;

public:
    static Simulator &getInstance();

    Vec3 cameraPos;
    Vec3 cameraRot;

    void run();

protected:
    GameObject* findObjectByName(const std::string objectName) const;
    void loadedNewObject(GameObject *newGameObject);
    void loadedNewFactory(Garage *newFactory);

private:
    Simulator();

    void registerObject(GameObject *go);
    void destroyObject(GameObject *go);

    void cleanSimulation();

    std::vector<GameObject*> objects;
    std::vector<Garage*> spots;

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

    const float CAMERA_VELOCITY;
};

#endif // SIMULTOR_H
