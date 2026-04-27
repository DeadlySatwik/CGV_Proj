///   File: CrosswalkZone.cpp
///   Zebra crossing geometry, zone checks, and visual rendering.

#include "CrosswalkZone.h"
#include "Road.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float CrosswalkZone::ROAD_HW       = 0.3f;
const float CrosswalkZone::APPROACH_DIST  = 1.5f;
const float CrosswalkZone::STOP_DIST      = 0.4f;

CrosswalkZone::CrosswalkZone(const std::string& crosswalkId, Driveable* streetPtr,
                             float t, float width)
    : cwId(crosswalkId), street(streetPtr), paramT(t), crossingWidth(width),
      roadHeight(0.0f)
{
    computeGeometry();
}

void CrosswalkZone::computeGeometry()
{
    if (!street) return;

    roadDir    = street->getDirection();
    roadNormal = street->getNormal();

    // Compute center position: lerp between joint points
    Vec3 begJ = street->getJointPoint(true);
    Vec3 endJ = street->getJointPoint(false);

    center.x = begJ.x + (endJ.x - begJ.x) * paramT;
    center.y = 0.0f;
    center.z = begJ.z + (endJ.z - begJ.z) * paramT;

    // Get road height at this parametric position
    roadHeight = street->getHeightAt(paramT);
}

// ---------- Zone hit-tests ----------

bool CrosswalkZone::isInRoadRect(const Vec3& pos, const Vec3& rectCenter,
                                  float halfAlong, float halfAcross) const
{
    // Project (pos - rectCenter) onto road direction and normal
    float dx = pos.x - rectCenter.x;
    float dz = pos.z - rectCenter.z;

    float along  = dx * roadDir.x    + dz * roadDir.z;
    float across = dx * roadNormal.x + dz * roadNormal.z;

    return (fabs(along) <= halfAlong) && (fabs(across) <= halfAcross);
}

bool CrosswalkZone::isInsideCrossingZone(const Vec3& pos) const
{
    return isInRoadRect(pos, center, crossingWidth * 0.5f, ROAD_HW + 0.05f);
}

bool CrosswalkZone::isInsideApproachZone(const Vec3& pos) const
{
    // Approach zone: wider along road direction (crossingWidth/2 + APPROACH_DIST on each side)
    float halfAlong = crossingWidth * 0.5f + APPROACH_DIST;
    return isInRoadRect(pos, center, halfAlong, ROAD_HW + 0.05f);
}

bool CrosswalkZone::isInsideStopZone(const Vec3& pos, const Vec3& travelDir) const
{
    // Stop zone is a narrow strip on the approaching side of the crossing
    float dx = pos.x - center.x;
    float dz = pos.z - center.z;
    float along = dx * roadDir.x + dz * roadDir.z;

    float travelDot = travelDir.x * roadDir.x + travelDir.z * roadDir.z;

    float edgeDist;
    if (travelDot > 0) {
        edgeDist = along + crossingWidth * 0.5f;
    } else {
        edgeDist = crossingWidth * 0.5f - along;
    }

    float across = dx * roadNormal.x + dz * roadNormal.z;
    return (edgeDist >= 0.0f && edgeDist <= STOP_DIST) && (fabs(across) <= ROAD_HW + 0.05f);
}

// ---------- Spawn points ----------

Vec3 CrosswalkZone::getSpawnPointA() const
{
    float spawnOffset = ROAD_HW + 0.15f;
    return Vec3(center.x + roadNormal.x * spawnOffset,
                roadHeight,
                center.z + roadNormal.z * spawnOffset);
}

Vec3 CrosswalkZone::getSpawnPointB() const
{
    float spawnOffset = ROAD_HW + 0.15f;
    return Vec3(center.x - roadNormal.x * spawnOffset,
                roadHeight,
                center.z - roadNormal.z * spawnOffset);
}

// ---------- Drawing ----------

