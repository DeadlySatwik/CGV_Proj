///   File: PlayerCar.cpp

#include "PlayerCar.h"
#include "Simulator.h"
#include <cmath>
#include <iostream>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PlayerCar::PlayerCar(Vec3 startPos)
{
    pos = startPos;
    rot = Vec3(0, 0, 0);

    speed = 0.0f;
    heading = 0.0f; // facing +X initially
    acceleration = 2.8f;
    maxSpeed = 3.4f;
    turnSpeed = 115.0f; // degrees per second at low speed
    friction = 1.45f;

    oldPos = pos;
    oldHeading = heading;
    isHonking = false;
    wasHonking = false;

    // Distinct bright blue color so player car stands out
    carColor = Vec3(0.15f, 0.45f, 0.90f);

    id = "PLAYER_CAR";
}

Vec3 PlayerCar::getForward() const
{
    // Math convention: heading in degrees, 0 = +X, increases CCW
    float rad = heading * M_PI / 180.0f;
    return Vec3(cosf(rad), 0.0f, sinf(rad));
}

void PlayerCar::setOldPosition()
{
    oldPos = pos;
    oldHeading = heading;
}

void PlayerCar::revertPosition()
{
    pos = oldPos;
    heading = oldHeading;
    speed = 0.0f;
    // Keep rot in sync
    rot.y = -heading; // angleXZ convention: rot.y = -angle
}

void PlayerCar::handleInput(unsigned int inputMap, const float delta)
{
    wasHonking = isHonking;
    isHonking = (inputMap & INPUT_HORN) != 0;

    if (isHonking && !wasHonking) {
        cout << "\a\n*** HONK! ***" << endl;
    }

    // Steering response scales with speed for stability.
    float speedAbs = fabs(speed);
    if (speedAbs > 0.02f)
    {
        float steerFactor = 1.0f;
        if (speedAbs > 1.2f)
            steerFactor = 1.2f / speedAbs;
        if (steerFactor < 0.35f)
            steerFactor = 0.35f;

        float signedSteer = 0.0f;
        if (inputMap & INPUT_STEER_LEFT)
            signedSteer += 1.0f;
        if (inputMap & INPUT_STEER_RIGHT)
            signedSteer -= 1.0f;

        heading += signedSteer * turnSpeed * steerFactor * delta;
    }

    // Acceleration / braking with traction-aware shaping.
    float accelGain = acceleration;
    if (speed > maxSpeed * 0.7f)
        accelGain *= 0.65f;

    if (inputMap & INPUT_ACCEL)
    {
        speed += accelGain * delta;
        if (speed > maxSpeed)
            speed = maxSpeed;
    }
    else if (inputMap & INPUT_BRAKE)
    {
        float brakeGain = acceleration * 2.0f;
        speed -= brakeGain * delta;
        if (speed < -maxSpeed * 0.22f)
            speed = -maxSpeed * 0.22f;
    }
    else
    {
        // Rolling resistance / drag toward zero.
        if (speed > 0)
        {
            speed -= friction * delta;
            if (speed < 0)
                speed = 0;
        }
        else if (speed < 0)
        {
            speed += friction * 1.2f * delta;
            if (speed > 0)
                speed = 0;
        }
    }

    // Clamp reverse speed and tiny oscillations.
    if (speed < -maxSpeed * 0.22f)
        speed = -maxSpeed * 0.22f;
    if (fabs(speed) < 0.01f)
        speed = 0.0f;

    // Update position
    Vec3 fwd = getForward();
    pos.x += fwd.x * speed * delta;
    pos.z += fwd.z * speed * delta;

    // Update rotation for rendering
    // angleXZ() convention: returns -atan2(z, x)
    // For forward = (cos(heading), 0, sin(heading)):
    //   atan2(sin(heading), cos(heading)) = heading
    //   angleXZ = -heading
    rot.y = -heading;
}

