///   File: Pedestrian.cpp
///   Pedestrian state machine, movement, and detailed humanoid rendering.

#include "Pedestrian.h"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float Pedestrian::WAIT_MIN_TIME = 0.5f;
const float Pedestrian::DESPAWN_DELAY = 1.5f;

// Color palettes for pedestrian variety
static const Vec3 shirtPalette[] = {
    Vec3(0.20f, 0.35f, 0.65f),  // blue shirt
    Vec3(0.65f, 0.18f, 0.18f),  // red shirt
    Vec3(0.15f, 0.50f, 0.25f),  // green shirt
    Vec3(0.55f, 0.45f, 0.15f),  // yellow-brown
    Vec3(0.40f, 0.20f, 0.50f),  // purple
    Vec3(0.70f, 0.35f, 0.10f),  // orange
    Vec3(0.25f, 0.25f, 0.25f),  // dark grey
    Vec3(0.80f, 0.80f, 0.75f),  // white
};

static const Vec3 pantsPalette[] = {
    Vec3(0.15f, 0.15f, 0.30f),  // dark blue jeans
    Vec3(0.10f, 0.10f, 0.10f),  // black
    Vec3(0.30f, 0.25f, 0.15f),  // khaki
    Vec3(0.20f, 0.20f, 0.22f),  // charcoal
};

static const int NUM_SHIRT_COLORS = sizeof(shirtPalette) / sizeof(shirtPalette[0]);
static const int NUM_PANTS_COLORS = sizeof(pantsPalette) / sizeof(pantsPalette[0]);

Pedestrian::Pedestrian(const Vec3& spawnPosition, const Vec3& targetPosition, float crossingSpeed)
    : state(PED_SPAWNED),
      position(spawnPosition),
      spawnPos(spawnPosition),
      targetPos(targetPosition),
      speed(crossingSpeed),
      waitTimer(0.0f),
      crossProgress(0.0f),
      despawnTimer(0.0f)
{
    // Randomize appearance
    variant = rand() % 100;
    bodyColor  = shirtPalette[variant % NUM_SHIRT_COLORS];
    pantsColor = pantsPalette[(variant / NUM_SHIRT_COLORS) % NUM_PANTS_COLORS];
    bodyHeight = 0.13f + (variant % 5) * 0.005f; // slight height variation
}

void Pedestrian::update(float delta, bool signalAllowsCrossing)
{
    switch (state)
    {
    case PED_SPAWNED:
        // Immediately transition to waiting
        state = PED_WAITING;
        waitTimer = 0.0f;
        break;

    case PED_WAITING:
        waitTimer += delta;
        if (signalAllowsCrossing && waitTimer >= WAIT_MIN_TIME)
        {
            state = PED_CROSSING;
            crossProgress = 0.0f;
        }
        break;

    case PED_CROSSING:
    {
        // Calculate total distance
        float dx = targetPos.x - spawnPos.x;
        float dz = targetPos.z - spawnPos.z;
        float totalDist = sqrt(dx * dx + dz * dz);
        if (totalDist < 0.01f) totalDist = 0.01f;

        // Advance progress
        float progressDelta = (speed * delta) / totalDist;
        crossProgress += progressDelta;

        if (crossProgress >= 1.0f)
        {
            crossProgress = 1.0f;
            position = targetPos;
            state = PED_CLEARED;
            despawnTimer = 0.0f;
        }
        else
        {
            // Lerp position
            position.x = spawnPos.x + (targetPos.x - spawnPos.x) * crossProgress;
            position.y = spawnPos.y + (targetPos.y - spawnPos.y) * crossProgress;
            position.z = spawnPos.z + (targetPos.z - spawnPos.z) * crossProgress;
        }
        break;
    }

    case PED_CLEARED:
        despawnTimer += delta;
        if (despawnTimer >= DESPAWN_DELAY)
        {
            state = PED_DESPAWNED;
        }
        break;

    case PED_DESPAWNED:
        // Nothing to do, manager will clean up
        break;
    }
}

// ========== Drawing ==========

