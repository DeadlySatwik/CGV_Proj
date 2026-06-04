///   File: PlayerCar.h

#ifndef PLAYERCAR_H
#define PLAYERCAR_H

#include "GameObject.h"
#include "EngineCore/Colors.h"

class PlayerCar : public GameObject
{
public:
    PlayerCar(Vec3 startPos);
    virtual ~PlayerCar() {}

    // Input bit flags
    enum InputFlag
    {
        INPUT_ACCEL      = (1 << 0),
        INPUT_BRAKE      = (1 << 1),
        INPUT_STEER_LEFT = (1 << 2),
        INPUT_STEER_RIGHT= (1 << 3),
        INPUT_HORN       = (1 << 4)
    };

    // Indicator and light state (toggled by keyPressed)
    bool indicatorLeft;
    bool indicatorRight;
    bool headlightsOn;
    float indicatorTimer; // blink cycle 0–1 s

    void handleInput(unsigned int inputMap, const float delta);

    bool isHonking;
    bool wasHonking;

    // Camera helpers for 3rd person view
    virtual Vec3 getCameraPos() const;
    virtual Vec3 getCameraTarget() const;
    // Rear backup camera (used when reversing)
    virtual Vec3 getRearCameraPos() const;
    virtual Vec3 getRearCameraTarget() const;
    Vec3 getForward() const;

    // Road constraint — set by Simulator before handleInput
    void setOldPosition();
    void revertPosition();

    float speed;
    float heading;       // yaw in degrees (math convention: 0 = +X, increases CCW)

protected:
    float acceleration;
    float maxSpeed;
    float turnSpeed;
    float friction;
    Vec3  carColor;
    Vec3  oldPos;        // for road constraint rollback
    float oldHeading;

    virtual void update(const float delta) override;
    virtual void draw() override;
    void drawRoof();
};

class PlayerBus : public PlayerCar
{
public:
    PlayerBus(Vec3 startPos);
    Vec3 getCameraPos() const override;
    Vec3 getCameraTarget() const override;
protected:
    void draw() override;
};

class PlayerBike : public PlayerCar
{
public:
    PlayerBike(Vec3 startPos);
    Vec3 getCameraPos() const override;
    Vec3 getCameraTarget() const override;
protected:
    void draw() override;
};

#endif // PLAYERCAR_H
