///   File: PedestrianManager.cpp
///   Crosswalk lifecycle: initialization, signal updates, pedestrian spawn/despawn, drawing.

#include "PedestrianManager.h"
#include "Road.h"
#include "GameObject.h"
#include <iostream>
#include <cstdlib>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// ===== Crosswalk placement configuration =====
// Each entry: { streetId, parametric position, crosswalk ID, phase offset }
struct CrosswalkConfig
{
    const char* streetId;
    float paramT;
    const char* cwId;
    float phaseOffset;
};

static const CrosswalkConfig CROSSWALK_CONFIGS[] = {
    { "HA3", 0.5f, "CW1", 0.0f  },
    { "HB5", 0.5f, "CW2", 6.0f  },
    { "HD2", 0.5f, "CW3", 12.0f },
    { "VB3", 0.5f, "CW4", 18.0f },
};
static const int NUM_CROSSWALKS = sizeof(CROSSWALK_CONFIGS) / sizeof(CROSSWALK_CONFIGS[0]);

// ===== Constructor / Destructor =====

PedestrianManager::PedestrianManager()
    : enabled(false), initialized(false)
{
}

PedestrianManager::~PedestrianManager()
{
    reset();
    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        delete crosswalks[i].zone;
    }
    crosswalks.clear();
}

// ===== Initialization =====

Driveable* PedestrianManager::findStreetById(const std::string& streetId,
                                              const std::vector<GameObject*>& objects) const
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        if (objects[i]->id == streetId)
        {
            Driveable* road = dynamic_cast<Driveable*>(objects[i]);
            if (road) return road;
        }
    }
    return NULL;
}

void PedestrianManager::initCrosswalks(const std::vector<GameObject*>& objects)
{
    if (initialized) return;

    for (int c = 0; c < NUM_CROSSWALKS; ++c)
    {
        const CrosswalkConfig& cfg = CROSSWALK_CONFIGS[c];
        Driveable* street = findStreetById(cfg.streetId, objects);
        if (!street)
        {
            cout << "[PEDESTRIAN] Warning: Could not find street " << cfg.streetId
                 << " for crosswalk " << cfg.cwId << endl;
            continue;
        }

        CrosswalkData cwd;
        cwd.zone = new CrosswalkZone(cfg.cwId, street, cfg.paramT, 0.25f);
        cwd.signal.setDurations(8.0f, 3.0f, 12.0f);
        cwd.signal.setPhaseOffset(cfg.phaseOffset);
        cwd.spawnCooldown = 6.0f;
        cwd.spawnCooldownTimer = 0.0f;
        cwd.maxActivePedestrians = 2;

        crosswalks.push_back(cwd);

        Vec3 cwCenter = cwd.zone->getCenterPos();
        cout << "[PEDESTRIAN] Crosswalk " << cfg.cwId << " placed on street " << cfg.streetId
             << " at t=" << cfg.paramT
             << " -> world pos (" << cwCenter.x << ", " << cwCenter.z << ")"
             << " height=" << cwd.zone->getStreet()->getHeightAt(cfg.paramT) << endl;
    }

    initialized = true;
    cout << "[PEDESTRIAN] Initialized " << crosswalks.size() << " crosswalks." << endl;
}

// ===== Reset =====

void PedestrianManager::reset()
{
    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        for (size_t j = 0; j < crosswalks[i].pedestrians.size(); ++j)
        {
            delete crosswalks[i].pedestrians[j];
        }
        crosswalks[i].pedestrians.clear();
        crosswalks[i].spawnCooldownTimer = 0.0f;
    }
}

// ===== Update =====

void PedestrianManager::update(float delta)
{
    if (!enabled) return;

    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        CrosswalkData& cw = crosswalks[i];

        // Update signal
        cw.signal.update(delta);

        // Update existing pedestrians
        bool signalAllows = cw.signal.canPedestrianStartCrossing();
        for (size_t j = 0; j < cw.pedestrians.size(); ++j)
        {
            cw.pedestrians[j]->update(delta, signalAllows);
        }

        // Cleanup despawned
        cleanupDespawned(cw);

        // Spawn logic
        cw.spawnCooldownTimer += delta;
        if (signalAllows &&
            cw.spawnCooldownTimer >= cw.spawnCooldown &&
            (int)cw.pedestrians.size() < cw.maxActivePedestrians)
        {
            spawnPedestrian(cw);
            cw.spawnCooldownTimer = 0.0f;
        }
    }
}