void CrosswalkZone::drawCrossing()
{
    if (!street) return;

    float y = roadHeight + 0.004f; // slightly above road surface to avoid z-fighting

    // === Disable lighting first so glColor3f actually controls color ===
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // --- Zebra stripes ---
    int numStripes = 8;
    float stripeWidth = crossingWidth / (float)(numStripes * 2 - 1);
    float acrossHalf = ROAD_HW - 0.02f;

    glColor3f(0.95f, 0.95f, 0.90f); // white stripes

    for (int i = 0; i < numStripes; ++i)
    {
        // Position along road direction for this stripe
        float offset = -crossingWidth * 0.5f + stripeWidth * (2 * i + 0.15f);
        float stripeHalfW = stripeWidth * 0.4f;

        Vec3 stripeCenter = Vec3(
            center.x + roadDir.x * offset,
            y,
            center.z + roadDir.z * offset
        );

        // Four corners of the stripe
        Vec3 c1(stripeCenter.x + roadDir.x * stripeHalfW + roadNormal.x * acrossHalf,
                y,
                stripeCenter.z + roadDir.z * stripeHalfW + roadNormal.z * acrossHalf);
        Vec3 c2(stripeCenter.x - roadDir.x * stripeHalfW + roadNormal.x * acrossHalf,
                y,
                stripeCenter.z - roadDir.z * stripeHalfW + roadNormal.z * acrossHalf);
        Vec3 c3(stripeCenter.x - roadDir.x * stripeHalfW - roadNormal.x * acrossHalf,
                y,
                stripeCenter.z - roadDir.z * stripeHalfW - roadNormal.z * acrossHalf);
        Vec3 c4(stripeCenter.x + roadDir.x * stripeHalfW - roadNormal.x * acrossHalf,
                y,
                stripeCenter.z + roadDir.z * stripeHalfW - roadNormal.z * acrossHalf);

        glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(c1.x, c1.y, c1.z);
        glVertex3f(c2.x, c2.y, c2.z);
        glVertex3f(c3.x, c3.y, c3.z);
        glVertex3f(c4.x, c4.y, c4.z);
        glEnd();
    }

    // --- Crossing boundary lines (two solid lines at edges) ---
    glColor3f(1.0f, 1.0f, 0.7f); // pale yellow boundary lines
    glLineWidth(2.5f);
    for (int side = -1; side <= 1; side += 2)
    {
        float edgeOffset = side * crossingWidth * 0.5f;
        Vec3 lineCenter(
            center.x + roadDir.x * edgeOffset,
            y + 0.001f,
            center.z + roadDir.z * edgeOffset
        );

        Vec3 la(lineCenter.x + roadNormal.x * ROAD_HW,
                y + 0.001f,
                lineCenter.z + roadNormal.z * ROAD_HW);
        Vec3 lb(lineCenter.x - roadNormal.x * ROAD_HW,
                y + 0.001f,
                lineCenter.z - roadNormal.z * ROAD_HW);

        glBegin(GL_LINES);
        glVertex3f(la.x, la.y, la.z);
        glVertex3f(lb.x, lb.y, lb.z);
        glEnd();
    }

    // --- Stop line markings (thick white line just before crossing, both sides) ---
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    for (int side = -1; side <= 1; side += 2)
    {
        float stopOffset = side * (crossingWidth * 0.5f + 0.04f);
        Vec3 stopCenter(
            center.x + roadDir.x * stopOffset,
            y + 0.002f,
            center.z + roadDir.z * stopOffset
        );

        Vec3 sa(stopCenter.x + roadNormal.x * (ROAD_HW - 0.03f),
                y + 0.002f,
                stopCenter.z + roadNormal.z * (ROAD_HW - 0.03f));
        Vec3 sb(stopCenter.x - roadNormal.x * (ROAD_HW - 0.03f),
                y + 0.002f,
                stopCenter.z - roadNormal.z * (ROAD_HW - 0.03f));

        glBegin(GL_LINES);
        glVertex3f(sa.x, sa.y, sa.z);
        glVertex3f(sb.x, sb.y, sb.z);
        glEnd();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}
