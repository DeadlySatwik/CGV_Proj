///   File: Road.cpp


#include "Road.h"
#include <cmath>
using namespace std;

class Simulator;

Vec3 Road::roadColor = Vec3(0.3, 0.3, 0.3);
Vec3 Road::sideColor = Vec3(0.18, 0.18, 0.18);
Vec3 Road::curbColor = Vec3(0.55, 0.55, 0.50);
Vec3 Road::markingColor = Vec3(0.9, 0.9, 0.85);
Vec3 Road::groundColor = Vec3(0.06, 0.10, 0.04);

const float Road::ROAD_DEPTH = 0.06f;
const float Road::CURB_H = 0.025f;
const float Road::CURB_W = 0.025f;
const float Road::SIDEWALK_W = 0.08f;

float Driveable::getLength() const
{
    return length;
}

Driveable::Driveable(Cross *begCross, Cross *endCross)
{
    crossBeg = begCross;
    crossEnd = endCross;

    begPos = crossBeg->getPos();
    endPos = crossEnd->getPos();

    // Inject intersection heights before commonConstructor extracts them
    begPos.y = crossBeg->getIntersectionHeight();
    endPos.y = crossEnd->getIntersectionHeight();

    commonConstructor();

    Cross::OneStreet temp;
    temp.street = this;

    temp.direction = true;
    begJoint = crossBeg->getPos() + direction * 0.3;;
    crossBeg->streets.push_back(temp);

    temp.direction = false;
    endJoint = crossEnd->getPos() - direction * 0.3;
    crossEnd->streets.push_back(temp);
}

Driveable::Driveable(Vec3 p, Cross *endCross)
{
    crossBeg = nullptr;
    crossEnd = endCross;

    pos = p;

    begPos = pos;
    endPos = crossEnd->getPos();
    endPos.y = crossEnd->getIntersectionHeight();  // inject height

    commonConstructor();

    Cross::OneStreet temp;
    temp.street = this;
    temp.direction = false;
    begJoint = begPos;
    endJoint = crossEnd->getPos() - direction * 0.3;
    crossEnd->streets.push_back(temp);
}

void Driveable::commonConstructor()
{
    // Extract Y as height, then flatten for XZ path calculations
    begHeight = begPos.y;
    endHeight = endPos.y;
    begPos.y = 0;
    endPos.y = 0;

    direction = endPos - begPos;
    direction.normalize();

    length = Vec3::dst(begPos, endPos);

    normal = Vec3::cross(Vec3(0,1,0), direction);
    normal.normalize();

    reservedSpaceBeg = 0;
    reservedSpaceEnd = 0;
}

// ===== Height / Elevation accessors =====
float Driveable::getHeightAt(float t) const {
    return begHeight + (endHeight - begHeight) * t;
}
float Driveable::getBegHeight() const { return begHeight; }
float Driveable::getEndHeight() const { return endHeight; }
bool Driveable::isElevated() const {
    return (begHeight > 0.05f || endHeight > 0.05f);
}
int Driveable::getCollisionLayer() const {
    float maxH = (begHeight > endHeight) ? begHeight : endHeight;
    return (maxH > 0.3f) ? 1 : 0;
}

float Driveable::freeSpace(const bool dir) const
{
    if (dir)
    {
        if (vehiclesBeg.size() == 0) return getLength() - reservedSpaceBeg - 0.2;

        return vehiclesBeg.back()->getXPos() - reservedSpaceBeg - 0.2;
    }

    if (vehiclesEnd.size() == 0) return getLength() - reservedSpaceEnd - 0.2;

    return vehiclesEnd.back()->getXPos() - reservedSpaceEnd - 0.2;
}

Vec3 Driveable::getJointPoint(const bool dir) const
{
    if (dir) return begJoint;
    return endJoint;
}

Vec3 Driveable::getNormal() const
{
    return normal;
}

Vec3 Driveable::getDirection() const
{
    return direction;
}

Vec3 Driveable::getBegJointWidth(const bool dir) const
{
    Vec3 p;
    if (dir)
    {
        p = begJoint + normal * 0.1;
    }
    else
    {
        p = begJoint - normal * 0.1;
    }
    p.y = begHeight;   // inject elevation
    return p;
}

Vec3 Driveable::getEndJointWidth(const bool dir) const
{
    Vec3 p;
    if (dir)
    {
        p = endJoint + normal * 0.1;
    }
    else
    {
        p = endJoint - normal * 0.1;
    }
    p.y = endHeight;   // inject elevation
    return p;
}

