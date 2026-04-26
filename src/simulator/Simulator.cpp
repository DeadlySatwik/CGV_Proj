///   File: Simulator.cpp

#include "Simulator.h"
#include "Environment.h"
using namespace std;

// ===== Day/Night static data =====
const char *Simulator::phaseNames[PHASE_COUNT] = {
    "Night", "Dawn", "Morning", "Noon", "Afternoon", "Evening", "Dusk"};

// Phase boundaries (hours): Night[0-5], Dawn[5-7], Morning[7-9], Noon[9-15], Afternoon[15-17], Evening[17-19], Dusk[19-21], then Night again[21-24]
float Simulator::phaseBoundaries[PHASE_COUNT + 1] = {
    0.0f, 5.0f, 7.0f, 9.0f, 15.0f, 17.0f, 19.0f, 21.0f};

// Sky colors per phase (interpolated between)
static Vec3 skyColors[8] = {
    Vec3(0.02f, 0.02f, 0.06f), // Night
    Vec3(0.65f, 0.40f, 0.30f), // Dawn — warm peach
    Vec3(0.50f, 0.70f, 0.90f), // Morning — clear blue
    Vec3(0.42f, 0.62f, 0.82f), // Noon — bright blue
    Vec3(0.45f, 0.65f, 0.85f), // Afternoon
    Vec3(0.75f, 0.42f, 0.22f), // Evening — golden
    Vec3(0.30f, 0.15f, 0.30f), // Dusk — purple
    Vec3(0.02f, 0.02f, 0.06f)  // Night again (wrap)
};

// Ambient light per phase
static Vec3 ambientColors[8] = {
    Vec3(0.08f, 0.08f, 0.12f), // Night
    Vec3(0.22f, 0.18f, 0.16f), // Dawn
    Vec3(0.30f, 0.30f, 0.32f), // Morning
    Vec3(0.35f, 0.35f, 0.38f), // Noon
    Vec3(0.32f, 0.32f, 0.35f), // Afternoon
    Vec3(0.25f, 0.20f, 0.16f), // Evening
    Vec3(0.12f, 0.10f, 0.15f), // Dusk
    Vec3(0.08f, 0.08f, 0.12f)  // Night again
};

// Diffuse (sun) color per phase
static Vec3 diffuseColors[8] = {
    Vec3(0.05f, 0.05f, 0.08f), // Night — moonlight
    Vec3(0.85f, 0.55f, 0.35f), // Dawn — warm orange
    Vec3(1.00f, 0.95f, 0.85f), // Morning — warm white
    Vec3(1.00f, 1.00f, 0.95f), // Noon — pure white
    Vec3(1.00f, 0.95f, 0.88f), // Afternoon
    Vec3(0.90f, 0.60f, 0.30f), // Evening — golden
    Vec3(0.35f, 0.18f, 0.28f), // Dusk — purple
    Vec3(0.05f, 0.05f, 0.08f)  // Night again
};

// Sun direction per phase (w=0 for directional)
static Vec3 sunDirs[8] = {
    Vec3(0.0f, 1.0f, 0.0f),   // Night — overhead dim
    Vec3(0.8f, 0.3f, 0.2f),   // Dawn — low east
    Vec3(0.5f, 0.8f, 0.3f),   // Morning — rising
    Vec3(0.1f, 1.0f, 0.1f),   // Noon — overhead
    Vec3(-0.3f, 0.9f, -0.2f), // Afternoon
    Vec3(-0.8f, 0.3f, -0.3f), // Evening — low west
    Vec3(-0.9f, 0.1f, -0.2f), // Dusk — horizon
    Vec3(0.0f, 1.0f, 0.0f)    // Night again
};

// Ground color per phase (grass)
static Vec3 groundColors[8] = {
    Vec3(0.03f, 0.04f, 0.02f), // Night — very dark
    Vec3(0.10f, 0.15f, 0.06f), // Dawn
    Vec3(0.18f, 0.30f, 0.12f), // Morning — bright grass
    Vec3(0.22f, 0.35f, 0.15f), // Noon — brightest
    Vec3(0.20f, 0.32f, 0.13f), // Afternoon
    Vec3(0.15f, 0.22f, 0.08f), // Evening — warm tint
    Vec3(0.06f, 0.08f, 0.04f), // Dusk — dimming
    Vec3(0.03f, 0.04f, 0.02f)  // Night again
};