void PedestrianManager::spawnPedestrian(CrosswalkData& cw)
{
    if (!cw.zone) return;

    // Randomly pick which side to spawn from
    Vec3 spawnPt, targetPt;
    if (rand() % 2 == 0)
    {
        spawnPt  = cw.zone->getSpawnPointA();
        targetPt = cw.zone->getSpawnPointB();
    }
    else
    {
        spawnPt  = cw.zone->getSpawnPointB();
        targetPt = cw.zone->getSpawnPointA();
    }

    // Slight random offset along road direction so pedestrians don't stack
    Vec3 roadDir = cw.zone->getRoadDirection();
    float jitter = ((rand() % 100) - 50) * 0.001f;
    spawnPt.x  += roadDir.x * jitter;
    spawnPt.z  += roadDir.z * jitter;
    targetPt.x += roadDir.x * jitter;
    targetPt.z += roadDir.z * jitter;

    // Random speed variation (0.25 to 0.35)
    float spd = 0.25f + (rand() % 100) * 0.001f;

    Pedestrian* ped = new Pedestrian(spawnPt, targetPt, spd);
    cw.pedestrians.push_back(ped);
}

void PedestrianManager::cleanupDespawned(CrosswalkData& cw)
{
    // Remove despawned pedestrians (iterate backward)
    for (int j = (int)cw.pedestrians.size() - 1; j >= 0; --j)
    {
        if (cw.pedestrians[j]->isFinished())
        {
            delete cw.pedestrians[j];
            cw.pedestrians.erase(cw.pedestrians.begin() + j);
        }
    }
}

// ===== Drawing =====

void PedestrianManager::drawAll()
{
    // Crosswalk stripes are road markings — always draw them regardless of mode
    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        if (crosswalks[i].zone)
        {
            crosswalks[i].zone->drawCrossing();
        }
    }

    // Pedestrians and signal posts only drawn when system is enabled (Training/Exam)
    if (!enabled) return;

    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        const CrosswalkData& cw = crosswalks[i];

        // Draw signal indicator post
        drawSignalPost(cw);

        // Draw pedestrians
        for (size_t j = 0; j < cw.pedestrians.size(); ++j)
        {
            cw.pedestrians[j]->drawPedestrian();
        }
    }
}

void PedestrianManager::drawSignalPost(const CrosswalkData& cw)
{
    if (!cw.zone) return;

    // Signal post at spawn point A (one side of road)
    Vec3 postPos = cw.zone->getSpawnPointA();

    // Offset slightly further from road for visibility
    Vec3 roadNorm = cw.zone->getRoadNormal();
    postPos.x += roadNorm.x * 0.05f;
    postPos.z += roadNorm.z * 0.05f;

    // Pole
    setColor(0.4f, 0.4f, 0.42f);
    pushMatrix();
    translate(postPos.x, postPos.y + 0.12f, postPos.z);
    drawCube(0.008f, 0.24f, 0.008f);
    popMatrix();

    // Signal box at top
    PedestrianSignal::SignalState ss = cw.signal.getState();
    float boxY = postPos.y + 0.22f;

    // Background box
    setColor(0.15f, 0.15f, 0.15f);
    pushMatrix();
    translate(postPos.x, boxY, postPos.z);
    drawCube(0.018f, 0.028f, 0.012f);
    popMatrix();

    // Signal light
    switch (ss)
    {
    case PedestrianSignal::WALK:
        setColor(0.1f, 0.9f, 0.2f);  // green
        break;
    case PedestrianSignal::FLASHING:
    {
        // Blink yellow at ~4 Hz
        float blinkPhase = cw.signal.getTimeRemaining();
        bool on = (fmod(blinkPhase * 4.0f, 1.0f) < 0.5f);
        if (on) setColor(1.0f, 0.85f, 0.0f);   // yellow
        else    setColor(0.2f, 0.2f, 0.0f);      // dim
        break;
    }
    case PedestrianSignal::DONT_WALK:
    default:
        setColor(0.9f, 0.1f, 0.1f);  // red
        break;
    }

    pushMatrix();
    translate(postPos.x, boxY + 0.005f, postPos.z);
    drawCube(0.012f, 0.012f, 0.013f);
    popMatrix();

    // Draw a small walk figure icon when WALK is active
    if (ss == PedestrianSignal::WALK)
    {
        setColor(0.0f, 1.0f, 0.3f);
        pushMatrix();
        translate(postPos.x, boxY + 0.022f, postPos.z);
        drawCube(0.004f, 0.008f, 0.004f); // tiny walk stick figure body
        popMatrix();
        pushMatrix();
        translate(postPos.x, boxY + 0.032f, postPos.z);
        drawCube(0.003f, 0.003f, 0.003f); // head
        popMatrix();
    }
}