void PlayerCar::update(const float delta)
{
    // Physics handled via handleInput called from Simulator
}

Vec3 PlayerCar::getCameraPos() const
{
    // The world is rendered with scale(10,10,10) then scale(1,1,-1) after
    // the view matrix. So the camera must operate in GL space where:
    //   GL.x = world.x * 10
    //   GL.y = world.y * 10
    //   GL.z = world.z * -10  (Z-flip)

    Vec3 fwd = getForward();
    float camDist = 18.0f;  // distance behind car (GL units)
    float camHeight = 7.0f; // height above car (GL units)

    // Car position in GL space
    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;

    // Forward in GL space (Z is flipped)
    float glFwdX = fwd.x;
    float glFwdZ = -fwd.z;

    Vec3 camPos;
    camPos.x = glX - glFwdX * camDist;
    camPos.y = glY + camHeight;
    camPos.z = glZ - glFwdZ * camDist;
    return camPos;
}

Vec3 PlayerCar::getCameraTarget() const
{
    // Target point slightly ahead of the car, in GL space
    Vec3 fwd = getForward();
    float lookAhead = 10.0f; // GL units

    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;

    float glFwdX = fwd.x;
    float glFwdZ = -fwd.z;

    Vec3 target;
    target.x = glX + glFwdX * lookAhead;
    target.y = glY + 1.5f;
    target.z = glZ + glFwdZ * lookAhead;
    return target;
}

// ========== DRAWING ==========
// Reuses the same visual style as the AI Car class but with player color

