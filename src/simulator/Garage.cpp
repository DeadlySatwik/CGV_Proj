///   File: Garage.cpp


#include "Garage.h"
#include "Simulator.h"
using namespace std;

int Garage::vehiclesCounter = 0;

Garage::Garage(Vec3 p, Cross *c) : Driveable(p, c)
{
    curTimeSpot = 0;
    frecSpot = 4;

    curTimeDelete = 0;
    frecDelete = 6;

    isReadyToSpot = false;
    isReadyToDelete = false;

    spottedVehicles = 0;
    maxVehicles = 30;
}

void Garage::draw()
{
    // ========== Hash from garage ID for procedural variety ==========
    unsigned int hash = 5381;
    for (size_t i = 0; i < id.size(); i++)
        hash = ((hash << 5) + hash) + (unsigned char)id[i];

    int style       = hash % 3;            // 0=office, 1=apartment, 2=house
    int colorIdx    = (hash >> 3) % 8;
    int roofStyle   = (hash >> 6) % 3;     // 0=flat, 1=peaked, 2=parapet
    int doorStyle   = (hash >> 9) % 3;     // 0=single, 1=double, 2=shopfront
    int detailSeed  = (hash >> 12) % 7;

    // Height varies by style
    float height;
    if (style == 0)      height = 0.8f + (hash % 5) * 0.12f;     // office: 0.8-1.28
    else if (style == 1) height = 0.6f + (hash % 4) * 0.10f;     // apartment: 0.6-0.9
    else                 height = 0.35f + (hash % 3) * 0.08f;    // house: 0.35-0.51

    float halfH = height * 0.5f;
    float buildW = (style == 2) ? 0.55f : 0.7f;   // houses are narrower
    float buildD = 1.0f;

    // ========== Color palette (8 muted architectural tones) ==========
    Vec3 wallColors[8] = {
        Vec3(0.62f, 0.60f, 0.58f),   // warm grey
        Vec3(0.72f, 0.65f, 0.52f),   // tan/sandstone
        Vec3(0.50f, 0.38f, 0.28f),   // brown
        Vec3(0.82f, 0.78f, 0.70f),   // cream
        Vec3(0.65f, 0.35f, 0.25f),   // terracotta
        Vec3(0.48f, 0.52f, 0.58f),   // blue-grey
        Vec3(0.45f, 0.48f, 0.35f),   // olive
        Vec3(0.40f, 0.42f, 0.45f)    // slate
    };
    Vec3 wallColor = wallColors[colorIdx];

    // Derive accent colors from wall color
    Vec3 trimColor = wallColor * 0.7f;
    Vec3 roofColor = wallColor * 0.55f;
    Vec3 windowColor(0.18f, 0.35f, 0.42f);    // dark teal glass
    Vec3 windowFrame(0.75f, 0.73f, 0.68f);    // light frame
    Vec3 doorColor(0.30f, 0.18f, 0.08f);       // dark wood

    // Day phase for window glow
    int bldgPhase = Simulator::getInstance().getDayPhase();
    bool isNightTime = (bldgPhase == 0 || bldgPhase == 1 || bldgPhase == 5 || bldgPhase == 6);
    // Window colors: at night, some windows glow warm (occupied), others dark (empty)
    Vec3 windowLit(0.85f, 0.75f, 0.45f);       // warm occupied room
    Vec3 windowDark(0.08f, 0.10f, 0.14f);      // dark empty room
    Vec3 windowDay(0.18f, 0.35f, 0.42f);       // daytime teal glass

    pushMatrix();
    translate(0, halfH + CURB_H, 0);
    rotateY(direction.angleXZ());

    // ========== MAIN BUILDING BODY ==========
    setColor(wallColor);
    drawCube(buildW, height, buildD);

    // ========== FOUNDATION / BASE STRIP ==========
    setColor(trimColor * 0.8f);
    pushMatrix();
    translate(0, -halfH + 0.015f, 0);
    drawCube(buildW + 0.02f, 0.03f, buildD + 0.02f);
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
        pushMatrix(); translate(0, halfH + 0.03f, buildD * 0.5f); drawCube(buildW + 0.02f, 0.06f, pw); popMatrix();
        pushMatrix(); translate(0, halfH + 0.03f, -buildD * 0.5f); drawCube(buildW + 0.02f, 0.06f, pw); popMatrix();
        pushMatrix(); translate(buildW * 0.5f, halfH + 0.03f, 0); drawCube(pw, 0.06f, buildD); popMatrix();
        pushMatrix(); translate(-buildW * 0.5f, halfH + 0.03f, 0); drawCube(pw, 0.06f, buildD); popMatrix();
    }

    // ========== FRONT FACE: DOOR ==========
    float frontX = -(buildW * 0.5f + 0.005f);

    if (doorStyle == 0)
    {
        // Single door
        setColor(doorColor);
        pushMatrix();
        translate(frontX, -halfH + 0.12f, 0);
        drawCube(0.01f, 0.24f, 0.14f);
        popMatrix();
        // Door frame
        setColor(windowFrame);
        pushMatrix();
        translate(frontX, -halfH + 0.125f, 0);
        drawCube(0.005f, 0.25f, 0.16f);
        popMatrix();
    }
    else if (doorStyle == 1)
    {
        // Double door
        setColor(doorColor);
        pushMatrix();
        translate(frontX, -halfH + 0.12f, -0.05f);
        drawCube(0.01f, 0.24f, 0.09f);
        popMatrix();
        pushMatrix();
        translate(frontX, -halfH + 0.12f, 0.05f);
        drawCube(0.01f, 0.24f, 0.09f);
        popMatrix();
        // Divider strip
        setColor(windowFrame);
        pushMatrix();
        translate(frontX, -halfH + 0.12f, 0);
        drawCube(0.005f, 0.24f, 0.01f);
        popMatrix();
    }
    else
    {
        // Shopfront — wide glass + awning
        setColor(0.15f, 0.40f, 0.45f);
        pushMatrix();
        translate(frontX, -halfH + 0.10f, 0);
        drawCube(0.008f, 0.20f, 0.50f);
        popMatrix();
        // Awning (colorful overhang)
        Vec3 awningColor((hash % 200) / 400.0f + 0.3f,
                         ((hash >> 4) % 200) / 400.0f + 0.2f,
                         ((hash >> 8) % 200) / 400.0f + 0.15f);
        setColor(awningColor);
        pushMatrix();
        translate(frontX - 0.06f, -halfH + 0.22f, 0);
        drawCube(0.10f, 0.02f, 0.55f);
        popMatrix();
    }

    // ========== FRONT FACE: WINDOWS ==========
    int numFloors = (int)(height / 0.2f);
    if (numFloors < 1) numFloors = 1;
    if (numFloors > 6) numFloors = 6;
    int winCols = (style == 2) ? 2 : 3;
    float winSpacing = buildD * 0.8f / (winCols + 1);

    for (int floor = 1; floor < numFloors; floor++)
    {
        float wy = -halfH + 0.15f + floor * 0.18f;
        if (wy > halfH - 0.08f) break;

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
    if (sideWinCols < 1) sideWinCols = 1;
    float sideSpacing = buildW * 0.8f / (sideWinCols + 1);

    for (int side = -1; side <= 1; side += 2)
    {
        float sideZ = side * (buildD * 0.5f + 0.005f);

        for (int floor = 0; floor < numFloors; floor++)
        {
            float wy = -halfH + 0.15f + floor * 0.18f;
            if (wy > halfH - 0.08f) break;

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
            if (by > halfH - 0.1f) break;

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

    translate(-pos);
    Driveable::draw();
}

void Garage::update(const float delta)
{
    // ===== Traffic density modulation by time of day (6G) =====
    static float spawnMults[7] = { 0.15f, 0.5f, 2.0f, 1.0f, 1.2f, 2.0f, 0.6f };
    static float maxMults[7]   = { 0.20f, 0.4f, 1.5f, 1.0f, 1.1f, 1.5f, 0.5f };

    int phase = Simulator::getInstance().getDayPhase();
    float spawnMult = spawnMults[phase];
    float effectiveFrec = frecSpot / spawnMult;
    int effectiveMax = (int)(maxVehicles * maxMults[phase]);
    if (effectiveMax < 2) effectiveMax = 2;

    if (vehiclesBeg.size() == 0 || (vehiclesBeg.size() > 0 && vehiclesBeg.back()->xPos > 1))
        curTimeSpot += delta;

    if (curTimeSpot > effectiveFrec && spottedVehicles < effectiveMax)
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
}

string Garage::itos(const int x)
{
    ostringstream ss;
    ss << x;
    return ss.str();
}

Vehicle* Garage::spotVeh()
{
    curTimeSpot = 0;
    isReadyToSpot = false;

    Vehicle *temp = createVehicle();

    temp->curRoad = this;
    temp->direction = true;
    temp->isFirstVeh = vehiclesBeg.size() == 0;

    vehiclesBeg.push(temp);

    spottedVehicles++;
    vehiclesCounter++;

    return temp;
}

Vehicle* Garage::deleteVeh()
{
    curTimeDelete = 0;
    isReadyToDelete = false;

    if (vehiclesEnd.size() > 0)
    {
        Vehicle *temp = vehiclesEnd.front();
        vehiclesEnd.pop();

        if(temp->backVeh != nullptr)
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