// ===== Rule enforcement queries =====

int PedestrianManager::getNearestCrosswalkIndex(const Vec3& pos) const
{
    int bestIdx = -1;
    float bestDist = 1e9f;

    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        if (!crosswalks[i].zone) continue;
        Vec3 cwPos = crosswalks[i].zone->getCenterPos();
        float dx = pos.x - cwPos.x;
        float dz = pos.z - cwPos.z;
        float dist = sqrt(dx * dx + dz * dz);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = (int)i;
        }
    }
    return bestIdx;
}

bool PedestrianManager::isPlayerInCrosswalkZone(const Vec3& playerPos) const
{
    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        if (crosswalks[i].zone && crosswalks[i].zone->isInsideCrossingZone(playerPos))
            return true;
    }
    return false;
}

bool PedestrianManager::isPlayerInApproachZone(const Vec3& playerPos) const
{
    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        if (crosswalks[i].zone && crosswalks[i].zone->isInsideApproachZone(playerPos))
            return true;
    }
    return false;
}

bool PedestrianManager::isPedestrianCrossingNear(const Vec3& playerPos) const
{
    int idx = getNearestCrosswalkIndex(playerPos);
    if (idx < 0) return false;

    // Only check if player is actually near this crosswalk (within approach distance)
    if (!crosswalks[idx].zone->isInsideApproachZone(playerPos))
        return false;

    const CrosswalkData& cw = crosswalks[idx];
    for (size_t j = 0; j < cw.pedestrians.size(); ++j)
    {
        if (cw.pedestrians[j]->isInCrosswalk())
            return true;
    }
    return false;
}

bool PedestrianManager::pedestrianHasRightOfWayNear(const Vec3& playerPos) const
{
    int idx = getNearestCrosswalkIndex(playerPos);
    if (idx < 0) return false;

    if (!crosswalks[idx].zone->isInsideApproachZone(playerPos))
        return false;

    const CrosswalkData& cw = crosswalks[idx];

    // Pedestrians with right-of-way
    for (size_t j = 0; j < cw.pedestrians.size(); ++j)
    {
        if (cw.pedestrians[j]->hasRightOfWay())
            return true;
    }

    // Also check if the signal is WALK (even if no ped yet, signal says WALK means yield)
    // Only enforce this when a pedestrian is actually present
    return false;
}

std::string PedestrianManager::getNearestCrosswalkId(const Vec3& playerPos) const
{
    int idx = getNearestCrosswalkIndex(playerPos);
    if (idx >= 0 && crosswalks[idx].zone)
        return crosswalks[idx].zone->getId();
    return "";
}

PedestrianSignal::SignalState PedestrianManager::getNearestSignalState(const Vec3& playerPos) const
{
    int idx = getNearestCrosswalkIndex(playerPos);
    if (idx >= 0)
        return crosswalks[idx].signal.getState();
    return PedestrianSignal::DONT_WALK;
}

void PedestrianManager::getActivePedestrianPositions(std::vector<Vec3>& outPositions) const
{
    outPositions.clear();
    if (!enabled) return;

    for (size_t i = 0; i < crosswalks.size(); ++i)
    {
        for (size_t j = 0; j < crosswalks[i].pedestrians.size(); ++j)
        {
            Pedestrian* ped = crosswalks[i].pedestrians[j];
            if (ped->getState() != Pedestrian::PED_DESPAWNED)
            {
                outPositions.push_back(ped->getPosition());
            }
        }
    }
}