void PlayerCar::draw()
{
    translate(0, -0.02, 0);

    // ========== WHEELS WITH RIMS ==========
    for (int lr = -1; lr <= 1; lr += 2)
    {
        for (int fb = -1; fb <= 1; fb += 2)
        {
            float wx = fb * 0.065f;
            float wz = lr * 0.052f;
            setColor(0.10f, 0.10f, 0.10f);
            pushMatrix();
            translate(wx, 0.016f, wz);
            drawCube(0.032f, 0.032f, 0.016f);
            popMatrix();
            setColor(0.65f, 0.65f, 0.68f);
            pushMatrix();
            translate(wx, 0.016f, wz + lr * 0.009f);
            drawCube(0.018f, 0.018f, 0.003f);
            popMatrix();
        }
    }

    // ========== UNDERCARRIAGE ==========
    setColor(0.08f, 0.08f, 0.08f);
    pushMatrix();
    translate(0, 0.008f, 0);
    drawCube(0.18f, 0.008f, 0.08f);
    popMatrix();

    // ========== MAIN BODY ==========
    setColor(carColor);
    pushMatrix();
    translate(0, 0.05f, 0);

    // Lower body
    drawCube(0.21f, 0.05f, 0.105f);

    // Hood
    setColor(carColor);
    pushMatrix();
    translate(0.07f, 0.015f, 0);
    drawCube(0.06f, 0.02f, 0.10f);
    popMatrix();

    // Trunk
    pushMatrix();
    translate(-0.07f, 0.012f, 0);
    drawCube(0.06f, 0.018f, 0.10f);
    popMatrix();

    // Side skirts
    setColor(carColor * 0.5f);
    pushMatrix();
    translate(0, -0.022f, 0.053f);
    drawCube(0.19f, 0.006f, 0.004f);
    popMatrix();
    pushMatrix();
    translate(0, -0.022f, -0.053f);
    drawCube(0.19f, 0.006f, 0.004f);
    popMatrix();

    // Cabin / Roof
    drawRoof();

    popMatrix();

    // ========== FRONT GRILLE ==========
    setColor(0.18f, 0.18f, 0.20f);
    pushMatrix();
    translate(0.107f, 0.042f, 0);
    drawCube(0.004f, 0.025f, 0.06f);
    popMatrix();
    setColor(0.70f, 0.70f, 0.72f);
    pushMatrix();
    translate(0.108f, 0.048f, 0);
    drawCube(0.002f, 0.008f, 0.055f);
    popMatrix();
    pushMatrix();
    translate(0.108f, 0.038f, 0);
    drawCube(0.002f, 0.008f, 0.055f);
    popMatrix();

    // ========== HEADLIGHTS ==========
    int carPhase = Simulator::getInstance().getDayPhase();
    bool nightDriving = (carPhase == 0 || carPhase == 1 || carPhase == 5 || carPhase == 6);

    setColor(0.80f, 0.80f, 0.82f);
    pushMatrix();
    translate(0.107f, 0.05f, 0.038f);
    drawCube(0.006f, 0.018f, 0.022f);
    popMatrix();
    pushMatrix();
    translate(0.107f, 0.05f, -0.038f);
    drawCube(0.006f, 0.018f, 0.022f);
    popMatrix();

    if (nightDriving)
        setColor(1.0f, 0.98f, 0.85f);
    else
        setColor(0.95f, 0.93f, 0.80f);
    pushMatrix();
    translate(0.109f, 0.05f, 0.038f);
    drawCube(0.003f, 0.014f, 0.018f);
    popMatrix();
    pushMatrix();
    translate(0.109f, 0.05f, -0.038f);
    drawCube(0.003f, 0.014f, 0.018f);
    popMatrix();

    if (nightDriving)
    {
        setColor(0.90f, 0.85f, 0.60f);
        pushMatrix();
        translate(0.14f, 0.045f, 0.038f);
        drawCube(0.05f, 0.008f, 0.015f);
        popMatrix();
        pushMatrix();
        translate(0.14f, 0.045f, -0.038f);
        drawCube(0.05f, 0.008f, 0.015f);
        popMatrix();
    }

    // ========== TAILLIGHTS ==========
    if (nightDriving)
        setColor(0.80f, 0.05f, 0.05f);
    else
        setColor(0.55f, 0.02f, 0.02f);
    pushMatrix();
    translate(-0.107f, 0.05f, 0.038f);
    drawCube(0.005f, 0.018f, 0.018f);
    popMatrix();
    pushMatrix();
    translate(-0.107f, 0.05f, -0.038f);
    drawCube(0.005f, 0.018f, 0.018f);
    popMatrix();

    // Brake light when decelerating
    if (speed < -0.01f)
    {
        setColor(1, 0, 0);
        pushMatrix();
        translate(-0.05, 0.085, 0);
        drawCube(0.07, 0.003, 0.04);
        popMatrix();
    }

    // ========== BUMPERS ==========
    setColor(carColor * 0.55f);
    pushMatrix();
    translate(0.112f, 0.032f, 0);
    drawCube(0.006f, 0.022f, 0.095f);
    popMatrix();
    pushMatrix();
    translate(-0.112f, 0.032f, 0);
    drawCube(0.006f, 0.022f, 0.095f);
    popMatrix();

    // ========== SIDE MIRRORS ==========
    setColor(carColor * 0.85f);
    pushMatrix();
    translate(0.04f, 0.068f, 0.056f);
    drawCube(0.012f, 0.008f, 0.008f);
    popMatrix();
    pushMatrix();
    translate(0.04f, 0.068f, -0.056f);
    drawCube(0.012f, 0.008f, 0.008f);
    popMatrix();
    setColor(0.50f, 0.55f, 0.60f);
    pushMatrix();
    translate(0.04f, 0.068f, 0.061f);
    drawCube(0.008f, 0.006f, 0.002f);
    popMatrix();
    pushMatrix();
    translate(0.04f, 0.068f, -0.061f);
    drawCube(0.008f, 0.006f, 0.002f);
    popMatrix();

    // ========== PLAYER INDICATOR — roof light bar ==========
    setColor(0.1f, 0.6f, 1.0f);
    pushMatrix();
    translate(0, 0.125f, 0);
    drawCube(0.04f, 0.008f, 0.06f);
    popMatrix();
}