void Pedestrian::drawPedestrian()
{
    if (state == PED_DESPAWNED) return;

    // Walking animation cycle
    float walkCycle = 0.0f;
    if (state == PED_CROSSING)
    {
        walkCycle = crossProgress * 20.0f; // oscillate during crossing
    }

    pushMatrix();
    translate(position.x, position.y, position.z);

    // Face toward target direction
    float dx = targetPos.x - spawnPos.x;
    float dz = targetPos.z - spawnPos.z;
    float angle = atan2(dx, -dz) * 180.0f / (float)M_PI;
    rotateY(angle);

    drawLegs(walkCycle);
    drawBody();
    drawArms(walkCycle);
    drawHead();

    popMatrix();
}

void Pedestrian::drawBody()
{
    // Torso
    setColor(bodyColor);
    pushMatrix();
    translate(0.0f, bodyHeight * 0.65f, 0.0f);
    drawCube(0.03f, bodyHeight * 0.4f, 0.02f);
    popMatrix();
}

void Pedestrian::drawHead()
{
    // Head (skin-colored sphere approximated as cube)
    setColor(0.85f, 0.72f, 0.58f);
    pushMatrix();
    translate(0.0f, bodyHeight + 0.015f, 0.0f);
    drawCube(0.018f, 0.018f, 0.018f);
    popMatrix();

    // Hair on top
    Vec3 hairColor(0.12f, 0.08f, 0.05f);
    if (variant % 3 == 0) hairColor = Vec3(0.45f, 0.30f, 0.10f); // brown
    if (variant % 3 == 2) hairColor = Vec3(0.05f, 0.05f, 0.05f);  // black
    setColor(hairColor);
    pushMatrix();
    translate(0.0f, bodyHeight + 0.030f, 0.0f);
    drawCube(0.019f, 0.008f, 0.019f);
    popMatrix();
}

void Pedestrian::drawLegs(float walkCycle)
{
    setColor(pantsColor);

    float legSwing = 0.0f;
    if (walkCycle != 0.0f)
    {
        legSwing = sin(walkCycle) * 12.0f; // degrees
    }

    // Left leg
    pushMatrix();
    translate(0.012f, bodyHeight * 0.2f, 0.0f);
    rotateX(legSwing);
    drawCube(0.012f, bodyHeight * 0.35f, 0.012f);
    popMatrix();

    // Right leg
    pushMatrix();
    translate(-0.012f, bodyHeight * 0.2f, 0.0f);
    rotateX(-legSwing);
    drawCube(0.012f, bodyHeight * 0.35f, 0.012f);
    popMatrix();

    // Shoes
    setColor(0.15f, 0.12f, 0.10f);
    float shoeY = bodyHeight * 0.02f;
    pushMatrix();
    translate(0.012f, shoeY, 0.0f);
    drawCube(0.013f, 0.008f, 0.016f);
    popMatrix();
    pushMatrix();
    translate(-0.012f, shoeY, 0.0f);
    drawCube(0.013f, 0.008f, 0.016f);
    popMatrix();
}

void Pedestrian::drawArms(float walkCycle)
{
    setColor(bodyColor);

    float armSwing = 0.0f;
    if (walkCycle != 0.0f)
    {
        armSwing = sin(walkCycle + (float)M_PI) * 10.0f; // opposite to legs
    }

    // Left arm
    pushMatrix();
    translate(0.04f, bodyHeight * 0.65f, 0.0f);
    rotateX(armSwing);
    drawCube(0.01f, bodyHeight * 0.3f, 0.01f);
    popMatrix();

    // Right arm
    pushMatrix();
    translate(-0.04f, bodyHeight * 0.65f, 0.0f);
    rotateX(-armSwing);
    drawCube(0.01f, bodyHeight * 0.3f, 0.01f);
    popMatrix();

    // Hands (skin color)
    setColor(0.85f, 0.72f, 0.58f);
    pushMatrix();
    translate(0.04f, bodyHeight * 0.43f, 0.0f);
    drawCube(0.008f, 0.008f, 0.008f);
    popMatrix();
    pushMatrix();
    translate(-0.04f, bodyHeight * 0.43f, 0.0f);
    drawCube(0.008f, 0.008f, 0.008f);
    popMatrix();
}
