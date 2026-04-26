///   File: PlayerCar.h

#ifndef PLAYERCAR_H
#define PLAYERCAR_H

#include "GameObject.h"
#include "EngineCore/Colors.h"

class PlayerCar : public GameObject
{
public:
    PlayerCar(Vec3 startPos);

    // Input bit flags
    enum InputFlag
    {
        INPUT_ACCEL      = (1 << 0),
        INPUT_BRAKE      = (1 << 1),
        INPUT_STEER_LEFT = (1 << 2),
        INPUT_STEER_RIGHT= (1 << 3)
    };

    void handleInput(unsigned int inputMap, const float delta);

    // Camera helpers for 3rd person view
    Vec3 getCameraPos() const;
    Vec3 getCameraTarget() const;
    Vec3 getForward() const;

    // Road constraint — set by Simulator before handleInput
    void setOldPosition();
    void revertPosition();

    float speed;
    float heading;       // yaw in degrees (math convention: 0 = +X, increases CCW)

private:
    float acceleration;
    float maxSpeed;
    float turnSpeed;
    float friction;
    Vec3  carColor;
    Vec3  oldPos;        // for road constraint rollback
    float oldHeading;

    void update(const float delta) override;
    void draw() override;
    void drawRoof();
};

#endif // PLAYERCAR_H
