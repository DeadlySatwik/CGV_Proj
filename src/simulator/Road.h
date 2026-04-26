///   File: Road.h

#ifndef ROAD_H
#define ROAD_H

#include "EngineCore/ExceptionClass.h"
#include "GameObject.h"
#include "Vehicle.h"
#include "RLTrafficClient.h"
#include <sstream>
#include <algorithm>

class Cross;
class Vehicle;
class Simulator;
class ObjectsLoader;

class Road : public GameObject
{
public:
    static Vec3 roadColor;
    static Vec3 sideColor;    // side wall color (darker)
    static Vec3 curbColor;    // curb concrete color
    static Vec3 markingColor; // lane marking color
    static Vec3 groundColor;  // ground plane color

    static const float ROAD_DEPTH; // depth below y=0
    static const float CURB_H;     // curb height above y=0
    static const float CURB_W;     // curb width
    static const float SIDEWALK_W; // sidewalk width beyond curb
protected:
    virtual ~Road() {};
};

class Driveable : public Road
{
public:
    Vec3 getJointPoint(const bool dir) const;
    Vec3 getNormal() const;
    Vec3 getDirection() const;
    float getLength() const;

    // Height / elevation support (AWAS)
    float getHeightAt(float t) const;
    float getBegHeight() const;
    float getEndHeight() const;
    bool isElevated() const;
    int getCollisionLayer() const; // 0=ground, 1=elevated

protected:
    Driveable(Cross *begCross, Cross *endCross);
    Driveable(Vec3 p, Cross *endCross);
    virtual ~Driveable() {};

    std::queue<Vehicle *> vehiclesBeg;
    std::queue<Vehicle *> vehiclesEnd;

    Vec3 begPos;
    Vec3 endPos;

    Vec3 begJoint;
    Vec3 endJoint;

    float begHeight; // Y offset at begPos (default 0)
    float endHeight; // Y offset at endPos (default 0)

    Vec3 getBegJointWidth(const bool dir) const;
    Vec3 getEndJointWidth(const bool dir) const;

    Vec3 direction;
    Vec3 normal;

    float length;

    virtual float freeSpace(const bool dir) const;

    Cross *crossBeg;
    Cross *crossEnd;

    void draw();
    void drawElevatedSupports(); // pylons + railings

private:
    float reservedSpaceBeg;
    float reservedSpaceEnd;

    void commonConstructor();

    friend Vehicle;
};

class Street : public Driveable
{
public:
    Street(Cross *begCross, Cross *endCross);
    virtual ~Street();

private:
    void draw();
    std::vector<GameObject *> sidewalkProps;
};

class Cross : public Road
{
public:
    Cross(Vec3 position);
    virtual void setDefaultPriority(Driveable *s0 = nullptr, Driveable *s1 = nullptr, Driveable *s2 = nullptr, Driveable *s3 = nullptr);

    float getIntersectionHeight() const;

    static int getTelemetryForcedPasses();
    static int getTelemetryJamRecoveries();
    static int getTelemetryRLFallbacks();

protected:
    virtual ~Cross() {};

    float intersectionHeight; // Y offset (default 0)

    struct OneStreet
    {
        Driveable *street;
        std::vector<Vehicle *> vehicles;
        bool direction;
        Vec3 getJointPos() const;

        std::vector<std::vector<int>> yield;
    };
    std::vector<OneStreet> streets;

    virtual void updateCross(const float delta);
    virtual bool dontCheckStreet(const int which);

    virtual void tryPassVehiclesWithRightOfWay();
    virtual void tryPassAnyVehicle();
    virtual int getTotalWaitingVehicles() const;
    virtual void tryForceUnjamPass();

    static void incrementTelemetryJamRecoveries();
    static void incrementTelemetryRLFallbacks();

    void draw();

private:
    bool isSet;
    int allowedVeh;
    float noGrantTimer;
    int roundRobinCursor;

    static int telemetryForcedPasses;
    static int telemetryJamRecoveries;
    static int telemetryRLFallbacks;

    bool checkSet();
    void update(const float delta);

    friend Driveable;
    friend Vehicle;
    friend ObjectsLoader;
};

class CrossLights : public Cross
{
public:
    CrossLights(Vec3 position);
    void setLightsDurations();

    struct LightsDuration
    {
        float durationGreen1;
        float durationGreen2;
        float durationYellow1;
        float durationYellow2;
        float durationBreak;
    } durLight;

private:
    enum ControlState
    {
        CTRL_NORMAL,
        CTRL_STARVATION_RECOVERY,
        CTRL_GRIDLOCK_RECOVERY,
        CTRL_FAILSAFE
    };

    std::vector<bool> defaultPriority;
    std::vector<bool> curPriority;

    void setDefaultPriority(Driveable *s0 = NULL, Driveable *s1 = NULL, Driveable *s2 = NULL, Driveable *s3 = NULL);
    void setDefaultLights(Driveable *s0, Driveable *s1, Driveable *s2, Driveable *s3);
    void setLightsPriority();
    void applyAction(int action);
    void collectState(int &north, int &south, int &east, int &west) const;
    int classifyStreetCardinal(const OneStreet &street) const;

    float curTime;
    float greenDuration;
    float decisionInterval;
    float decisionTimer;
    float minGreenDuration;
    float maxGreenDuration;
    bool phaseNS;
    bool useRL;
    int rlFailureStreak;
    float rlDisableTimer;
    float starvationTimer;
    float gridlockTimer;
    ControlState controlState;
    float controlStateTimer;

    RLTrafficClient rlClient;

    bool dontCheckStreet(const int which);

    void update(const float delta);
    void draw();
};

#endif // STREET_H