void PlayerCar::drawRoof()
{
    Vec3 a1(0, 0, -0.052f);
    Vec3 a2(0.020f, 0.048f, -0.040f);
    Vec3 a3(0.072f, 0.048f, -0.040f);
    Vec3 a4(0.110f, 0, -0.052f);
    Vec3 a5(0, 0, 0.052f);
    Vec3 a6(0.020f, 0.048f, 0.040f);
    Vec3 a7(0.072f, 0.048f, 0.040f);
    Vec3 a8(0.110f, 0, 0.052f);

    pushMatrix();
    translate(-0.072f, 0.025f, 0);
    beginDraw(QUADS);

    // Roof panel
    setColor(carColor);
    drawQuad(a2, a6, a7, a3);

    // Windshield
    setColor(0.12f, 0.20f, 0.28f);
    drawQuad(a4, a8, a7, a3);

    // Rear window
    setColor(0.15f, 0.22f, 0.30f);
    drawQuad(a1, a2, a6, a5);

    // Bottom
    setColor(0.18f, 0.30f, 0.38f);
    drawQuad(a1, a5, a8, a4);

    // Side windows
    setColor(0.15f, 0.28f, 0.35f);
    drawQuad(a5, a8, a7, a6);
    drawQuad(a1, a2, a3, a4);

    endDraw();

    // Window trim
    setColor(0.65f, 0.65f, 0.68f);
    pushMatrix();
    translate(0.093f, 0.024f, 0);
    drawCube(0.002f, 0.044f, 0.002f);
    popMatrix();

    popMatrix();
}

// ========== PlayerBus Implementation ==========

PlayerBus::PlayerBus(Vec3 startPos) : PlayerCar(startPos)
{
    acceleration = 1.2f;
    maxSpeed = 2.2f;
    turnSpeed = 65.0f; // slower turning
    friction = 0.8f;
    carColor = Vec3(0.9f, 0.2f, 0.15f); // Red bus
    id = "PLAYER_BUS";
}

Vec3 PlayerBus::getCameraPos() const
{
    Vec3 fwd = getForward();
    float camDist = 28.0f;
    float camHeight = 12.0f;
    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;
    Vec3 camPos;
    camPos.x = glX - fwd.x * camDist;
    camPos.y = glY + camHeight;
    camPos.z = glZ - (-fwd.z) * camDist;
    return camPos;
}

Vec3 PlayerBus::getCameraTarget() const
{
    Vec3 fwd = getForward();
    float lookAhead = 15.0f;
    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;
    Vec3 target;
    target.x = glX + fwd.x * lookAhead;
    target.y = glY + 3.0f;
    target.z = glZ + (-fwd.z) * lookAhead;
    return target;
}

void PlayerBus::draw()
{
    translate(0, -0.02, 0);

    // WHEELS
    for (int lr = -1; lr <= 1; lr += 2)
    {
        for (int fb = -1; fb <= 1; fb += 2)
        {
            float wx = fb * 0.18f;
            float wz = lr * 0.052f;
            setColor(0.10f, 0.10f, 0.10f);
            pushMatrix();
            translate(wx, 0.02f, wz);
            drawCube(0.04f, 0.04f, 0.018f);
            popMatrix();
            setColor(0.65f, 0.65f, 0.68f);
            pushMatrix();
            translate(wx, 0.02f, wz + lr * 0.01f);
            drawCube(0.025f, 0.025f, 0.003f);
            popMatrix();
        }
    }

    // BODY
    setColor(carColor);
    pushMatrix();
    translate(0, 0.08f, 0);
    drawCube(0.5f, 0.12f, 0.11f);
    popMatrix();

    // WINDOWS
    setColor(0.15f, 0.2f, 0.25f);
    pushMatrix();
    translate(0.252f, 0.09f, 0);
    drawCube(0.002f, 0.06f, 0.09f); // front
    popMatrix();
    
    for (int side = -1; side <= 1; side += 2)
    {
        for (int w = 0; w < 4; w++)
        {
            pushMatrix();
            translate(0.15f - w * 0.1f, 0.09f, side * 0.056f);
            drawCube(0.08f, 0.05f, 0.002f);
            popMatrix();
        }
    }

    // HEADLIGHTS
    setColor(0.95f, 0.93f, 0.80f);
    pushMatrix();
    translate(0.251f, 0.04f, 0.04f);
    drawCube(0.002f, 0.015f, 0.02f);
    popMatrix();
    pushMatrix();
    translate(0.251f, 0.04f, -0.04f);
    drawCube(0.002f, 0.015f, 0.02f);
    popMatrix();

    // TAILLIGHTS
    setColor(0.8f, 0.1f, 0.1f);
    pushMatrix();
    translate(-0.251f, 0.05f, 0.04f);
    drawCube(0.002f, 0.02f, 0.015f);
    popMatrix();
    pushMatrix();
    translate(-0.251f, 0.05f, -0.04f);
    drawCube(0.002f, 0.02f, 0.015f);
    popMatrix();
}