void Driveable::draw()
{
    Vec3 szer = Vec3::cross(Vec3(0,1,0), direction);
    szer.normalize();
    float hw = 0.3f; // road half-width
    szer *= hw;

    // Road corners at top surface (y=0) - drawn from joint to joint to avoid crossing into intersections
    Vec3 a = endJoint + szer;  // end-left
    Vec3 b = endJoint - szer;  // end-right
    Vec3 c = begJoint + szer;  // beg-left
    Vec3 d = begJoint - szer;  // beg-right

    float depthA = endHeight - ROAD_DEPTH;
    float depthC = begHeight - ROAD_DEPTH;

    // ---- 1. Top surface (road asphalt) ----
    setColor(roadColor);
    beginDraw(QUADS);
    setNormal(0, -1, 0);
    drawQuad(a, b, d, c);
    endDraw();

    // ---- 2. Left side wall ----
    setColor(sideColor);
    beginDraw(QUADS);
    setNormal(normal.x, 0, normal.z);
    drawQuad(c, a,
             Vec3(a.x, depthA, a.z),
             Vec3(c.x, depthC, c.z));
    endDraw();

    // ---- 3. Right side wall ----
    beginDraw(QUADS);
    setNormal(-normal.x, 0, -normal.z);
    drawQuad(b, d,
             Vec3(d.x, depthC, d.z),
             Vec3(b.x, depthA, b.z));
    endDraw();

    // ---- 4. Bottom face ----
    setColor(Vec3(0.12f, 0.12f, 0.12f));
    beginDraw(QUADS);
    setNormal(0, 1, 0);
    drawQuad(Vec3(c.x, depthC, c.z), Vec3(d.x, depthC, d.z),
             Vec3(b.x, depthA, b.z), Vec3(a.x, depthA, a.z));
    endDraw();

    // ---- 5. Curbs (raised edges) ----
    Vec3 curbN = szer;
    curbN.normalize();
    curbN *= CURB_W;
    float cyC = begHeight + CURB_H;
    float cyA = endHeight + CURB_H;

    setColor(curbColor);

    // Left curb - top face
    beginDraw(QUADS);
    setNormal(0, -1, 0);
    drawQuad(Vec3(c.x, cyC, c.z), Vec3(c.x - curbN.x, cyC, c.z - curbN.z),
             Vec3(a.x - curbN.x, cyA, a.z - curbN.z), Vec3(a.x, cyA, a.z));
    endDraw();
    // Left curb - outer face
    beginDraw(QUADS);
    setNormal(normal.x, 0, normal.z);
    drawQuad(Vec3(c.x, cyC, c.z), Vec3(a.x, cyA, a.z),
             a, c);
    endDraw();
    // Left curb - inner face
    beginDraw(QUADS);
    setNormal(-normal.x, 0, -normal.z);
    drawQuad(Vec3(a.x - curbN.x, cyA, a.z - curbN.z),
             Vec3(c.x - curbN.x, cyC, c.z - curbN.z),
             Vec3(c.x - curbN.x, begHeight, c.z - curbN.z),
             Vec3(a.x - curbN.x, endHeight, a.z - curbN.z));
    endDraw();

    // Right curb - top face
    beginDraw(QUADS);
    setNormal(0, -1, 0);
    drawQuad(Vec3(d.x + curbN.x, cyC, d.z + curbN.z), Vec3(d.x, cyC, d.z),
             Vec3(b.x, cyA, b.z), Vec3(b.x + curbN.x, cyA, b.z + curbN.z));
    endDraw();
    // Right curb - outer face
    beginDraw(QUADS);
    setNormal(-normal.x, 0, -normal.z);
    drawQuad(Vec3(b.x, cyA, b.z), Vec3(d.x, cyC, d.z),
             d, b);
    endDraw();
    // Right curb - inner face
    beginDraw(QUADS);
    setNormal(normal.x, 0, normal.z);
    drawQuad(Vec3(d.x + curbN.x, cyC, d.z + curbN.z),
             Vec3(b.x + curbN.x, cyA, b.z + curbN.z),
             Vec3(b.x + curbN.x, endHeight, b.z + curbN.z),
             Vec3(d.x + curbN.x, begHeight, d.z + curbN.z));
    endDraw();

    // ---- 6. Sidewalk strips (skip for elevated to keep clean look) ----
    if (!isElevated())
    {
        Vec3 swN = szer;
        swN.normalize();
        swN *= SIDEWALK_W;
        float swY = CURB_H - 0.005f;

        setColor(Vec3(0.45f, 0.44f, 0.40f));
        // Left sidewalk
        beginDraw(QUADS);
        setNormal(0, -1, 0);
        drawQuad(Vec3(c.x, swY, c.z),
                 Vec3(c.x + swN.x, swY, c.z + swN.z),
                 Vec3(a.x + swN.x, swY, a.z + swN.z),
                 Vec3(a.x, swY, a.z));
        endDraw();
        // Left sidewalk side wall
    setColor(sideColor);
        beginDraw(QUADS);
        setNormal(normal.x, 0, normal.z);
        drawQuad(Vec3(c.x + swN.x, swY, c.z + swN.z),
                 Vec3(a.x + swN.x, swY, a.z + swN.z),
                 Vec3(a.x + swN.x, -ROAD_DEPTH, a.z + swN.z),
                 Vec3(c.x + swN.x, -ROAD_DEPTH, c.z + swN.z));
        endDraw();

        // Right sidewalk
    setColor(Vec3(0.45f, 0.44f, 0.40f));
        beginDraw(QUADS);
        setNormal(0, -1, 0);
        drawQuad(Vec3(d.x - swN.x, swY, d.z - swN.z),
                 Vec3(d.x, swY, d.z),
                 Vec3(b.x, swY, b.z),
                 Vec3(b.x - swN.x, swY, b.z - swN.z));
        endDraw();
        // Right sidewalk side wall
    setColor(sideColor);
        beginDraw(QUADS);
        setNormal(-normal.x, 0, -normal.z);
        drawQuad(Vec3(d.x - swN.x, swY, d.z - swN.z),
                 Vec3(b.x - swN.x, swY, b.z - swN.z),
                 Vec3(b.x - swN.x, -ROAD_DEPTH, b.z - swN.z),
                 Vec3(d.x - swN.x, -ROAD_DEPTH, d.z - swN.z));
        endDraw();
    }

    // ---- 7. Lane markings (follow slope) ----
    setColor(markingColor);
    float jointLength = Vec3::dst(begJoint, endJoint);
    int numDashes = (int)(jointLength / 0.25f);
    if (numDashes < 2) numDashes = 2;
    beginDraw(LINES);
    for (int i = 0; i < numDashes; i++)
    {
        if (i % 2 != 0) continue;
        float t0 = (float)i / (float)numDashes;
        float t1 = (float)(i + 1) / (float)numDashes;
        Vec3 p0 = Vec3::lerp(begJoint, endJoint, t0);
        Vec3 p1 = Vec3::lerp(begJoint, endJoint, t1);
        float y0 = getHeightAt(t0) + 0.002f;
        float y1 = getHeightAt(t1) + 0.002f;
        drawVertex(Vec3(p0.x, y0, p0.z));
        drawVertex(Vec3(p1.x, y1, p1.z));
    }
    endDraw();

    // Solid edge lines
    setColor(Vec3(0.8f, 0.8f, 0.75f));
    float edgeOff = hw - 0.02f;
    Vec3 edgeN = szer;
    edgeN.normalize();
    edgeN *= edgeOff;
    beginDraw(LINES);
    drawVertex(Vec3(begJoint.x + edgeN.x, begHeight + 0.002f, begJoint.z + edgeN.z));
    drawVertex(Vec3(endJoint.x + edgeN.x, endHeight + 0.002f, endJoint.z + edgeN.z));
    drawVertex(Vec3(begJoint.x - edgeN.x, begHeight + 0.002f, begJoint.z - edgeN.z));
    drawVertex(Vec3(endJoint.x - edgeN.x, endHeight + 0.002f, endJoint.z - edgeN.z));
    endDraw();

    // ---- 8. Elevated road supports ----
    if (isElevated())
    {
        drawElevatedSupports();
    }
}

