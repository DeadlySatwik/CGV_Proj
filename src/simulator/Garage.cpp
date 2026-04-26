///   File: Garage.cpp

#include "Garage.h"
#include "Simulator.h"
using namespace std;

int Garage::vehiclesCounter = 0;
const float Garage::GATE_SPEED = 120.0f; // degrees per second (opens in ~0.75s)
const float Garage::GATE_HOLD = 2.5f;    // seconds the gate stays fully open

Garage::Garage(Vec3 p, Cross *c) : Driveable(p, c)
{
    curTimeSpot = 0;
    frecSpot = 4;

    curTimeDelete = 0;
    frecDelete = 6;

    isReadyToSpot = false;
    isReadyToDelete = false;

    spottedVehicles = 0;
    maxVehicles = 15;

    // Gates start fully closed
    gateStateOut = GATE_CLOSED;
    gateAngleOut = 0.0f;
    gateTimerOut = 0.0f;

    gateStateIn = GATE_CLOSED;
    gateAngleIn = 0.0f;
    gateTimerIn = 0.0f;
}

void Garage::draw()
{
    // Draw the garage's own driveway so the door doesn't look like it opens into the grass
    // Driveable::draw() expects world coordinates, but Simulator::draw loop has already translated by pos
    pushMatrix();
    translate(-pos.x, -pos.y, -pos.z);
    Driveable::draw();
    popMatrix();

    // ========== Hash from garage ID for procedural variety ==========
    unsigned int hash = 5381;
    for (size_t i = 0; i < id.size(); i++)
        hash = ((hash << 5) + hash) + (unsigned char)id[i];

    int style = hash % 3; // 0=office, 1=apartment, 2=house
    int colorIdx = (hash >> 3) % 8;
    int roofStyle = (hash >> 6) % 3; // 0=flat, 1=peaked, 2=parapet
    int doorStyle = (hash >> 9) % 3; // 0=single, 1=double, 2=shopfront
    int detailSeed = (hash >> 12) % 7;

    // Height varies by style
    float height;
    if (style == 0)
        height = 0.8f + (hash % 5) * 0.12f; // office: 0.8-1.28
    else if (style == 1)
        height = 0.6f + (hash % 4) * 0.10f; // apartment: 0.6-0.9
    else
        height = 0.35f + (hash % 3) * 0.08f; // house: 0.35-0.51

    float halfH = height * 0.5f;
    float buildW = (style == 2) ? 0.55f : 0.7f; // houses are narrower
    float buildD = 1.0f;

    // ========== Color palette (8 muted architectural tones) ==========
    Vec3 wallColors[8] = {
        Vec3(0.62f, 0.60f, 0.58f), // warm grey
        Vec3(0.72f, 0.65f, 0.52f), // tan/sandstone
        Vec3(0.50f, 0.38f, 0.28f), // brown
        Vec3(0.82f, 0.78f, 0.70f), // cream
        Vec3(0.65f, 0.35f, 0.25f), // terracotta
        Vec3(0.48f, 0.52f, 0.58f), // blue-grey
        Vec3(0.45f, 0.48f, 0.35f), // olive
        Vec3(0.40f, 0.42f, 0.45f)  // slate
    };
    Vec3 wallColor = wallColors[colorIdx];

    // Derive accent colors from wall color
    Vec3 trimColor = wallColor * 0.7f;
    Vec3 roofColor = wallColor * 0.55f;
    Vec3 windowColor(0.18f, 0.35f, 0.42f); // dark teal glass
    Vec3 windowFrame(0.75f, 0.73f, 0.68f); // light frame
    Vec3 doorColor(0.30f, 0.18f, 0.08f);   // dark wood

    // Day phase for window glow
    int bldgPhase = Simulator::getInstance().getDayPhase();
    bool isNightTime = (bldgPhase == 0 || bldgPhase == 1 || bldgPhase == 5 || bldgPhase == 6);
    // Window colors: at night, some windows glow warm (occupied), others dark (empty)
    Vec3 windowLit(0.85f, 0.75f, 0.45f);  // warm occupied room
    Vec3 windowDark(0.08f, 0.10f, 0.14f); // dark empty room
    Vec3 windowDay(0.18f, 0.35f, 0.42f);  // daytime teal glass

    pushMatrix();
    translate(0, halfH + CURB_H, 0);
    // The building's procedural front face is -X.
    // direction.angleXZ() aligns +Z with the road.
    // So we add 90 degrees to align -X with the road!
    float angle = direction.angleXZ();
    // Snap to nearest 90 degrees to ensure garages always face the road grid cleanly
    // (This fixes the visual issue where diagonal direction vectors make the building twisted)
    angle = round(angle / 90.0f) * 90.0f;
    rotateY(angle + 180.0f);

    // ========== MAIN BUILDING BODY (Hollowed out for garage) ==========
    setColor(wallColor);
    float hw = buildW * 0.5f;
    float hd = buildD * 0.5f;

    // Gate dimensions and hole setup
    float gateW = buildD * 0.55f;
    float gateH = 0.22f;
    float sideWallW = (buildD - gateW) * 0.5f;

    // Top roof
    pushMatrix();
    translate(0, halfH - 0.01f, 0);
    drawCube(buildW, 0.02f, buildD);
    popMatrix();
    // Back wall (+X)
    pushMatrix();
    translate(hw - 0.01f, 0, 0);
    drawCube(0.02f, height, buildD);
    popMatrix();
    // Left wall (+Z)
    pushMatrix();
    translate(0, 0, hd - 0.01f);
    drawCube(buildW, height, 0.02f);
    popMatrix();
    // Right wall (-Z)
    pushMatrix();
    translate(0, 0, -hd + 0.01f);
    drawCube(buildW, height, 0.02f);
    popMatrix();

    // Front-top wall (above the gate)
    pushMatrix();
    translate(-hw + 0.01f, -halfH + gateH + (height - gateH) * 0.5f, 0);
    drawCube(0.02f, height - gateH, buildD);
    popMatrix();
    // Front-left wall (side of gate)
    pushMatrix();
    translate(-hw + 0.01f, -halfH + gateH * 0.5f, gateW * 0.5f + sideWallW * 0.5f);
    drawCube(0.02f, gateH, sideWallW);
    popMatrix();
    // Front-right wall (side of gate)
    pushMatrix();
    translate(-hw + 0.01f, -halfH + gateH * 0.5f, -gateW * 0.5f - sideWallW * 0.5f);
    drawCube(0.02f, gateH, sideWallW);
    popMatrix();

    // Floor of the garage interior
    setColor(Vec3(0.2f, 0.2f, 0.2f)); // dark floor
    pushMatrix();
    translate(0, -halfH + 0.005f, 0);
    drawCube(buildW, 0.01f, buildD);
    popMatrix();

    // Interior walls (dark)
    setColor(Vec3(0.15f, 0.15f, 0.15f));
    // Back interior
    pushMatrix();
    translate(hw - 0.03f, -halfH + gateH * 0.5f, 0);
    drawCube(0.01f, gateH, buildD);
    popMatrix();
    // Left interior
    pushMatrix();
    translate(0, -halfH + gateH * 0.5f, hd - 0.03f);
    drawCube(buildW, gateH, 0.01f);
    popMatrix();
    // Right interior
    pushMatrix();
    translate(0, -halfH + gateH * 0.5f, -hd + 0.03f);
    drawCube(buildW, gateH, 0.01f);
    popMatrix();

    // ========== FOUNDATION / BASE STRIP ==========
    setColor(trimColor * 0.8f);
    // Back strip
    pushMatrix();
    translate(hw + 0.01f, -halfH + 0.015f, 0);
    drawCube(0.02f, 0.03f, buildD + 0.02f);
    popMatrix();
    // Left strip
    pushMatrix();
    translate(0, -halfH + 0.015f, hd + 0.01f);
    drawCube(buildW + 0.02f, 0.03f, 0.02f);
    popMatrix();
    // Right strip
    pushMatrix();
    translate(0, -halfH + 0.015f, -hd - 0.01f);
    drawCube(buildW + 0.02f, 0.03f, 0.02f);
    popMatrix();
    // Front-left strip
    pushMatrix();
    translate(-hw - 0.01f, -halfH + 0.015f, gateW * 0.5f + sideWallW * 0.5f);
    drawCube(0.02f, 0.03f, sideWallW);
    popMatrix();
    // Front-right strip
    pushMatrix();
    translate(-hw - 0.01f, -halfH + 0.015f, -gateW * 0.5f - sideWallW * 0.5f);
    drawCube(0.02f, 0.03f, sideWallW);
    popMatrix();

    // ========== CORNICE (top trim band) ==========
    setColor(trimColor);
    pushMatrix();
    translate(0, halfH - 0.015f, 0);
    drawCube(buildW + 0.03f, 0.03f, buildD + 0.03f);
    popMatrix();

    // ========== ROOF ==========
    if (roofStyle == 0)
    {
        // Flat roof with slight raised edge
        setColor(roofColor);
        pushMatrix();
        translate(0, halfH + 0.01f, 0);
        drawCube(buildW + 0.01f, 0.02f, buildD + 0.01f);
        popMatrix();

        // Rooftop AC unit / water tank (detail)
        if (detailSeed > 2)
        {
            setColor(0.50f, 0.50f, 0.52f);
            pushMatrix();
            translate(0.1f, halfH + 0.06f, 0.15f);
            drawCube(0.12f, 0.08f, 0.12f);
            popMatrix();
        }
    }
    else if (roofStyle == 1)
    {
        // Peaked/gable roof
        setColor(roofColor * 0.9f);
        float peakH = 0.15f;
        // Ridge beam
        pushMatrix();
        translate(0, halfH + peakH * 0.5f, 0);
        drawCube(0.06f, peakH, buildD * 0.9f);
        popMatrix();
        // Left slope
        pushMatrix();
        translate(-buildW * 0.25f, halfH + peakH * 0.25f, 0);
        drawCube(buildW * 0.5f, peakH * 0.5f, buildD * 0.95f);
        popMatrix();
        // Right slope
        pushMatrix();
        translate(buildW * 0.25f, halfH + peakH * 0.25f, 0);
        drawCube(buildW * 0.5f, peakH * 0.5f, buildD * 0.95f);
        popMatrix();
    }
    else
    {
        // Parapet roof (raised wall around flat top)
        setColor(trimColor);
        float pw = 0.02f;
        pushMatrix();
        translate(0, halfH + 0.03f, buildD * 0.5f);
        drawCube(buildW + 0.02f, 0.06f, pw);
        popMatrix();
        pushMatrix();
        translate(0, halfH + 0.03f, -buildD * 0.5f);
        drawCube(buildW + 0.02f, 0.06f, pw);
        popMatrix();
        pushMatrix();
        translate(buildW * 0.5f, halfH + 0.03f, 0);
        drawCube(pw, 0.06f, buildD);
        popMatrix();
        pushMatrix();
        translate(-buildW * 0.5f, halfH + 0.03f, 0);
        drawCube(pw, 0.06f, buildD);
        popMatrix();
    }

    float frontX = -(buildW * 0.5f + 0.005f);

    // ========== FRONT FACE: WINDOWS ==========
    int numFloors = (int)(height / 0.2f);
    if (numFloors < 1)
        numFloors = 1;
    if (numFloors > 6)
        numFloors = 6;
    int winCols = (style == 2) ? 2 : 3;
    float winSpacing = buildD * 0.8f / (winCols + 1);

    for (int floor = 1; floor < numFloors; floor++)
    {
        float wy = -halfH + 0.15f + floor * 0.18f;
        if (wy > halfH - 0.08f)
            break;

        for (int col = 0; col < winCols; col++)
        {
            float wz = -buildD * 0.4f + winSpacing * (col + 1);

            // Window frame
            setColor(windowFrame);
            pushMatrix();
            translate(frontX, wy, wz);
            drawCube(0.005f, 0.12f, 0.09f);
            popMatrix();

            // Glass pane — lit/unlit pattern at night
            if (isNightTime)
            {
                bool isLit = ((floor * 7 + col * 13 + hash) % 10) < 6;
                setColor(isLit ? windowLit : windowDark);
            }
            else
            {
                setColor(windowDay);
            }
            pushMatrix();
            translate(frontX - 0.001f, wy, wz);
            drawCube(0.004f, 0.10f, 0.07f);
            popMatrix();
        }
    }

    // ========== SIDE FACES: WINDOWS ==========
    int sideWinCols = (int)(buildW / 0.2f);
    if (sideWinCols < 1)
        sideWinCols = 1;
    float sideSpacing = buildW * 0.8f / (sideWinCols + 1);

    for (int side = -1; side <= 1; side += 2)
    {
        float sideZ = side * (buildD * 0.5f + 0.005f);

        for (int floor = 0; floor < numFloors; floor++)
        {
            float wy = -halfH + 0.15f + floor * 0.18f;
            if (wy > halfH - 0.08f)
                break;

            for (int col = 0; col < sideWinCols; col++)
            {
                float wx = -buildW * 0.4f + sideSpacing * (col + 1);

                setColor(windowFrame);
                pushMatrix();
                translate(wx, wy, sideZ);
                drawCube(0.08f, 0.10f, 0.005f);
                popMatrix();

                // Glass — lit/unlit at night
                if (isNightTime)
                {
                    bool isLit = ((floor * 11 + col * 17 + side * 3 + hash) % 10) < 6;
                    setColor(isLit ? windowLit : windowDark);
                }
                else
                {
                    setColor(windowDay);
                }
                pushMatrix();
                translate(wx, wy, sideZ - side * 0.001f);
                drawCube(0.06f, 0.08f, 0.004f);
                popMatrix();
            }
        }
    }

    // ========== ARCHITECTURAL DETAILS ==========

    // Balconies (apartments only, random floors)
    if (style == 1 && numFloors >= 3)
    {
        for (int floor = 2; floor < numFloors; floor += 2)
        {
            float by = -halfH + 0.08f + floor * 0.18f;
            if (by > halfH - 0.1f)
                break;

            setColor(0.60f, 0.58f, 0.55f);
            // Balcony slab
            pushMatrix();
            translate(frontX - 0.08f, by, 0);
            drawCube(0.12f, 0.015f, 0.30f);
            popMatrix();
            // Railing
            setColor(0.35f, 0.35f, 0.38f);
            pushMatrix();
            translate(frontX - 0.13f, by + 0.04f, 0);
            drawCube(0.01f, 0.06f, 0.30f);
            popMatrix();
        }
    }

    // Pilasters / vertical trim strips (office style)
    if (style == 0)
    {
        setColor(trimColor * 1.1f);
        for (int side = -1; side <= 1; side += 2)
        {
            pushMatrix();
            translate(frontX, 0, side * buildD * 0.45f);
            drawCube(0.015f, height * 0.9f, 0.03f);
            popMatrix();
        }
    }

    // Chimney (house style)
    if (style == 2)
    {
        setColor(0.55f, 0.35f, 0.25f);
        pushMatrix();
        translate(buildW * 0.25f, halfH + 0.12f, buildD * 0.3f);
        drawCube(0.06f, 0.18f, 0.06f);
        popMatrix();
        // Chimney cap
        setColor(0.40f, 0.40f, 0.42f);
        pushMatrix();
        translate(buildW * 0.25f, halfH + 0.22f, buildD * 0.3f);
        drawCube(0.08f, 0.015f, 0.08f);
        popMatrix();
    }

    popMatrix();

    // ========== GATE (animated roller door) ==========
    // Gate is on the front face (frontX side in building-local coords),
    // centered vertically. We draw it in world space (after the building popMatrix).
    {
        // Gate dimensions — slightly narrower than building front face (which spans buildD)
        float gateW = buildD * 0.55f; // width of opening
        float gateH = 0.22f;          // full closed height
        int numSlats = 5;
        float slatH = gateH / numSlats;

        // The gate hinge is at world ground level on the front face of the building.
        // In building-local space, front face is at -buildW*0.5, door base at -halfH.
        // We need to get that in world space. Use the object's own transform.
        // Since Garage::draw() runs inside the object's pushMatrix context,
        // we reproduce the same transform here.

        // Colors
        Vec3 gateColor(0.35f, 0.34f, 0.32f);     // dark steel
        Vec3 gateHighlight(0.50f, 0.49f, 0.46f); // lighter slat edge
        Vec3 gateFrame(0.25f, 0.24f, 0.22f);     // frame

        pushMatrix();
        translate(0, halfH + CURB_H, 0);
        rotateY(angle + 180.0f);

        // --- Gate frame / surround ---
        float frontFace = -(buildW * 0.5f + 0.006f);
        float frameT = 0.012f;
        setColor(gateFrame);
        // Top bar of frame
        pushMatrix();
        translate(frontFace - frameT * 0.5f, -halfH + gateH + frameT * 0.5f, 0);
        drawCube(frameT, frameT, gateW + frameT * 2);
        popMatrix();
        // Left post
        pushMatrix();
        translate(frontFace - frameT * 0.5f, -halfH + gateH * 0.5f, -(gateW * 0.5f + frameT * 0.5f));
        drawCube(frameT, gateH, frameT);
        popMatrix();
        // Right post
        pushMatrix();
        translate(frontFace - frameT * 0.5f, -halfH + gateH * 0.5f, gateW * 0.5f + frameT * 0.5f);
        drawCube(frameT, gateH, frameT);
        popMatrix();

        // Center post
        pushMatrix();
        translate(frontFace - frameT * 0.5f, -halfH + gateH * 0.5f, 0);
        drawCube(frameT, gateH, frameT * 0.5f);
        popMatrix();

        float panelW = gateW * 0.5f - 0.01f;

        // --- IN Gate panel (Left side, -Z) ---
        pushMatrix();
        translate(frontFace - 0.002f, -halfH, 0);
        rotateZ(-gateAngleIn);
        for (int s = 0; s < numSlats; s++)
        {
            float sy = slatH * s + slatH * 0.5f;
            if (s % 2 == 0) setColor(gateColor); else setColor(gateHighlight);
            pushMatrix();
            translate(0, sy, -gateW * 0.25f);
            drawCube(0.008f, slatH - 0.003f, panelW);
            popMatrix();
        }
        setColor(gateFrame);
        for (int s = 1; s < numSlats; s++)
        {
            float sy = slatH * s;
            pushMatrix();
            translate(0, sy, -gateW * 0.25f);
            drawCube(0.009f, 0.003f, panelW + 0.005f);
            popMatrix();
        }
        setColor(gateFrame);
        pushMatrix();
        translate(0, gateH - 0.01f, -gateW * 0.25f);
        drawCube(0.009f, 0.012f, panelW * 0.8f);
        popMatrix();
        popMatrix(); // end IN gate panel

        // --- OUT Gate panel (Right side, +Z) ---
        pushMatrix();
        translate(frontFace - 0.002f, -halfH, 0);
        rotateZ(-gateAngleOut);
        for (int s = 0; s < numSlats; s++)
        {
            float sy = slatH * s + slatH * 0.5f;
            if (s % 2 == 0) setColor(gateColor); else setColor(gateHighlight);
            pushMatrix();
            translate(0, sy, gateW * 0.25f);
            drawCube(0.008f, slatH - 0.003f, panelW);
            popMatrix();
        }
        setColor(gateFrame);
        for (int s = 1; s < numSlats; s++)
        {
            float sy = slatH * s;
            pushMatrix();
            translate(0, sy, gateW * 0.25f);
            drawCube(0.009f, 0.003f, panelW + 0.005f);
            popMatrix();
        }
        setColor(gateFrame);
        pushMatrix();
        translate(0, gateH - 0.01f, gateW * 0.25f);
        drawCube(0.009f, 0.012f, panelW * 0.8f);
        popMatrix();
        popMatrix(); // end OUT gate panel

        popMatrix(); // end building transform
    }

    translate(-pos);
    Driveable::draw();
}

