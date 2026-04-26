///   File: Colors.cpp


#include "Colors.h"

Vec3 Colors::getRandomColor()
{
    static Colors *instance;
    if (instance == nullptr)
    {
        instance = new Colors();
    }
    return instance->pickRandom();
}

Colors::Colors()
{
    amount = 16;
    colors = new Vec3[amount];

    colors[0]  = Vec3(0.75f, 0.75f, 0.75f);   // Silver
    colors[1]  = Vec3(0.12f, 0.12f, 0.12f);   // Black
    colors[2]  = Vec3(0.92f, 0.91f, 0.88f);   // Pearl white
    colors[3]  = Vec3(0.10f, 0.15f, 0.35f);   // Dark navy
    colors[4]  = Vec3(0.50f, 0.06f, 0.08f);   // Burgundy
    colors[5]  = Vec3(0.08f, 0.28f, 0.12f);   // Forest green
    colors[6]  = Vec3(0.30f, 0.30f, 0.32f);   // Charcoal
    colors[7]  = Vec3(0.15f, 0.20f, 0.42f);   // Navy blue
    colors[8]  = Vec3(0.62f, 0.08f, 0.05f);   // Deep red
    colors[9]  = Vec3(0.85f, 0.82f, 0.75f);   // Pearl
    colors[10] = Vec3(0.55f, 0.40f, 0.20f);   // Bronze
    colors[11] = Vec3(0.22f, 0.10f, 0.32f);   // Midnight purple
    colors[12] = Vec3(0.00f, 0.25f, 0.10f);   // British racing green
    colors[13] = Vec3(0.80f, 0.35f, 0.08f);   // Sunset orange
    colors[14] = Vec3(0.35f, 0.45f, 0.55f);   // Steel blue
    colors[15] = Vec3(0.78f, 0.72f, 0.58f);   // Champagne
}

Vec3 Colors::pickRandom()
{
    return colors[rand() % amount];
}
