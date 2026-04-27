///   File: CrosswalkZone.h
///   Defines zebra crossing geometry and logical bounding zones on road segments.

#ifndef CROSSWALKZONE_H
#define CROSSWALKZONE_H

#include "EngineCore/Graphics.h"
#include "EngineCore/Vec3.h"
#include <string>

class Driveable;

class CrosswalkZone : public Graphics
{
public:
    /// streetPtr: the Driveable (Street) this crosswalk is placed on
    /// t: parametric position along the street (0.0 = beg end, 1.0 = end end)
    /// width: crossing width along road direction (world units)
    CrosswalkZone(const std::string& crosswalkId, Driveable* streetPtr, float t, float width = 0.25f);

    // --- Getters ---
    std::string getId() const { return cwId; }
    Vec3 getCenterPos() const { return center; }
    Vec3 getRoadDirection() const { return roadDir; }
    Vec3 getRoadNormal() const { return roadNormal; }
    float getWidth() const { return crossingWidth; }
    Driveable* getStreet() const { return street; }

    // --- Zone hit-tests ---
    /// Full crossing rectangle (where stripes are)
    bool isInsideCrossingZone(const Vec3& pos) const;
    /// Wider approach zone (for early warning)
    bool isInsideApproachZone(const Vec3& pos) const;
    /// Narrow stop-line zone just before crossing edge
    bool isInsideStopZone(const Vec3& pos, const Vec3& travelDir) const;

    // --- Spawn point helpers ---
    /// Sidewalk-edge point on the +normal side of the road
    Vec3 getSpawnPointA() const;
    /// Sidewalk-edge point on the -normal side of the road
    Vec3 getSpawnPointB() const;

    /// Draw zebra stripes, crossing boundary lines, and signal posts
    void drawCrossing();

private:
    std::string cwId;
    Driveable* street;
    float paramT;            // [0,1] along street
    float crossingWidth;     // extent along road direction

    Vec3 center;             // world XZ position of crossing center
    Vec3 roadDir;            // unit direction of road
    Vec3 roadNormal;         // unit normal perpendicular to road
    float roadHeight;        // Y height at paramT

    static const float ROAD_HW;        // road half-width (0.3)
    static const float APPROACH_DIST;   // warning zone distance (1.5)
    static const float STOP_DIST;       // stop-line distance before edge (0.4)

    void computeGeometry();

    /// Test if pos is inside an axis-aligned-to-road rectangle
    /// centered at 'rectCenter', with half-extents 'halfAlong' (road dir) and 'halfAcross' (normal dir)
    bool isInRoadRect(const Vec3& pos, const Vec3& rectCenter,
                      float halfAlong, float halfAcross) const;
};

#endif // CROSSWALKZONE_H