void Driveable::drawElevatedSupports()
{
    // ---- Concrete pylons ----
    setColor(0.38f, 0.38f, 0.40f);
    int numPylons = (int)(length / 2.0f);
    if (numPylons < 1) numPylons = 1;

    for (int p = 0; p <= numPylons; p++)
    {
        float t = (float)p / (float)(numPylons > 0 ? numPylons : 1);
        Vec3 pylonPos = Vec3::lerp(begPos, endPos, t);
        float pylonH = getHeightAt(t);

        if (pylonH > 0.08f)
        {
            // Main column
            pushMatrix();
            translate(pylonPos.x, pylonH * 0.5f - ROAD_DEPTH, pylonPos.z);
            drawCube(0.06f, pylonH, 0.06f);
            popMatrix();

            // Column base (wider)
            pushMatrix();
            setColor(0.32f, 0.32f, 0.35f);
            translate(pylonPos.x, 0.015f, pylonPos.z);
            drawCube(0.10f, 0.03f, 0.10f);
            popMatrix();

            setColor(0.38f, 0.38f, 0.40f);
        }
    }

    // ---- Railing posts (along both edges) ----
    Vec3 szer = Vec3::cross(Vec3(0,1,0), direction);
    szer.normalize();
    float railOff = 0.28f;  // just inside the road edge

    setColor(0.50f, 0.50f, 0.52f);  // metal grey
    int numPosts = (int)(length / 0.3f);
    if (numPosts < 2) numPosts = 2;

    for (int p = 0; p <= numPosts; p++)
    {
        float t = (float)p / (float)numPosts;
        Vec3 rp = Vec3::lerp(begPos, endPos, t);
        float rh = getHeightAt(t);

        if (rh > 0.08f)
        {
            // Left railing post
            pushMatrix();
            translate(rp.x + szer.x * railOff, rh + CURB_H + 0.04f, rp.z + szer.z * railOff);
            drawCube(0.008f, 0.08f, 0.008f);
            popMatrix();

            // Right railing post
            pushMatrix();
            translate(rp.x - szer.x * railOff, rh + CURB_H + 0.04f, rp.z - szer.z * railOff);
            drawCube(0.008f, 0.08f, 0.008f);
            popMatrix();
        }
    }

    // Horizontal rail bars (connecting posts)
    if (length > 0.3f && (begHeight > 0.08f || endHeight > 0.08f))
    {
        setColor(0.45f, 0.45f, 0.48f);
        // Left rail bar
        pushMatrix();
        Vec3 mid = Vec3::lerp(begPos, endPos, 0.5f);
        float midH = getHeightAt(0.5f);
        translate(mid.x + szer.x * railOff, midH + CURB_H + 0.065f, mid.z + szer.z * railOff);
        drawCube(length * 0.5f, 0.006f, 0.006f);
        popMatrix();

        // Right rail bar
        pushMatrix();
        translate(mid.x - szer.x * railOff, midH + CURB_H + 0.065f, mid.z - szer.z * railOff);
        drawCube(length * 0.5f, 0.006f, 0.006f);
        popMatrix();
    }
}

