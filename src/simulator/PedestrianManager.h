///   File: PedestrianManager.h
///   Orchestrator for crosswalk zones, pedestrian signals, and pedestrian actors.

#ifndef PEDESTRIANMANAGER_H
#define PEDESTRIANMANAGER_H

#include "CrosswalkZone.h"
#include "PedestrianSignal.h"
#include "Pedestrian.h"
#include "EngineCore/Graphics.h"
#include <vector>
#include <string>

class GameObject;
class Driveable;

/// Bundles a crosswalk zone with its signal and active pedestrians
struct CrosswalkData
{
    CrosswalkZone* zone;
    PedestrianSignal signal;
    std::vector<Pedestrian*> pedestrians;
    float spawnCooldownTimer;
    float spawnCooldown;          // seconds between spawns
    int   maxActivePedestrians;   // max simultaneous at this crossing

    CrosswalkData()
        : zone(NULL), spawnCooldownTimer(0.0f),
          spawnCooldown(6.0f), maxActivePedestrians(2) {}
};

class PedestrianManager : public Graphics
{
public:
    PedestrianManager();
    ~PedestrianManager();

    /// Call once after road loading. Finds streets and creates crosswalk zones.
    void initCrosswalks(const std::vector<GameObject*>& objects);

    /// Per-frame update: signals, spawning, pedestrian state machines, cleanup
    void update(float delta);

    /// Draw all crosswalks, signals, and pedestrians
    void drawAll();

    /// Reset all state (e.g. when switching modes)
    void reset();

    // --- Rule enforcement queries (used by TrainingManager) ---

    /// Is the position inside any crosswalk's crossing zone?
    bool isPlayerInCrosswalkZone(const Vec3& playerPos) const;

    /// Is the position inside any crosswalk's approach zone?
    bool isPlayerInApproachZone(const Vec3& playerPos) const;

    /// Is there a pedestrian currently crossing at the crosswalk nearest to playerPos?
    bool isPedestrianCrossingNear(const Vec3& playerPos) const;

    /// Does any pedestrian near playerPos have right-of-way (WAITING with WALK or CROSSING)?
    bool pedestrianHasRightOfWayNear(const Vec3& playerPos) const;

    /// Get the ID of the nearest crosswalk to a position (empty if none nearby)
    std::string getNearestCrosswalkId(const Vec3& playerPos) const;

    /// Get the signal state of the nearest crosswalk
    PedestrianSignal::SignalState getNearestSignalState(const Vec3& playerPos) const;

    // --- Enable/disable ---
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }

    /// Get active pedestrian positions (for collision checks)
    void getActivePedestrianPositions(std::vector<Vec3>& outPositions) const;

private:
    std::vector<CrosswalkData> crosswalks;
    bool enabled;
    bool initialized;

    void spawnPedestrian(CrosswalkData& cw);
    void cleanupDespawned(CrosswalkData& cw);

    /// Find a Driveable (Street) by ID in the objects list
    Driveable* findStreetById(const std::string& streetId,
                              const std::vector<GameObject*>& objects) const;

    /// Get index of nearest crosswalk to a position, or -1
    int getNearestCrosswalkIndex(const Vec3& pos) const;

    /// Draw the signal indicator post near a crosswalk
    void drawSignalPost(const CrosswalkData& cw);
};

#endif // PEDESTRIANMANAGER_H
