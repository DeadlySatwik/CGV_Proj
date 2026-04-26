///   File: Environment.cpp

#include "Environment.h"

// ============================================================
//  TREE — brown trunk + layered green canopy
// ============================================================

Tree::Tree(Vec3 position)
{
    pos = position;
    // Hash-based variety from position
    int h = (int)(pos.x * 7 + pos.z * 13);
    if (h < 0) h = -h;
    variant = h % 4;
    trunkH = 0.15f + (variant % 3) * 0.05f;
    canopyR = 0.12f + (variant % 2) * 0.04f;
}

void Tree::draw()
{
    // --- Trunk ---
    setColor(0.35f, 0.22f, 0.08f);
    pushMatrix();
    translate(0, trunkH * 0.5f, 0);
    drawCube(0.04f, trunkH, 0.04f);
    popMatrix();

    // --- Canopy layers (stacked, getting smaller toward top) ---
    float baseY = trunkH;

    if (variant == 0)
    {
        // Round bushy tree — 3 stacked layers
        setColor(0.15f, 0.50f, 0.12f);
        pushMatrix();
        translate(0, baseY + 0.06f, 0);
        drawCube(canopyR * 2, 0.12f, canopyR * 2);
        popMatrix();

        setColor(0.12f, 0.55f, 0.10f);
        pushMatrix();
        translate(0, baseY + 0.15f, 0);
        drawCube(canopyR * 1.6f, 0.10f, canopyR * 1.6f);
        popMatrix();

        setColor(0.10f, 0.60f, 0.08f);
        pushMatrix();
        translate(0, baseY + 0.22f, 0);
        drawCube(canopyR * 1.0f, 0.08f, canopyR * 1.0f);
        popMatrix();
    }
    else if (variant == 1)
    {
        // Tall conical tree (like a cypress)
        setColor(0.08f, 0.40f, 0.10f);
        pushMatrix();
        translate(0, baseY + 0.08f, 0);
        drawCube(0.18f, 0.16f, 0.18f);
        popMatrix();

        setColor(0.06f, 0.45f, 0.08f);
        pushMatrix();
        translate(0, baseY + 0.20f, 0);
        drawCube(0.12f, 0.14f, 0.12f);
        popMatrix();

        setColor(0.05f, 0.50f, 0.06f);
        pushMatrix();
        translate(0, baseY + 0.30f, 0);
        drawCube(0.06f, 0.10f, 0.06f);
        popMatrix();
    }
    else if (variant == 2)
    {
        // Wide spreading tree
        setColor(0.18f, 0.48f, 0.15f);
        pushMatrix();
        translate(0, baseY + 0.06f, 0);
        drawCube(0.30f, 0.10f, 0.30f);
        popMatrix();

        setColor(0.15f, 0.52f, 0.12f);
        pushMatrix();
        translate(0, baseY + 0.14f, 0);
        drawCube(0.22f, 0.08f, 0.22f);
        popMatrix();
    }
    else
    {
        // Small ornamental tree
        setColor(0.20f, 0.55f, 0.18f);
        pushMatrix();
        translate(0, baseY + 0.05f, 0);
        drawCube(0.16f, 0.10f, 0.16f);
        popMatrix();

        setColor(0.22f, 0.58f, 0.15f);
        pushMatrix();
        translate(0, baseY + 0.12f, 0);
        drawCube(0.10f, 0.08f, 0.10f);
        popMatrix();
    }
}

// ============================================================
//  LAMPPOST — thin dark pole + yellow lamp, ON/OFF by day phase
// ============================================================

#include "Simulator.h"

Lamppost::Lamppost(Vec3 position)
{
    pos = position;
}

void Lamppost::draw()
{
    int phase = Simulator::getInstance().getDayPhase();
    // Lamp is ON during: Night(0), Dawn(1), Evening(5), Dusk(6)
    bool lampOn = (phase == 0 || phase == 1 || phase == 5 || phase == 6);

    // --- Pole ---
    setColor(0.20f, 0.20f, 0.22f);
    pushMatrix();
    translate(0, 0.20f, 0);
    drawCube(0.02f, 0.40f, 0.02f);
    popMatrix();

    // --- Arm extending outward ---
    setColor(0.22f, 0.22f, 0.24f);
    pushMatrix();
    translate(0.05f, 0.38f, 0);
    drawCube(0.10f, 0.015f, 0.015f);
    popMatrix();

    // --- Lamp head ---
    if (lampOn)
    {
        // Bright warm glow
        float brightness = (phase == 0) ? 1.0f : (phase == 6 ? 0.85f : 0.7f);
        setColor(1.0f * brightness, 0.92f * brightness, 0.50f * brightness);
        pushMatrix();
        translate(0.09f, 0.36f, 0);
        drawCube(0.04f, 0.025f, 0.04f);
        popMatrix();

        // Glow halo (larger, slightly dimmer)
        setColor(0.90f * brightness, 0.80f * brightness, 0.30f * brightness);
        pushMatrix();
        translate(0.09f, 0.36f, 0);
        drawCube(0.07f, 0.04f, 0.07f);
        popMatrix();

        // Light cone on ground (subtle bright patch)
        setColor(0.60f * brightness, 0.55f * brightness, 0.25f * brightness);
        pushMatrix();
        translate(0.09f, 0.002f, 0);
        drawCube(0.20f, 0.002f, 0.20f);
        popMatrix();
    }
    else
    {
        // OFF — dark grey lamp
        setColor(0.30f, 0.30f, 0.30f);
        pushMatrix();
        translate(0.09f, 0.36f, 0);
        drawCube(0.04f, 0.025f, 0.04f);
        popMatrix();
    }

    // --- Base plate ---
    setColor(0.18f, 0.18f, 0.20f);
    pushMatrix();
    translate(0, 0.005f, 0);
    drawCube(0.05f, 0.01f, 0.05f);
    popMatrix();
}

// ============================================================
//  BENCH — brown wooden seat + backrest
// ============================================================

Bench::Bench(Vec3 position)
{
    pos = position;
    // Rotation based on position hash
    int h = (int)(pos.x * 11 + pos.z * 17);
    if (h < 0) h = -h;
    angle = (h % 4) * 90.0f;
}

void Bench::draw()
{
    pushMatrix();
    rotateY(angle);

    // --- Seat ---
    setColor(0.45f, 0.30f, 0.12f);
    pushMatrix();
    translate(0, 0.04f, 0);
    drawCube(0.15f, 0.015f, 0.06f);
    popMatrix();

    // --- Backrest ---
    setColor(0.40f, 0.25f, 0.10f);
    pushMatrix();
    translate(0, 0.07f, -0.025f);
    drawCube(0.15f, 0.05f, 0.01f);
    popMatrix();

    // --- Legs (4 small cubes) ---
    setColor(0.25f, 0.25f, 0.25f);
    pushMatrix(); translate(-0.06f, 0.02f, 0.02f); drawCube(0.01f, 0.04f, 0.01f); popMatrix();
    pushMatrix(); translate( 0.06f, 0.02f, 0.02f); drawCube(0.01f, 0.04f, 0.01f); popMatrix();
    pushMatrix(); translate(-0.06f, 0.02f,-0.02f); drawCube(0.01f, 0.04f, 0.01f); popMatrix();
    pushMatrix(); translate( 0.06f, 0.02f,-0.02f); drawCube(0.01f, 0.04f, 0.01f); popMatrix();

    popMatrix();
}