Cross::Cross(Vec3 position)
{
    intersectionHeight = position.y;  // extract height
    pos = position;
    pos.y = 0;  // flatten for XZ logic
    allowedVeh = 0;

    isSet = false;
}

float Cross::getIntersectionHeight() const { return intersectionHeight; }

void Cross::update(const float delta)
{
    updateCross(delta);
}

bool Cross::dontCheckStreet(const int which)
{
    return false;
}

bool Cross::checkSet()
{
    if (isSet) return false;

    if(streets.size() == 4)
    {
        setDefaultPriority(streets[0].street, streets[1].street, streets[2].street, streets[3].street);
    }

    else if (streets.size() == 2)
    {
        setDefaultPriority();
    }

    else if (streets.size() == 3)
    {
        setDefaultPriority(streets[0].street, streets[1].street, streets[2].street);
    }
    else
    {
        throw ExceptionClass("failed to set default right of way at intersection " + id);
    }

    return !isSet;
}

void Cross::updateCross(const float delta)
{
    if (checkSet()) return;

    // For 2-way pass-through nodes, aggressively clear allowedVeh
    // since there's no cross-traffic conflict
    if (streets.size() <= 2)
    {
        tryPassVehiclesWithRightOfWay();
        // Don't let allowedVeh accumulate — these are just pass-throughs
        return;
    }

    if (allowedVeh == 0)
    {
        tryPassVehiclesWithRightOfWay();

        if (allowedVeh == 0)
        {
            tryPassAnyVehicle();
        }
    }
}

void Cross::tryPassVehiclesWithRightOfWay()
{
    vector<int> indexesToPass;

    if (streets.size() == 2)
    {
        for(unsigned int i=0;i<streets.size();i++)
        {
            if (streets[i].vehicles.size() > 0)
            {
                indexesToPass.push_back(i);
            }
        }
    }

    else if (streets.size() > 2)
    for (unsigned int i=0;i<streets.size();i++)
    {
        if (streets[i].vehicles.size() > 0)
        {
            if (dontCheckStreet(i)) continue;
            if (streets[i].vehicles[0]->getDstToCross() > 1.2) continue;

            int which = streets[i].vehicles[0]->desiredTurn;
            vector<int> yielding = streets[i].yield[which];
            bool isOK = true;

            for (unsigned int j = 0; j < yielding.size(); j++)
            {
                if (streets[yielding[j]].vehicles.size() > 0 && !dontCheckStreet(yielding[j]))
                {
                    isOK = false;
                    break;
                }
            }

            if (isOK)
            {
                indexesToPass.push_back(i);
            }
        }
    }

    for (unsigned int i=0;i<indexesToPass.size();i++)
    {
        if (streets[indexesToPass[i]].vehicles[0]->isEnoughSpace())
        {
            streets[indexesToPass[i]].vehicles[0]->allowedToCross = true;
            streets[indexesToPass[i]].vehicles.erase(streets[indexesToPass[i]].vehicles.begin());
            allowedVeh++;
        }
    }
}