// Fog distances per phase [start, end]
static float fogDists[8][2] = {
    {100.0f, 350.0f}, // Night — close fog
    {150.0f, 500.0f}, // Dawn
    {250.0f, 700.0f}, // Morning
    {300.0f, 800.0f}, // Noon — far fog
    {280.0f, 750.0f}, // Afternoon
    {180.0f, 550.0f}, // Evening
    {120.0f, 400.0f}, // Dusk
    {100.0f, 350.0f}  // Night again
};

// ===== Helper: linear interpolation =====
static Vec3 lerpVec3(const Vec3 &a, const Vec3 &b, float t)
{
    return Vec3(a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t);
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// ============================================================

Simulator &Simulator::getInstance()
{
    static Simulator instanceSimulator;

    return instanceSimulator;
}

void Simulator::run()
{
    objects.reserve(maxNumberOfObjects);

    cout << "Simulator is running" << endl;
    EngineCore::run();
}

void Simulator::setupProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;

    if (!projectionOrtho)
    {
        // Perspective: near=0.5, far=5000 (scaled by 10x world)
        glFrustum(-0.1 * aspect, 0.1 * aspect, -0.1, 0.1, 0.5, 5000.0);
    }
    else
    {
        // Orthographic: zoom-based
        float w = orthoZoom * aspect;
        float h = orthoZoom;
        glOrtho(-w, w, -h, h, -5000.0, 5000.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void Simulator::updateCameraVectors()
{
    float yawRad = cameraRot.x * M_PI / 180.0f;
    float pitchRad = cameraRot.y * M_PI / 180.0f;
    float cosPitch = cosf(pitchRad);

    // Front vector from yaw/pitch
    cameraFront.x = sinf(yawRad) * cosPitch;
    cameraFront.y = -sinf(pitchRad);
    cameraFront.z = -cosf(yawRad) * cosPitch;
    cameraFront.normalize();

    // Right = front x worldUp
    Vec3 worldUp(0, 1, 0);
    cameraRight = Vec3::cross(cameraFront, worldUp);
    cameraRight.normalize();

    // Up = right x front
    cameraUp = Vec3::cross(cameraRight, cameraFront);
    cameraUp.normalize();
}

// ===== Day/Night Clock =====

int Simulator::getDayPhase() const { return dayPhase; }
float Simulator::getWorldTime() const { return worldTime; }

int Simulator::computeDayPhase() const
{
    float t = worldTime;
    if (t >= 21.0f)
        return PHASE_NIGHT; // 21-24 wraps to night
    for (int i = PHASE_COUNT - 1; i >= 0; i--)
    {
        if (t >= phaseBoundaries[i])
            return i;
    }
    return PHASE_NIGHT;
}

float Simulator::getDayPhaseProgress() const
{
    int phase = dayPhase;
    float start = phaseBoundaries[phase];
    float end;
    if (phase == PHASE_DUSK)
        end = 21.0f;
    else if (phase == PHASE_NIGHT)
    {
        // Night spans 21-24 and 0-5
        if (worldTime >= 21.0f)
            return (worldTime - 21.0f) / 8.0f; // 21-24 → 0.0-0.375
        else
            return (worldTime + 3.0f) / 8.0f; // 0-5 → 0.375-1.0
    }
    else
        end = phaseBoundaries[phase + 1];

    float duration = end - start;
    if (duration <= 0)
        return 0;
    return (worldTime - start) / duration;
}

void Simulator::updateWorldTime(float delta)
{
    if (!timeFlowing)
        return;

    // timeSpeed = simulated minutes per real second
    float simMinutes = timeSpeed * delta;
    worldTime += simMinutes / 60.0f; // convert to hours
    if (worldTime >= 24.0f)
        worldTime -= 24.0f;

    int newPhase = computeDayPhase();
    if (newPhase != dayPhase)
    {
        dayPhase = newPhase;
        int h = (int)worldTime;
        int m = (int)((worldTime - h) * 60);
        cout << "[" << (h < 10 ? "0" : "") << h << ":" << (m < 10 ? "0" : "") << m << "] "
             << phaseNames[dayPhase] << endl;
    }
}

// ===== Dynamic Sky & Lighting =====

void Simulator::updateSkyAndLighting()
{
    // Determine interpolation: current phase → next phase
    int curPhaseIdx = dayPhase;
    int nextPhaseIdx = (curPhaseIdx + 1) % PHASE_COUNT;

    // For night (21-24 and 0-5), map to indices 7 (wrap) appropriately
    int curColorIdx = curPhaseIdx;
    int nextColorIdx;
    if (curPhaseIdx == PHASE_NIGHT)
        nextColorIdx = 1; // night → dawn
    else
        nextColorIdx = curColorIdx + 1;

    float t = getDayPhaseProgress();

    // Interpolate sky color
    Vec3 sky = lerpVec3(skyColors[curColorIdx], skyColors[nextColorIdx], t);
    glClearColor(sky.x, sky.y, sky.z, 1.0f);

    // Interpolate lighting
    Vec3 amb = lerpVec3(ambientColors[curColorIdx], ambientColors[nextColorIdx], t);
    Vec3 diff = lerpVec3(diffuseColors[curColorIdx], diffuseColors[nextColorIdx], t);
    Vec3 sunD = lerpVec3(sunDirs[curColorIdx], sunDirs[nextColorIdx], t);

    GLfloat lambient[] = {amb.x, amb.y, amb.z, 1.0f};
    GLfloat ldiffuse[] = {diff.x, diff.y, diff.z, 1.0f};
    GLfloat lposition[] = {sunD.x, sunD.y, sunD.z, 0.0f}; // w=0 directional

    glLightfv(GL_LIGHT0, GL_AMBIENT, lambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, ldiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, lposition);

    // Fog
    GLfloat fogColor[] = {sky.x, sky.y, sky.z, 1.0f};
    float fogStart = lerpf(fogDists[curColorIdx][0], fogDists[nextColorIdx][0], t);
    float fogEnd = lerpf(fogDists[curColorIdx][1], fogDists[nextColorIdx][1], t);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, fogStart);
    glFogf(GL_FOG_END, fogEnd);
}

// ===== Redraw =====

void Simulator::redraw()
{
    // Update projection each frame (handles toggle)
    setupProjection();

    Vec3 eyePos, eyeFront, eyeUp, eyeRight;

    if (thirdPersonMode && activePlayerVeh)
    {
        // 3rd person chase camera
        eyePos = activePlayerVeh->getCameraPos();

        // Compute front from eye to target
        Vec3 target = activePlayerVeh->getCameraTarget();
        eyeFront = target - eyePos;
        eyeFront.normalize();

        // Compute right and up
        Vec3 worldUp(0, 1, 0);
        eyeRight = Vec3::cross(eyeFront, worldUp);
        eyeRight.normalize();
        eyeUp = Vec3::cross(eyeRight, eyeFront);
        eyeUp.normalize();
    }
    else
    {
        // Free-fly camera (existing)
        updateCameraVectors();
        eyePos = cameraPos;
        eyeFront = cameraFront;
        eyeRight = cameraRight;
        eyeUp = cameraUp;
    }

    // Build lookAt view matrix manually (column-major for OpenGL)
    Vec3 f = eyeFront;
    Vec3 r = eyeRight;
    Vec3 u = eyeUp;

    // View matrix = rotation * translation
    // OpenGL column-major: m[col*4 + row]
    float view[16];
    view[0] = r.x;
    view[4] = r.y;
    view[8] = r.z;
    view[12] = -(r.x * eyePos.x + r.y * eyePos.y + r.z * eyePos.z);
    view[1] = u.x;
    view[5] = u.y;
    view[9] = u.z;
    view[13] = -(u.x * eyePos.x + u.y * eyePos.y + u.z * eyePos.z);
    view[2] = -f.x;
    view[6] = -f.y;
    view[10] = -f.z;
    view[14] = (f.x * eyePos.x + f.y * eyePos.y + f.z * eyePos.z);
    view[3] = 0;
    view[7] = 0;
    view[11] = 0;
    view[15] = 1;

    glLoadIdentity();

    // Apply the lookAt view matrix
    glMultMatrixf(view);

    // World scale: 1 world unit = 10 GL units
    scale(10, 10, 10);

    // Z-axis flip for coordinate convention
    scale(1, 1, -1);

    // ===== Dynamic sky, lighting, fog =====
    updateSkyAndLighting();

    // ---- Ground plane (color interpolated by time) ----
    int curCI = dayPhase;
    int nextCI = (curCI + 1) % PHASE_COUNT;
    if (curCI == PHASE_NIGHT)
        nextCI = 1; // night→dawn color
    float gT = getDayPhaseProgress();
    Vec3 groundCol = lerpVec3(groundColors[curCI],
                              curCI == PHASE_NIGHT ? groundColors[0] : groundColors[nextCI], gT);

    setColor(groundCol);
    float gs = 80.0f;
    float gy = -(Road::ROAD_DEPTH + 0.005f);
    beginDraw(QUADS);
    setNormal(0, 1, 0);
    drawVertex(Vec3(-gs, gy, -gs));
    drawVertex(Vec3(gs, gy, -gs));
    drawVertex(Vec3(gs, gy, gs));
    drawVertex(Vec3(-gs, gy, gs));
    endDraw();

    pushMatrix();

    for (const auto &object : objects)
    {
        object->drawObject();
    }

    // Draw the active player vehicle
    if (activePlayerVeh)
    {
        activePlayerVeh->drawObject();
    }

    popMatrix();
}

Simulator::Simulator() : maxNumberOfObjects(0), CAMERA_VELOCITY(3)
{
    cout << "Initializing simulator...  ";

    init();

    cameraPos = Vec3(-5.5, 2.5, -7.84);
    cameraRot = Vec3(-215, 13.2, 0);

    cameraDirection = 0;
    moveSpeed = 10.0f;
    mouseSensitivity = 0.1f;
    projectionOrtho = false;
    orthoZoom = 30.0f;

    // Player car initialization
    playerCar = new PlayerCar(Vec3(6.0f, 0.0f, 0.0f)); // middle of street HC5
    thirdPersonMode = false;
    playerInputMap = 0;
    globalMaxVehicles = 50;

    // Day/Night initialization — start at 10:00 (morning)
    worldTime = 10.0f;
    timeSpeed = 2.0f; // 2 sim-minutes per real-second → full day in 12 real minutes
    timeFlowing = true;
    dayPhase = computeDayPhase();

    updateCameraVectors();

    cout << "Success" << endl;
    cout << "[10:00] " << phaseNames[dayPhase] << endl;
}

void Simulator::keyHeld(char k)
{
    // In 3rd person mode, arrow keys / IJKL drive the player car
    if (thirdPersonMode)
    {
        // Arrow keys come as VK codes (Windows): Up=38, Down=40, Left=37, Right=39
        switch (k)
        {
        case 38: // VK_UP
        case 'i':
            playerInputMap |= PlayerCar::INPUT_ACCEL;
            break;
        case 40: // VK_DOWN
        case 'k':
            playerInputMap |= PlayerCar::INPUT_BRAKE;
            break;
        case 37: // VK_LEFT
        case 'j':
            playerInputMap |= PlayerCar::INPUT_STEER_LEFT;
            break;
        case 39: // VK_RIGHT
        case 'l':
            playerInputMap |= PlayerCar::INPUT_STEER_RIGHT;
            break;
        }
        return; // Don't move free camera while driving
    }

    if (k >= 'A' && k <= 'Z')
    {
        k += 32;
        cameraVelocity = moveSpeed * 4.0f; // Shift = 4x speed
    }
    else
    {
        cameraVelocity = moveSpeed;
    }

    switch (k)
    {
    case 'w':
        cameraDirection |= (1 << FORWARD);
        break;
    case 's':
        cameraDirection |= (1 << BACK);
        break;
    case 'a':
        cameraDirection |= (1 << LEFT);
        break;
    case 'd':
        cameraDirection |= (1 << RIGHT);
        break;
    case 'e':
        cameraDirection |= (1 << UP);
        break;
    case 'q':
        cameraDirection |= (1 << DOWN);
        break;
    case ' ':
        cameraDirection |= (1 << UP);
        break; // Space = up
    }
}

void Simulator::keyPressed(char k)
{
    if (k == 27)
    {
        cout << "Stopping simulator" << endl;
        cleanSimulation();
        breakMainLoop();
        return;
    }

    if (k >= 'A' && k <= 'Z')
        k += 32;

    switch (k)
    {
    case 'y':
        updatesPerFrame++;
        break;
    case 't':
        updatesPerFrame--;
        break;
    case 'h':
        timeScale += 0.1;
        break;
    case 'g':
        timeScale -= 0.1;
        break;
    case 'p':
        projectionOrtho = !projectionOrtho;
        cout << "Projection: " << (projectionOrtho ? "Orthographic" : "Perspective") << endl;
        break;
    case 'f':
        thirdPersonMode = !thirdPersonMode;
        cout << "Camera: " << (thirdPersonMode ? "3rd Person (Arrow keys to drive)" : "Free Fly (WASD to move)") << endl;
        break;

    // --- Day/Night controls ---
    case 'n':
    {
        // Jump to next day phase
        int next = (dayPhase + 1) % PHASE_COUNT;
        worldTime = phaseBoundaries[next];
        if (next == PHASE_NIGHT)
            worldTime = 21.0f;
        dayPhase = next;
        int h = (int)worldTime;
        int m = (int)((worldTime - h) * 60);
        cout << "[" << (h < 10 ? "0" : "") << h << ":" << (m < 10 ? "0" : "") << m << "] "
             << phaseNames[dayPhase] << " (jumped)" << endl;
        break;
    }
    case 'm':
        timeFlowing = !timeFlowing;
        cout << "Time flow: " << (timeFlowing ? "ON" : "PAUSED") << endl;
        break;
    case 'i':
    {
        printTelemetry();
        break;
    }
    case '1':
    case '2':
    case '3':
    {
        int newIdx = k - '1';
        if (newIdx != currentVehicleIdx) {
            PlayerCar *oldVeh = playerVehicles[currentVehicleIdx];
            currentVehicleIdx = newIdx;
            activePlayerVeh = playerVehicles[currentVehicleIdx];
            
            // Transfer state
            activePlayerVeh->pos = oldVeh->pos;
            activePlayerVeh->speed = oldVeh->speed;
            activePlayerVeh->heading = oldVeh->heading;
            activePlayerVeh->rot = oldVeh->rot;
            activePlayerVeh->setOldPosition();
            
            cout << "Switched to vehicle " << (newIdx == 0 ? "Car" : (newIdx == 1 ? "Bus" : "Bike")) << endl;
        }
        break;
    }
    }

    updatesPerFrame = max(MIN_UPDATES_PER_FRAME, min(updatesPerFrame, MAX_UPDATES_PER_FRAME));
    timeScale = max(MIN_TIME_SCALE, min(timeScale, MAX_TIME_SCALE));
}

void Simulator::cameraMove(const float delta)
{
    float speed = cameraVelocity * delta;

    // Vector-based FPS movement using derived camera vectors
    if ((cameraDirection >> FORWARD) & 1)
    {
        cameraPos = cameraPos + cameraFront * speed;
    }
    if ((cameraDirection >> BACK) & 1)
    {
        cameraPos = cameraPos - cameraFront * speed;
    }
    if ((cameraDirection >> LEFT) & 1)
    {
        cameraPos = cameraPos - cameraRight * speed;
    }
    if ((cameraDirection >> RIGHT) & 1)
    {
        cameraPos = cameraPos + cameraRight * speed;
    }
    if ((cameraDirection >> UP) & 1)
    {
        cameraPos.y += speed;
    }
    if ((cameraDirection >> DOWN) & 1)
    {
        cameraPos.y -= speed;
    }
    cameraDirection = 0;
}

void Simulator::mouseMove(const int dx, const int dy)
{
    cameraRot.x += dx * mouseSensitivity;
    cameraRot.y += dy * mouseSensitivity;

    // Clamp pitch to prevent gimbal lock
    if (cameraRot.y > 89.0f)
        cameraRot.y = 89.0f;
    else if (cameraRot.y < -89.0f)
        cameraRot.y = -89.0f;

    updateCameraVectors();
}

void Simulator::singleUpdate(const float delta)
{
    // Save position before move (for road constraint)
    activePlayerVeh->setOldPosition();

    // Update player vehicle input
    activePlayerVeh->handleInput(playerInputMap, delta);
    playerInputMap = 0;

    float sampledRoadHeight = 0.0f;
    bool onRoad = sampleRoadHeight(activePlayerVeh->getPos(), sampledRoadHeight);

    // Road/end constraint + collision constraint
    if (!onRoad || isPlayerBlocked(activePlayerVeh->getPos()))
    {
        activePlayerVeh->revertPosition();
    }
    else
    {
        Vec3 correctedPos = activePlayerVeh->getPos();
        correctedPos.y = sampledRoadHeight;
        activePlayerVeh->setPos(correctedPos);
    }

    if (!thirdPersonMode)
    {
        cameraMove(delta);
    }

    updateWorldTime(delta); // advance world clock
}

void Simulator::update(const float delta)
{
    int activeVehicles = getActiveVehicleCount();

    for (auto &spot : spots)
    {
        if (spot->checkReadyToSpot() && activeVehicles < globalMaxVehicles)
        {
            registerObject(spot->spotVeh());
            activeVehicles++;
        }
        if (spot->checkReadyToDelete())
        {
            destroyObject(spot->deleteVeh());
            activeVehicles--;
        }
    }

    for (auto &object : objects)
    {
        object->updateObject(delta);
    }
}

int Simulator::getActiveVehicleCount() const
{
    // objects includes roads, crosses, garages, environment + vehicles
    // maxNumberOfObjects = count of non-vehicle objects (roads, crosses, etc.) + garage maxVehicles
    // A simpler approach: count objects that exceed the static object count
    int staticObjects = 0;
    for (const auto &spot : spots)
    {
        staticObjects += spot->maxVehicles;
    }
    int baseObjects = maxNumberOfObjects - staticObjects;
    return (int)objects.size() - baseObjects;
}

void Simulator::registerObject(GameObject *go)
{
    objects.push_back(go);
}

void Simulator::destroyObject(GameObject *go)
{
    auto objectToRemove = find_if(objects.begin(), objects.end(), [&go](GameObject *item)
                                  { return item == go; });
    iter_swap(objectToRemove, objects.end() - 1);
    objects.pop_back();
}

void Simulator::cleanSimulation()
{
    maxNumberOfObjects = 0;

    while (objects.size() > 0)
        destroyObject(objects.back());
}

GameObject *Simulator::findObjectByName(const string objectName) const
{
    auto foundObject = find_if(objects.begin(), objects.end(), [&objectName](GameObject *item)
                               { return item->id.compare(objectName) == 0; });

    if (foundObject != objects.end())
        return *foundObject;
    return nullptr;
}

void Simulator::loadedNewObject(GameObject *newGameObject)
{
    objects.push_back(newGameObject);
    maxNumberOfObjects++;
}

void Simulator::loadedNewFactory(Garage *newFactory)
{
    spots.push_back(newFactory);
    maxNumberOfObjects += newFactory->maxVehicles;
}

bool Simulator::isOnRoad(Vec3 testPos) const
{
    float sampled = 0.0f;
    return sampleRoadHeight(testPos, sampled);
}

bool Simulator::sampleRoadHeight(const Vec3 &testPos, float &outHeight) const
{
    // Check if testPos is within any road segment or intersection.
    // Road half-width = 0.3 (visual), we use 0.4 for a little tolerance so they don't clip curbs.
    const float roadHW = 0.4f;
    // Intersection half-size = 0.3. We use 0.6 here to give a very generous bounding box
    // at intersections, allowing the player to make smooth, curved turns instead of
    // being forced into a square path by invisible walls.
    const float crossHalf = 0.6f;

    bool found = false;
    float bestDistance = 1e9f;
    float bestHeight = 0.0f;

    for (const auto &obj : objects)
    {
        // Check if it's a Driveable (Street or Garage)
        Driveable *road = dynamic_cast<Driveable *>(obj);
        if (road)
        {
            // Project testPos onto the road's line segment (begPos -> endPos)
            Vec3 roadDir = road->getDirection();
            Vec3 roadNorm = road->getNormal();

            // Get road endpoints (use the raw begPos/endPos via getJointPoint
            // but those are inset by 0.3; we need the full road extent)
            // Actually, begPos and endPos are the cross center positions.
            // The road surface goes from begPos to endPos.
            Vec3 begP = road->getJointPoint(true);  // begJoint
            Vec3 endP = road->getJointPoint(false); // endJoint

            // Use begPos/endPos for the full road — but those are private.
            // Use joints as approximation (they're inset by ~0.3 from cross center).
            // The actual road surface extends slightly beyond joints toward crosses,
            // so add some margin along the road direction.
            float roadLen = Vec3::dst(begP, endP);

            // Vector from begJoint to testPos
            Vec3 diff = testPos - begP;
            // Project along road direction (dot product via components)
            float along = diff.x * roadDir.x + diff.z * roadDir.z;
            // Project along road normal (perpendicular distance)
            float across = diff.x * roadNorm.x + diff.z * roadNorm.z;

            // Check if within road bounds
            // Along: from -0.3 (margin toward beg cross) to roadLen + 0.3
            // Across: within +/- roadHW
            if (along >= -0.2f && along <= roadLen + 0.2f &&
                fabs(across) <= roadHW + 0.06f)
            {
                float s = roadLen > 0.0001f ? along / roadLen : 0.0f;
                if (s < 0.0f)
                    s = 0.0f;
                if (s > 1.0f)
                    s = 1.0f;

                float y = road->getHeightAt(s);
                float dist = fabs(across);
                if (!found || dist < bestDistance)
                {
                    found = true;
                    bestDistance = dist;
                    bestHeight = y;
                }
            }
        }

        // Check if it's a Cross (intersection)
        Cross *cross = dynamic_cast<Cross *>(obj);
        if (cross && !road) // Crosses that aren't also Driveables
        {
            Vec3 crossPos = cross->getPos();
            float dx = fabs(testPos.x - crossPos.x);
            float dz = fabs(testPos.z - crossPos.z);
            if (dx <= crossHalf && dz <= crossHalf)
            {
                float dist = dx + dz;
                if (!found || dist < bestDistance)
                {
                    found = true;
                    bestDistance = dist;
                    bestHeight = cross->getIntersectionHeight();
                }
            }
        }
    }

    if (found)
    {
        outHeight = bestHeight;
        return true;
    }

    outHeight = 0.0f;
    return false;
}

bool Simulator::isPlayerBlocked(const Vec3 &testPos) const
{
    const float playerRadius = 0.13f;

    for (const auto &obj : objects)
    {
        if (obj == playerCar)
            continue;

        Vec3 op = obj->getPos();
        float dx = testPos.x - op.x;
        float dz = testPos.z - op.z;
        float dist2 = dx * dx + dz * dz;

        if (dynamic_cast<Vehicle *>(obj) != nullptr)
        {
            float minDst = 0.26f;
            if (dist2 < minDst * minDst)
                return true;
        }
        else if (dynamic_cast<Tree *>(obj) != nullptr ||
                 dynamic_cast<Lamppost *>(obj) != nullptr ||
                 dynamic_cast<Bench *>(obj) != nullptr ||
                 dynamic_cast<Dustbin *>(obj) != nullptr)
        {
            float minDst = playerRadius + 0.11f;
            if (dist2 < minDst * minDst)
                return true;
        }
    }

    return false;
}

void Simulator::printTelemetry() const
{
    int h = (int)worldTime;
    int m = (int)((worldTime - h) * 60);
    cout << "[" << (h < 10 ? "0" : "") << h << ":" << (m < 10 ? "0" : "") << m << "] "
         << phaseNames[dayPhase]
         << " | ActiveVehicles: " << getActiveVehicleCount()
         << " | Objects: " << objects.size()
         << " | Camera: " << (thirdPersonMode ? "3rd" : "Free")
         << " | JamRecoveries: " << Cross::getTelemetryJamRecoveries()
         << " | ForcedPasses: " << Cross::getTelemetryForcedPasses()
         << " | RLFallbacks: " << Cross::getTelemetryRLFallbacks()
         << " | BridgeAnomalies: " << Vehicle::getBridgeAnomalyCount()
         << endl;
}
