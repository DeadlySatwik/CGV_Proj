///   File: PlayerCar.cpp

#include "PlayerCar.h"
#include "Simulator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PlayerCar::PlayerCar(Vec3 startPos)
{
    pos = startPos;
    rot = Vec3(0, 0, 0);

    speed = 0.0f;
    heading = 0.0f;       // facing +X initially
    acceleration = 2.5f;
    maxSpeed = 3.0f;
    turnSpeed = 120.0f;   // degrees per second
    friction = 1.2f;

    oldPos = pos;
    oldHeading = heading;

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
    rot.y = -heading;  // angleXZ convention: rot.y = -angle
}

void PlayerCar::handleInput(unsigned int inputMap, const float delta)
{
    // Steering (only when moving)
    // angleXZ() returns -atan2(z,x), so the visual rotation is -heading.
    // LEFT arrow should turn the car visually left (decrease heading in math coords
    // because the Z-flip makes CCW appear CW).
    // RIGHT arrow should turn visually right (increase heading).
    if (fabs(speed) > 0.05f)
    {
        float steerFactor = 1.0f;
        // Reduce turning at high speed for stability
        if (fabs(speed) > 1.5f)
            steerFactor = 1.5f / fabs(speed);

        if (inputMap & INPUT_STEER_LEFT)
            heading += turnSpeed * steerFactor * delta;
        if (inputMap & INPUT_STEER_RIGHT)
            heading -= turnSpeed * steerFactor * delta;
    }

    // Acceleration / braking
    if (inputMap & INPUT_ACCEL)
    {
        speed += acceleration * delta;
        if (speed > maxSpeed) speed = maxSpeed;
    }
    else if (inputMap & INPUT_BRAKE)
    {
        speed -= acceleration * 1.5f * delta;
        if (speed < -maxSpeed * 0.3f) speed = -maxSpeed * 0.3f;
    }
    else
    {
        // Friction deceleration
        if (speed > 0)
        {
            speed -= friction * delta;
            if (speed < 0) speed = 0;
        }
        else if (speed < 0)
        {
            speed += friction * delta;
            if (speed > 0) speed = 0;
        }
    }

    // Update position
    Vec3 fwd = getForward();
    pos.x += fwd.x * speed * delta;
    pos.z += fwd.z * speed * delta;

    // Keep on ground plane
    pos.y = 0.0f;

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
    float camDist = 18.0f;    // distance behind car (GL units)
    float camHeight = 7.0f;   // height above car (GL units)

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
    float lookAhead = 10.0f;  // GL units

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
            pushMatrix(); translate(wx, 0.016f, wz); drawCube(0.032f, 0.032f, 0.016f); popMatrix();
            setColor(0.65f, 0.65f, 0.68f);
            pushMatrix(); translate(wx, 0.016f, wz + lr * 0.009f); drawCube(0.018f, 0.018f, 0.003f); popMatrix();
        }
    }

    // ========== UNDERCARRIAGE ==========
    setColor(0.08f, 0.08f, 0.08f);
    pushMatrix(); translate(0, 0.008f, 0); drawCube(0.18f, 0.008f, 0.08f); popMatrix();

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
    pushMatrix(); translate(0, -0.022f, 0.053f); drawCube(0.19f, 0.006f, 0.004f); popMatrix();
    pushMatrix(); translate(0, -0.022f, -0.053f); drawCube(0.19f, 0.006f, 0.004f); popMatrix();

    // Cabin / Roof
    drawRoof();

    popMatrix();

    // ========== FRONT GRILLE ==========
    setColor(0.18f, 0.18f, 0.20f);
    pushMatrix(); translate(0.107f, 0.042f, 0); drawCube(0.004f, 0.025f, 0.06f); popMatrix();
    setColor(0.70f, 0.70f, 0.72f);
    pushMatrix(); translate(0.108f, 0.048f, 0); drawCube(0.002f, 0.008f, 0.055f); popMatrix();
    pushMatrix(); translate(0.108f, 0.038f, 0); drawCube(0.002f, 0.008f, 0.055f); popMatrix();

    // ========== HEADLIGHTS ==========
    int carPhase = Simulator::getInstance().getDayPhase();
    bool nightDriving = (carPhase == 0 || carPhase == 1 || carPhase == 5 || carPhase == 6);

    setColor(0.80f, 0.80f, 0.82f);
    pushMatrix(); translate(0.107f, 0.05f, 0.038f); drawCube(0.006f, 0.018f, 0.022f); popMatrix();
    pushMatrix(); translate(0.107f, 0.05f, -0.038f); drawCube(0.006f, 0.018f, 0.022f); popMatrix();

    if (nightDriving) setColor(1.0f, 0.98f, 0.85f);
    else setColor(0.95f, 0.93f, 0.80f);
    pushMatrix(); translate(0.109f, 0.05f, 0.038f); drawCube(0.003f, 0.014f, 0.018f); popMatrix();
    pushMatrix(); translate(0.109f, 0.05f, -0.038f); drawCube(0.003f, 0.014f, 0.018f); popMatrix();

    if (nightDriving)
    {
        setColor(0.90f, 0.85f, 0.60f);
        pushMatrix(); translate(0.14f, 0.045f, 0.038f); drawCube(0.05f, 0.008f, 0.015f); popMatrix();
        pushMatrix(); translate(0.14f, 0.045f, -0.038f); drawCube(0.05f, 0.008f, 0.015f); popMatrix();
    }

    // ========== TAILLIGHTS ==========
    if (nightDriving) setColor(0.80f, 0.05f, 0.05f);
    else setColor(0.55f, 0.02f, 0.02f);
    pushMatrix(); translate(-0.107f, 0.05f, 0.038f); drawCube(0.005f, 0.018f, 0.018f); popMatrix();
    pushMatrix(); translate(-0.107f, 0.05f, -0.038f); drawCube(0.005f, 0.018f, 0.018f); popMatrix();

    // Brake light when decelerating
    if (speed < -0.01f)
    {
        setColor(1, 0, 0);
        pushMatrix(); translate(-0.05, 0.085, 0); drawCube(0.07, 0.003, 0.04); popMatrix();
    }

    // ========== BUMPERS ==========
    setColor(carColor * 0.55f);
    pushMatrix(); translate(0.112f, 0.032f, 0); drawCube(0.006f, 0.022f, 0.095f); popMatrix();
    pushMatrix(); translate(-0.112f, 0.032f, 0); drawCube(0.006f, 0.022f, 0.095f); popMatrix();

    // ========== SIDE MIRRORS ==========
    setColor(carColor * 0.85f);
    pushMatrix(); translate(0.04f, 0.068f, 0.056f); drawCube(0.012f, 0.008f, 0.008f); popMatrix();
    pushMatrix(); translate(0.04f, 0.068f, -0.056f); drawCube(0.012f, 0.008f, 0.008f); popMatrix();
    setColor(0.50f, 0.55f, 0.60f);
    pushMatrix(); translate(0.04f, 0.068f, 0.061f); drawCube(0.008f, 0.006f, 0.002f); popMatrix();
    pushMatrix(); translate(0.04f, 0.068f, -0.061f); drawCube(0.008f, 0.006f, 0.002f); popMatrix();

    // ========== PLAYER INDICATOR — roof light bar ==========
    setColor(0.1f, 0.6f, 1.0f);
    pushMatrix(); translate(0, 0.125f, 0); drawCube(0.04f, 0.008f, 0.06f); popMatrix();
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
    pushMatrix(); translate(0.093f, 0.024f, 0); drawCube(0.002f, 0.044f, 0.002f); popMatrix();

    popMatrix();
}