void Cross::tryPassAnyVehicle()
{
    for(unsigned int i=0;i<streets.size();i++)
    {
        if (dontCheckStreet(i)) continue;
        if (streets[i].vehicles.size() > 0 && streets[i].vehicles[0]->dstToCross < 0.7)
        {
            if (streets[i].vehicles[0]->isEnoughSpace())
            {
                streets[i].vehicles[0]->allowedToCross = true;
                streets[i].vehicles.erase(streets[i].vehicles.begin());
                allowedVeh++;

                break;
            }
        }
    }
}

void Cross::setDefaultPriority(Driveable *s0, Driveable *s1, Driveable *s2, Driveable *s3)
{
    isSet = true;

    for(unsigned int i=0;i<streets.size();i++)
    {
        streets[i].yield.clear();
    }

    if (streets.size() == 2)
    {

    }
    else if (streets.size() == 3)
    {
        vector<int> yield0;
        yield0.push_back(1);
        vector<int> yield0empty;
        vector<vector<int> > finalVec0;
        finalVec0.push_back(yield0);
        finalVec0.push_back(yield0empty);
        finalVec0.push_back(yield0);

        vector<int> yield1;
        vector<int> yield1empty;
        yield1.push_back(2);
        vector<vector<int> > finalVec1;
        finalVec1.push_back(yield1);
        finalVec1.push_back(yield1);
        finalVec1.push_back(yield1empty);

        vector<int> yield2;
        vector<int> yield2empty;
        yield2.push_back(0);
        vector<vector<int> >finalVec2;
        finalVec2.push_back(yield2empty);
        finalVec2.push_back(yield2);
        //yield2.push_back(1);
        finalVec2.push_back(yield2);

        for(int i=0;i<3;i++)
        {
            if (streets[i].street == s0) streets[i].yield = finalVec0;
            if (streets[i].street == s1) streets[i].yield = finalVec1;
            if (streets[i].street == s2) streets[i].yield = finalVec2;
        }
    }

    else if (streets.size() == 4)
    {
        vector<int> yield[4];

        //yield[0].push_back(1);
        //yield[0].push_back(2);
        //yield[1].push_back(1);

        yield[2].push_back(1);

        yield[3].push_back(1);
        yield[3].push_back(2);

        vector<vector<int> > finalVec;

        finalVec.push_back(yield[0]);
        finalVec.push_back(yield[1]);
        finalVec.push_back(yield[2]);
        finalVec.push_back(yield[3]);

        vector<OneStreet> tempVector = streets;
        for (int i = 0; i < 4; i++)
        {
            if (streets[i].street == s0) tempVector[0] = streets[i];
            if (streets[i].street == s1) tempVector[1] = streets[i];
            if (streets[i].street == s2) tempVector[2] = streets[i];
            if (streets[i].street == s3) tempVector[3] = streets[i];
        }
        streets = tempVector;

        for (int i = 0; i < 4; i++)
        {
            streets[i].yield = finalVec;

            rotate(streets[i].yield.begin(), streets[i].yield.begin() + (-i + 4)%4 , streets[i].yield.end());
            for(int j = 0; j < 4; j++)
            for(unsigned int k = 0; k < streets[i].yield[j].size(); k++)
            {
                streets[i].yield[j][k]+= i;
                streets[i].yield[j][k]%=4;
            }
        }
    }
    else
    {
        throw ExceptionClass("incorrect number of streets at intersection " + id);
    }
}

Vec3 Cross::OneStreet::getJointPos() const
{
    return street->getJointPoint(direction);
}

