///   File: Pedestrian.h
///   Individual pedestrian actor with state machine and crossing movement.

#ifndef PEDESTRIAN_H
#define PEDESTRIAN_H

#include "EngineCore/Graphics.h"
#include "EngineCore/Vec3.h"

class Pedestrian : public Graphics
{
public:
    enum PedestrianState
    {
        PED_SPAWNED   = 0,
        PED_WAITING   = 1,
        PED_CROSSING  = 2,
        PED_CLEARED   = 3,
        PED_DESPAWNED = 4
    };

    Pedestrian(const Vec3& spawnPosition, const Vec3& targetPosition, float crossingSpeed = 0.3f);

    /// Advance state machine. signalAllowsCrossing = true when signal is WALK.
    void update(float delta, bool signalAllowsCrossing);

    /// Draw the pedestrian figure
    void drawPedestrian();

    PedestrianState getState() const { return state; }
    Vec3 getPosition() const { return position; }

    /// True while physically in the crossing area (CROSSING state)
    bool isInCrosswalk() const { return state == PED_CROSSING; }

    /// True when the pedestrian has a legal right to be in or entering the crossing
    bool hasRightOfWay() const { return state == PED_WAITING || state == PED_CROSSING; }

    /// True when the pedestrian has finished and should be cleaned up
    bool isFinished() const { return state == PED_DESPAWNED; }

    /// Get crossing progress [0,1] — useful for lane-clear checks
    float getCrossProgress() const { return crossProgress; }

private:
    PedestrianState state;
    Vec3 position;
    Vec3 spawnPos;
    Vec3 targetPos;
    float speed;
    float waitTimer;
    float crossProgress;   // 0.0 to 1.0 (spawn to target)
    float despawnTimer;

    // Visual variety
    Vec3 bodyColor;
    Vec3 pantsColor;
    float bodyHeight;      // slight height variation
    int variant;           // visual variant index

    static const float WAIT_MIN_TIME;   // minimum wait at sidewalk
    static const float DESPAWN_DELAY;   // linger time after crossing

    void drawBody();
    void drawHead();
    void drawLegs(float walkCycle);
    void drawArms(float walkCycle);
};

#endif // PEDESTRIAN_H