void Garage::update(const float delta)
{
    // ===== Traffic density modulation by time of day =====
    static float spawnMults[7] = {0.15f, 0.5f, 2.0f, 1.0f, 1.2f, 2.0f, 0.6f};
    static float maxMults[7] = {0.20f, 0.4f, 1.5f, 1.0f, 1.1f, 1.5f, 0.5f};

    int phase = Simulator::getInstance().getDayPhase();
    float spawnMult = spawnMults[phase];
    float effectiveFrec = frecSpot / spawnMult;
    int effectiveMax = (int)(maxVehicles * maxMults[phase]);
    if (effectiveMax < 2)
        effectiveMax = 2;

    // Local anti-jam dampening: if this garage lane is saturated,
    // progressively slow spawning to avoid permanent lock-ups.
    int laneQueued = (int)vehiclesBeg.size();
    if (laneQueued > effectiveMax * 0.50f)
        effectiveFrec *= 1.25f;
    if (laneQueued > effectiveMax * 0.70f)
        effectiveFrec *= 1.60f;
    if (laneQueued > effectiveMax * 0.85f)
        effectiveFrec *= 2.20f;

    // If delete queue is growing, ease inflow as well.
    if ((int)vehiclesEnd.size() > 6)
        effectiveFrec *= 1.35f;

    if (vehiclesBeg.size() == 0 || (vehiclesBeg.size() > 0 && vehiclesBeg.back()->xPos > 1))
        curTimeSpot += delta;

    if (curTimeSpot > effectiveFrec && spottedVehicles < effectiveMax && laneQueued < effectiveMax)
    {
        isReadyToSpot = true;
    }

    if (vehiclesEnd.size() > 0)
        curTimeDelete += delta;

    if (curTimeDelete > frecDelete)
    {
        if (vehiclesEnd.size() > 0)
        {
            isReadyToDelete = true;
        }
    }

    // ===== OUT Gate animation state machine =====
    switch (gateStateOut)
    {
    case GATE_OPENING:
        gateAngleOut += GATE_SPEED * delta;
        if (gateAngleOut >= 90.0f)
        {
            gateAngleOut = 90.0f;
            gateStateOut = GATE_OPEN;
            gateTimerOut = 0.0f;
        }
        break;

    case GATE_OPEN:
        gateTimerOut += delta;
        if (gateTimerOut > GATE_HOLD)
        {
            bool vehicleNearGate = (vehiclesBeg.size() > 0 && vehiclesBeg.back()->xPos < 0.3f);
            if (!vehicleNearGate)
                gateStateOut = GATE_CLOSING;
        }
        break;

    case GATE_CLOSING:
        gateAngleOut -= GATE_SPEED * delta;
        if (gateAngleOut <= 0.0f)
        {
            gateAngleOut = 0.0f;
            gateStateOut = GATE_CLOSED;
        }
        break;

    case GATE_CLOSED:
    default:
        break;
    }

    // ===== IN Gate animation state machine =====
    if (vehiclesEnd.size() > 0 && gateStateIn == GATE_CLOSED) {
        gateStateIn = GATE_OPENING;
        gateTimerIn = 0.0f;
    }

    switch (gateStateIn)
    {
    case GATE_OPENING:
        gateAngleIn += GATE_SPEED * delta;
        if (gateAngleIn >= 90.0f)
        {
            gateAngleIn = 90.0f;
            gateStateIn = GATE_OPEN;
            gateTimerIn = 0.0f;
        }
        break;

    case GATE_OPEN:
        gateTimerIn += delta;
        if (gateTimerIn > GATE_HOLD)
        {
            if (vehiclesEnd.size() == 0)
                gateStateIn = GATE_CLOSING;
        }
        break;

    case GATE_CLOSING:
        gateAngleIn -= GATE_SPEED * delta;
        if (gateAngleIn <= 0.0f)
        {
            gateAngleIn = 0.0f;
            gateStateIn = GATE_CLOSED;
        }
        if (vehiclesEnd.size() > 0) {
            gateStateIn = GATE_OPENING;
        }
        break;

    case GATE_CLOSED:
    default:
        break;
    }
}