void Cross::draw()
{
    float a = 0.3f;  // half-size (matching 0.6 tile)
    float h = intersectionHeight;  // height offset
    float depth = h - ROAD_DEPTH;
    float cy = h + CURB_H;
    float markY = h + 0.002f;

    // ---- 1. Top surface ----
    setColor(roadColor);
    beginDraw(QUADS);
    setNormal(0, -1, 0);
    drawQuad(Vec3(-a, h, -a), Vec3(a, h, -a), Vec3(a, h, a), Vec3(-a, h, a));
    endDraw();

    // ---- 2. Four side walls ----
    setColor(sideColor);
    // Front (-Z face)
    beginDraw(QUADS);
    setNormal(0, 0, -1);
    drawQuad(Vec3(-a, h, -a), Vec3(a, h, -a),
             Vec3(a, depth, -a), Vec3(-a, depth, -a));
    endDraw();
    // Back (+Z face)
    beginDraw(QUADS);
    setNormal(0, 0, 1);
    drawQuad(Vec3(a, h, a), Vec3(-a, h, a),
             Vec3(-a, depth, a), Vec3(a, depth, a));
    endDraw();
    // Left (-X face)
    beginDraw(QUADS);
    setNormal(-1, 0, 0);
    drawQuad(Vec3(-a, h, a), Vec3(-a, h, -a),
             Vec3(-a, depth, -a), Vec3(-a, depth, a));
    endDraw();
    // Right (+X face)
    beginDraw(QUADS);
    setNormal(1, 0, 0);
    drawQuad(Vec3(a, h, -a), Vec3(a, h, a),
             Vec3(a, depth, a), Vec3(a, depth, -a));
    endDraw();

    // ---- 3. Bottom face ----
    setColor(Vec3(0.12f, 0.12f, 0.12f));
    beginDraw(QUADS);
    setNormal(0, 1, 0);
    drawQuad(Vec3(-a, depth, -a), Vec3(a, depth, -a),
             Vec3(a, depth, a), Vec3(-a, depth, a));
    endDraw();

    // ---- 4. Corner curb posts ----
    float cp = a + 0.01f;
    float cpSize = CURB_W;
    setColor(curbColor);
    for (int cx = -1; cx <= 1; cx += 2)
    {
        for (int cz = -1; cz <= 1; cz += 2)
        {
            pushMatrix();
            translate(cx * cp, h + CURB_H * 0.5f, cz * cp);
            drawCube(cpSize, CURB_H, cpSize);
            popMatrix();
        }
    }

    // ---- 4.5. Missing boundaries (curbs + sidewalks) for unconnected sides ----
    bool hasRoad[4] = {false, false, false, false}; // +X, -X, +Z, -Z
    for (const auto& st : streets) {
        Vec3 dir = st.street->getDirection();
        if (!st.direction) dir = Vec3(0,0,0) - dir; // Reverse direction (OUT of cross)
        if (dir.x > 0.5f) hasRoad[0] = true;
        else if (dir.x < -0.5f) hasRoad[1] = true;
        else if (dir.z > 0.5f) hasRoad[2] = true;
        else if (dir.z < -0.5f) hasRoad[3] = true;
    }

    float curbCenter = a - CURB_W * 0.5f;
    float swCenter = a + SIDEWALK_W * 0.5f;
    float swY = cy - 0.005f;
    float swH = swY - depth;
    float swCY = depth + swH * 0.5f;
    float span = 2 * a;
    float spanSW = 2 * a + 2 * SIDEWALK_W; // extended to cover corners

    for (int i = 0; i < 4; i++) {
        if (!hasRoad[i]) {
            float cx = (i == 0) ? 1 : (i == 1) ? -1 : 0;
            float cz = (i == 2) ? 1 : (i == 3) ? -1 : 0;
            
            // Draw Curb (inward from 'a')
            setColor(curbColor);
            pushMatrix();
            translate(cx * curbCenter, cy * 0.5f, cz * curbCenter);
            if (i < 2) drawCube(CURB_W, cy, span);
            else drawCube(span, cy, CURB_W);
            popMatrix();

            // Draw Sidewalk (outward from 'a')
            setColor(Vec3(0.45f, 0.44f, 0.40f));
            pushMatrix();
            translate(cx * swCenter, swCY, cz * swCenter);
            if (i < 2) drawCube(SIDEWALK_W, swH, spanSW);
            else drawCube(spanSW, swH, SIDEWALK_W);
            popMatrix();

            // Draw outermost side wall using sideColor
            setColor(sideColor);
            float outEdge = a + SIDEWALK_W;
            float spanHalf = spanSW * 0.5f;
            beginDraw(QUADS);
            if (i == 0) { // +X
                setNormal(1, 0, 0);
                drawQuad(Vec3(outEdge, depth, -spanHalf), Vec3(outEdge, depth, spanHalf),
                         Vec3(outEdge, swY, spanHalf), Vec3(outEdge, swY, -spanHalf));
            } else if (i == 1) { // -X
                setNormal(-1, 0, 0);
                drawQuad(Vec3(-outEdge, depth, spanHalf), Vec3(-outEdge, depth, -spanHalf),
                         Vec3(-outEdge, swY, -spanHalf), Vec3(-outEdge, swY, spanHalf));
            } else if (i == 2) { // +Z
                setNormal(0, 0, 1);
                drawQuad(Vec3(spanHalf, depth, outEdge), Vec3(-spanHalf, depth, outEdge),
                         Vec3(-spanHalf, swY, outEdge), Vec3(spanHalf, swY, outEdge));
            } else if (i == 3) { // -Z
                setNormal(0, 0, -1);
                drawQuad(Vec3(-spanHalf, depth, -outEdge), Vec3(spanHalf, depth, -outEdge),
                         Vec3(spanHalf, swY, -outEdge), Vec3(-spanHalf, swY, -outEdge));
            }
            endDraw();
        }
    }

    // ---- 5. Subtle crosshatch marking ----
    setColor(Vec3(0.35f, 0.35f, 0.35f));
    beginDraw(LINES);
    float step = 0.12f;
    for (float s = -a + step; s < a; s += step)
    {
        drawVertex(Vec3(s, markY, -a)); drawVertex(Vec3(s, markY, a));
        drawVertex(Vec3(-a, markY, s)); drawVertex(Vec3(a, markY, s));
    }
    endDraw();

    // ---- 6. Support pylons for elevated intersections ----
    if (h > 0.08f)
    {
        setColor(0.38f, 0.38f, 0.40f);
        // Center pylon
        pushMatrix();
        translate(0, h * 0.5f - ROAD_DEPTH, 0);
        drawCube(0.08f, h, 0.08f);
        popMatrix();

        // Corner pylons (thinner)
        float pylOff = a * 0.6f;
        setColor(0.35f, 0.35f, 0.38f);
        for (int px = -1; px <= 1; px += 2)
        {
            for (int pz = -1; pz <= 1; pz += 2)
            {
                pushMatrix();
                translate(px * pylOff, h * 0.5f - ROAD_DEPTH, pz * pylOff);
                drawCube(0.05f, h, 0.05f);
                popMatrix();
            }
        }
    }
}