// ========== PlayerBike Implementation ==========

PlayerBike::PlayerBike(Vec3 startPos) : PlayerCar(startPos)
{
    acceleration = 4.5f;
    maxSpeed = 4.5f;
    turnSpeed = 160.0f; // fast turning
    friction = 1.0f;
    carColor = Vec3(0.2f, 0.8f, 0.2f); // Green bike
    id = "PLAYER_BIKE";
}

Vec3 PlayerBike::getCameraPos() const
{
    Vec3 fwd = getForward();
    float camDist = 12.0f;
    float camHeight = 5.0f;
    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;
    Vec3 camPos;
    camPos.x = glX - fwd.x * camDist;
    camPos.y = glY + camHeight;
    camPos.z = glZ - (-fwd.z) * camDist;
    return camPos;
}

Vec3 PlayerBike::getCameraTarget() const
{
    Vec3 fwd = getForward();
    float lookAhead = 8.0f;
    float glX = pos.x * 10.0f;
    float glY = pos.y * 10.0f;
    float glZ = pos.z * -10.0f;
    Vec3 target;
    target.x = glX + fwd.x * lookAhead;
    target.y = glY + 1.0f;
    target.z = glZ + (-fwd.z) * lookAhead;
    return target;
}

void PlayerBike::draw()
{
    translate(0, -0.02, 0);

    // WHEELS (2 wheels inline)
    setColor(0.10f, 0.10f, 0.10f);
    pushMatrix();
    translate(0.045f, 0.016f, 0);
    drawCube(0.032f, 0.032f, 0.01f);
    popMatrix();
    pushMatrix();
    translate(-0.045f, 0.016f, 0);
    drawCube(0.032f, 0.032f, 0.01f);
    popMatrix();

    // Rims
    setColor(0.65f, 0.65f, 0.68f);
    for(int i=-1; i<=1; i+=2) {
        for(int j=-1; j<=1; j+=2) {
            pushMatrix();
            translate(i*0.045f, 0.016f, j*0.006f);
            drawCube(0.018f, 0.018f, 0.002f);
            popMatrix();
        }
    }

    // BODY
    setColor(carColor);
    pushMatrix();
    translate(0, 0.035f, 0);
    drawCube(0.08f, 0.03f, 0.02f);
    popMatrix();

    // HANDLEBARS
    setColor(0.1f, 0.1f, 0.1f);
    pushMatrix();
    translate(0.03f, 0.055f, 0);
    drawCube(0.01f, 0.01f, 0.05f);
    popMatrix();

    // HEADLIGHT
    setColor(0.95f, 0.93f, 0.80f);
    pushMatrix();
    translate(0.041f, 0.04f, 0);
    drawCube(0.002f, 0.01f, 0.01f);
    popMatrix();
}