string Garage::itos(const int x)
{
    ostringstream ss;
    ss << x;
    return ss.str();
}

Vehicle *Garage::spotVeh()
{
    curTimeSpot = 0;
    isReadyToSpot = false;

    // Open OUT gate for outgoing vehicle
    gateStateOut = GATE_OPENING;
    gateTimerOut = 0.0f;

    Vehicle *temp = createVehicle();

    temp->curRoad = this;
    temp->direction = true;
    temp->isFirstVeh = vehiclesBeg.size() == 0;

    vehiclesBeg.push(temp);

    spottedVehicles++;
    vehiclesCounter++;

    return temp;
}

Vehicle *Garage::deleteVeh()
{
    curTimeDelete = 0;
    isReadyToDelete = false;

    if (vehiclesEnd.size() > 0)
    {
        Vehicle *temp = vehiclesEnd.front();
        vehiclesEnd.pop();

        if (temp->backVeh != nullptr)
        {
            temp->backVeh->isFirstVeh = true;
            temp->backVeh->frontVeh = nullptr;
        }
        delete temp;

        spottedVehicles--;

        return temp;
    }

    return nullptr;
}

bool Garage::checkReadyToSpot() const
{
    return isReadyToSpot;
}

bool Garage::checkReadyToDelete() const
{
    return isReadyToDelete;
}

GarageCar::GarageCar(Vec3 p, Cross *c) : Garage(p, c)
{
}

Vehicle *GarageCar::createVehicle()
{
    Vehicle *temp = new Car(this);
    temp->id = "CAR_" + id + "_" + itos(Garage::vehiclesCounter);
    return temp;
}

GarageBus::GarageBus(Vec3 p, Cross *c) : Garage(p, c)
{
}

Vehicle *GarageBus::createVehicle()
{
    Vehicle *temp = new Bus(this);
    temp->id = "BUS_" + id + "_" + itos(Garage::vehiclesCounter);
    return temp;
}

GarageBike::GarageBike(Vec3 p, Cross *c) : Garage(p, c)
{
}

Vehicle *GarageBike::createVehicle()
{
    Vehicle *temp = new Bike(this);
    temp->id = "BIKE_" + id + "_" + itos(Garage::vehiclesCounter);
    return temp;
}