void CrossLights::setDefaultPriority(Driveable *s0, Driveable *s1, Driveable *s2, Driveable *s3)
{
    Cross::setDefaultPriority(s0, s1, s2, s3);
    setDefaultLights(s0, s1, s2, s3);
}

bool CrossLights::dontCheckStreet(const int which)
{
    return !curPriority[which];
}

void CrossLights::setDefaultLights(Driveable *s0, Driveable *s1, Driveable *s2, Driveable *s3)
{
    defaultPriority.clear();
    curPriority.clear();

    for (unsigned int i=0;i<streets.size();i++)
    {
        defaultPriority.push_back(false);
        curPriority.push_back(false);
    }

    if (streets.size() == 3)
    {
        //defaultPriority[0] = true;
        for (unsigned int i=0;i<streets.size();i++)
        {
            if (streets[i].street == s0) defaultPriority[i] = true;
        }
    }

    else if (streets.size() == 4)
    {
        for (unsigned int i=0;i<streets.size();i++)
        {
            if (streets[i].street == s0) defaultPriority[i] = true;
            if (streets[i].street == s2) defaultPriority[i] = true;
        }
    }
    else
    {
        throw ExceptionClass("incorrect number of streets at intersection " + id);
    }
}

void CrossLights::setLightsPriority()
{
    for(unsigned int i=0;i<streets.size();i++)
    {
        if (phaseNS)
        {
            curPriority[i] = defaultPriority[i];
        }
        else
        {
            curPriority[i] = !defaultPriority[i];
        }
    }
}

CrossLights::CrossLights(Vec3 position) : Cross(position)
{
    setLightsDurations();

    phaseNS = true;
    curTime = 0.0f;
    decisionInterval = 0.25f;
    decisionTimer = 0.0f;
    minGreenDuration = 4.0f;
    maxGreenDuration = 45.0f;
    useRL = true;
    setLightsPriority();
}

void CrossLights::setLightsDurations()
{
    greenDuration = randFloat(10, 20);
    durLight.durationGreen1 = greenDuration;
    durLight.durationGreen2 = greenDuration;
    durLight.durationYellow1 = 0;
    durLight.durationYellow2 = 0;
    durLight.durationBreak = 0;
}

int CrossLights::classifyStreetCardinal(const OneStreet &street) const
{
    Vec3 d = street.getJointPos() - pos;
    if (fabs(d.x) > fabs(d.z))
    {
        return d.x >= 0 ? 2 : 3; // east : west
    }
    return d.z >= 0 ? 0 : 1; // north : south
}

void CrossLights::collectState(int &north, int &south, int &east, int &west) const
{
    north = 0;
    south = 0;
    east = 0;
    west = 0;

    for (const auto &street : streets)
    {
        const int c = classifyStreetCardinal(street);
        const int waiting = (int)street.vehicles.size();
        if (c == 0) north += waiting;
        else if (c == 1) south += waiting;
        else if (c == 2) east += waiting;
        else west += waiting;
    }
}

void CrossLights::applyAction(int action)
{
    if (action == 1)
    {
        phaseNS = !phaseNS;
        curTime = 0.0f;
        setLightsPriority();
    }
    else if (action == 2)
    {
        greenDuration = min(maxGreenDuration, greenDuration + 1.0f);
        durLight.durationGreen1 = greenDuration;
        durLight.durationGreen2 = greenDuration;
    }
    else if (action == 3)
    {
        greenDuration = max(minGreenDuration, greenDuration - 1.0f);
        durLight.durationGreen1 = greenDuration;
        durLight.durationGreen2 = greenDuration;
    }
}

void CrossLights::update(const float delta)
{
    updateCross(delta);

    curTime += delta;
    decisionTimer += delta;

    if (decisionTimer >= decisionInterval)
    {
        decisionTimer = 0.0f;

        int carsN, carsS, carsE, carsW;
        collectState(carsN, carsS, carsE, carsW);

        const float reward = -(float)(carsN + carsS + carsE + carsW);

        int action = -1;
        if (useRL)
        {
            action = rlClient.requestAction(carsN,
                                            carsS,
                                            carsE,
                                            carsW,
                                            phaseNS ? 0 : 1,
                                            greenDuration,
                                            reward);
        }

        // If RL server is unavailable, preserve deterministic behavior.
        if (action >= 0 && action <= 3)
        {
            applyAction(action);
        }
        else if (curTime >= greenDuration)
        {
            applyAction(1);
        }
    }

    if (curTime >= greenDuration)
    {
        applyAction(1);
    }
}

void CrossLights::draw()
{
    Cross::draw();

    Vec3 color1 = phaseNS ? Vec3(0,1,0) : Vec3(1,0,0);
    Vec3 color2 = phaseNS ? Vec3(1,0,0) : Vec3(0,1,0);

    translate(-pos);

    for(unsigned int i =0;i<streets.size();i++)
    {
        pushMatrix();
        translate(0,0.35,0);

        if (!streets[i].direction)
            translate(streets[i].street->getNormal() / 10.0);
        else
            translate(-streets[i].street->getNormal() / 10.0);

        translate(streets[i].getJointPos());
        if (defaultPriority[i]) setColor(color1.x, color1.y, color1.z);
        else setColor(color2.x, color2.y, color2.z);

        if (streets[i].direction)
            rotateY(streets[i].street->getNormal().angleXZ());
        else
            rotateY(streets[i].street->getNormal().angleXZ() + 180);

        drawCube(0.05,0.1,0.05);

        translate(-0.2,0,0);

        setColor(0.5, 0.5, 0.5);
        translate(0,-0.35/2,0);
        drawCube(0.025,0.35,0.025);

        translate(0.1,0.35/2,0);
        drawCube(0.225,0.025,0.025);

        popMatrix();
    }
}

#include "Environment.h"

Street::Street(Cross *begCross, Cross *endCross) : Driveable(begCross, endCross)
{
    float len = getLength();
    Vec3 dir = getDirection();
    Vec3 norm = getNormal();
    
    // Sidewalk is centered at HW + SIDEWALK_W/2
    float offsetDist = 0.3f + 0.08f / 2.0f; 
    
    float jointLength = Vec3::dst(begJoint, endJoint);
    int numSpots = (int)(jointLength / 2.0f); 
    if (numSpots < 1) return; // Street too short

    float step = jointLength / (numSpots + 1);
    
    for (int side = -1; side <= 1; side += 2)
    {
        for (int i = 1; i <= numSpots; i++)
        {
            Vec3 basePos = begJoint + dir * (step * i);
            Vec3 pos = basePos + norm * (offsetDist * side);
            
            // Deterministic hash based on world position
            unsigned int hash = (unsigned int)(pos.x * 1337 + pos.z * 8191);
            if ((hash % 100) < 50) // 50% chance of a prop at this spot
            {
                int type = (hash >> 3) % 4; // 0=Tree, 1=Lamppost, 2=Bench, 3=Dustbin
                if (type == 0) sidewalkProps.push_back(new Tree(pos));
                else if (type == 1) sidewalkProps.push_back(new Lamppost(pos));
                else if (type == 2) 
                {
                    // Align bench with the street
                    // Benches face inwards: side=1 (left side) -> rotate -90, side=-1 (right side) -> rotate +90
                    float benchAngle = dir.angleXZ() + (side == 1 ? -90.0f : 90.0f);
                    sidewalkProps.push_back(new Bench(pos, benchAngle));
                }
                else if (type == 3) sidewalkProps.push_back(new Dustbin(pos));
            }
        }
    }
}

Street::~Street()
{
    for (auto prop : sidewalkProps)
    {
        delete prop;
    }
    sidewalkProps.clear();
}

void Street::draw()
{
    Driveable::draw();
    
    // Draw procedural props
    for (auto prop : sidewalkProps)
    {
        prop->drawObject();
    }
}
