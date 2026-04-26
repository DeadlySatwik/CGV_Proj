///   File: Vehicle.cpp

#include "Vehicle.h"
#include "Road.h"
#include "Simulator.h"

class Driveable;

const Vec3 Vehicle::blinkerColor = Vec3(1, 0.647, 0);
int Vehicle::bridgeAnomalyCount = 0;

int Vehicle::getBridgeAnomalyCount()
{
    return bridgeAnomalyCount;
}

Vehicle::Vehicle(Driveable *spawnRoad)
{
    initRandValues();

    velocity = 0;
    specs.vehicleLength = 0.2;
    xPos = 0;

    isBraking = false;
    collisionLayer = 0;

    dstToCross = 1000;
    direction = true;
    desiredTurn = 0;

    blinker.init();

    initPointers(spawnRoad);

    rot = Vec3(0, curRoad->getDirection().angleXZ(), 0);

    color = Colors::getRandomColor();
    color *= 0.70f;
}

void Vehicle::initRandValues()
{
    specs.maxV = randFloat(1.2, 1.8);
    specs.minV = randFloat(0.02, 0.08);
    specs.cornerVelocity = 1.4;
    specs.stopTime = randFloat(0.4, 0.7);
    specs.acceleration = randFloat(0.15, 0.25);
    specs.remainDst = randFloat(0.06, 0.08);
}

void Vehicle::Blinker::init()
{
    which = 0;
    time = 0;
    duration = randFloat(0.45, 0.55);
    isLighting = true;
}

void Vehicle::initPointers(Driveable *spawnRoad)
{
    curRoad = spawnRoad;
    curCross = nullptr;
    nextRoad = nullptr;

    backVeh = nullptr;

    allowedToCross = false;
    crossState.isChanging = false;
    crossState.didReachCross = false;
    crossState.isLeavingRoad = false;
    crossState.reservedSpaceAmount = 0.0f;

    frontVeh = NULL;
    isFirstVeh = true;

    if (curRoad->vehiclesBeg.size() > 0 /*&& !curRoad->vehiclesBeg.back()->isChanging*/)
    {
        isFirstVeh = false;
        frontVeh = curRoad->vehiclesBeg.back();
        frontVeh->backVeh = this;
    }
}

float Vehicle::requiredSpaceForRoad(const Driveable *road) const
{
    float required = specs.vehicleLength + specs.remainDst;
    if (road == nullptr)
        return required;

    if (specs.vehicleLength > 0.55f)
    {
        if (road->isElevated())
            required *= 0.72f;

        if (road->getLength() < required + 0.35f)
        {
            float shortRoadCap = road->getLength() * 0.70f;
            if (required > shortRoadCap)
                required = shortRoadCap;
        }

        if (required < 0.45f)
            required = 0.45f;
    }

    return required;
}

void Vehicle::update(const float delta)
{
    if (!crossState.isChanging && !crossState.didReachCross)
    {
        float prevVelocity = velocity;

        xPos += velocity * delta;

        setVelocity();
        checkVelocity(delta, prevVelocity);
        setNewPos();

        // Register to cross: use shorter distance for 2-way pass-through nodes
        float registerDst = 2.4f;
        if (curCross == nullptr)
        {
            // Peek at the upcoming cross to check if it's 2-way
            Cross *upcomingCross = direction ? curRoad->crossEnd : curRoad->crossBeg;
            if (upcomingCross != nullptr && upcomingCross->streets.size() <= 2)
            {
                // Ensure register distance is larger than the maximum stopping gap
                // so long vehicles (buses) don't halt before they can register!
                registerDst = std::max(1.2f, specs.vehicleLength * 2.0f + 0.5f);
            }
        }

        if (curRoad->getLength() - xPos < registerDst && curCross == nullptr)
        {
            registerToCross();
        }
    }

    if (!crossState.isLeavingRoad && curCross != nullptr && nextRoad != nullptr && allowedToCross /* && nextRoad->freeSpace(direction) > vehicleLength + remainDst*/)
    {
        tryBeAllowedToEnterCross();
    }

    if (crossState.isChanging)
    {
        // Use higher speed for 2-way pass-through nodes
        float crossSpeed = specs.cornerVelocity;
        if (curCross != nullptr && curCross->streets.size() <= 2)
        {
            crossSpeed = specs.maxV * 0.85f; // near full speed
        }

        xPos += crossSpeed * delta;

        float s = xPos / curRoad->getLength();

        if (direction)
        {
            pos = Vec3::lerp(curRoad->getBegJointWidth(direction), curRoad->getEndJointWidth(direction), s);
        }
        else
        {
            pos = Vec3::lerp(curRoad->getEndJointWidth(direction), curRoad->getBegJointWidth(direction), s);
        }

        // Pitch rotation on ramps (tilt vehicle to match slope)
        if (curRoad->isElevated())
        {
            float dH = curRoad->getEndHeight() - curRoad->getBegHeight();
            if (!direction)
                dH = -dH;
            float pitch = atan2(dH, curRoad->getLength()) * 180.0f / 3.14159f;
            rot = Vec3(pitch, curRoad->getDirection().angleXZ() + (direction ? 0 : 180), 0);
        }

        if (s > 1)
        {
            leaveRoad();
        }
    }

    if (crossState.didReachCross)
    {
        float crossSpeed = specs.cornerVelocity;
        if (curCross != nullptr && curCross->streets.size() <= 2)
        {
            crossSpeed = specs.maxV * 0.85f;
        }

        xPos += crossSpeed * delta;

        setCornerPosition();

        if (crossState.crossProgress >= 1)
        {
            enterNewRoad();
        }
    }

    blinker.updateBlinkers(delta);

    // Update collision layer based on current road height
    updateCollisionLayer();
}

void Vehicle::updateCollisionLayer()
{
    if (curRoad != nullptr)
    {
        collisionLayer = curRoad->getCollisionLayer();
    }
}

void Vehicle::setVelocity()
{
    float gap = getDst() - specs.remainDst;
    if (gap < 0.0f)
        gap = 0.0f;

    float safeGap = specs.vehicleLength * 0.8f + specs.remainDst;
    float targetVelocity = specs.maxV;

    // Follow distance control to prevent overlap and clipping under congestion.
    if (gap <= safeGap)
    {
        targetVelocity = 0.0f;
    }
    else
    {
        float followLimit = (gap - safeGap) / std::max(0.12f, specs.stopTime);
        if (followLimit < targetVelocity)
            targetVelocity = followLimit;
    }

    // When approaching a cross without permission, smoothly prepare to stop.
    if (curCross != nullptr && !allowedToCross)
    {
        float approachLimit = std::max(0.0f, dstToCross - 0.25f) * 2.0f;
        if (approachLimit < targetVelocity)
            targetVelocity = approachLimit;
    }

    if (targetVelocity < specs.minV)
        targetVelocity = 0.0f;

    // Ease toward target velocity (single-step integrator).
    float dv = targetVelocity - velocity;
    float maxStep = specs.acceleration * 0.8f;
    if (dv > maxStep)
        dv = maxStep;
    else if (dv < -maxStep)
        dv = -maxStep;

    velocity += dv;
}

void Vehicle::checkVelocity(const float delta, float prevVelocity)
{
    float braking = (velocity - prevVelocity) / delta;

    if (braking < -0.3)
    {
        isBraking = true;
    }
    else
    {
        isBraking = false;
    }

    if (velocity < specs.minV)
    {
        velocity = 0;
        isBraking = true;
    }

    if (velocity > specs.maxV)
    {
        velocity = specs.maxV;
    }
}

float Vehicle::getXPos() const
{
    return xPos;
}

float Vehicle::getDstToCross() const
{
    return dstToCross;
}

void Vehicle::setNewPos()
{

    if (xPos > curRoad->getLength())
    {
        xPos = curRoad->getLength();
    }

    float s = xPos / curRoad->getLength();

    if (s > 1)
    {
        s = 1;
    }

    if (direction)
    {
        pos = Vec3::lerp(curRoad->getBegJointWidth(direction), curRoad->getEndJointWidth(direction), s);
    }
    else
    {
        pos = Vec3::lerp(curRoad->getEndJointWidth(direction), curRoad->getBegJointWidth(direction), s);
    }

    dstToCross = curRoad->getLength() - xPos;

    // Bridge/elevation safety: clamp drift to current road height profile.
    float heightT = direction ? s : (1.0f - s);
    float expectedY = curRoad->getHeightAt(heightT);
    if (fabs(pos.y - expectedY) > 0.12f)
    {
        pos.y = expectedY;
        bridgeAnomalyCount++;
    }

    // Pitch rotation on ramps (tilt vehicle to match slope)
    if (curRoad->isElevated())
    {
        float dH = curRoad->getEndHeight() - curRoad->getBegHeight();
        if (!direction)
            dH = -dH;
        float pitch = atan2(dH, curRoad->getLength()) * 180.0f / 3.14159f;
        rot = Vec3(pitch, curRoad->getDirection().angleXZ() + (direction ? 0 : 180), 0);
    }
    else
    {
        rot.x = 0;
    }
}

void Vehicle::registerToCross()
{
    allowedToCross = false;

    if (direction)
    {
        curCross = curRoad->crossEnd;
    }
    else
    {
        curCross = curRoad->crossBeg;
    }

    blinker.which = 0;

    if (curCross != nullptr)
    {
        for (unsigned int i = 0; i < curCross->streets.size(); i++)
        {
            if (curCross->streets[i].street == curRoad)
            {
                desiredTurn = randInt(0, curCross->streets.size() - 1);
                if (desiredTurn == (int)i)
                    desiredTurn = (desiredTurn + 1) % curCross->streets.size();

                if (curCross->streets.size() == 2)
                    desiredTurn = (i + 1) % 2;

                nextRoad = curCross->streets[desiredTurn].street;

                crossState.begRot = curRoad->direction.angleXZ();
                crossState.endRot = nextRoad->direction.angleXZ();

                if (!direction)
                    crossState.begRot += 180;
                if (!curCross->streets[desiredTurn].direction)
                    crossState.endRot += 180;

                blinker.which = rotateDirection(crossState.begRot, crossState.endRot);
                if (curCross->streets.size() == 2)
                    blinker.which = 0;

                blinker.time = 0;
                blinker.isLighting = true;

                curCross->streets[i].vehicles.push_back(this);

                break;
            }
        }
    }
}

void Vehicle::tryBeAllowedToEnterCross()
{
    for (auto &street : curCross->streets)
    {
        if (street.street == nextRoad)
        {
            // The vehicle has already been granted permission by the cross!
            // It MUST proceed to prevent permanently blocking the intersection state machine.
            crossState.isLeavingRoad = true;

            const float requiredSpace = requiredSpaceForRoad(nextRoad);
            float immediateSpace = nextRoad->freeSpace(street.direction);

            float reserveAmount = requiredSpace;
            if (immediateSpace < reserveAmount)
            {
                reserveAmount = std::max(0.12f, immediateSpace - 0.02f);
            }

            crossState.reservedSpaceAmount = reserveAmount;

            if (street.direction)
            {
                nextRoad->reservedSpaceBeg += reserveAmount;
            }
            else
            {
                nextRoad->reservedSpaceEnd += reserveAmount;
            }

            crossState.isChanging = true;
            crossState.didReachCross = false;
            break;
        }
    }
}

void Vehicle::leaveRoad()
{
    xPos = 0;
    crossState.didReachCross = true;
    crossState.isChanging = false;

    if (curCross->streets[desiredTurn].direction)
    {
        nextRoadJoint = nextRoad->getBegJointWidth(curCross->streets[desiredTurn].direction);
    }
    else
    {
        nextRoadJoint = nextRoad->getEndJointWidth(curCross->streets[desiredTurn].direction);
    }

    if (backVeh != nullptr)
    {
        backVeh->isFirstVeh = true;
        backVeh->frontVeh = nullptr;
    }

    crossState.begRot = curRoad->direction.angleXZ();
    crossState.endRot = nextRoad->direction.angleXZ();

    if (!direction)
        crossState.begRot += 180;
    if (!curCross->streets[desiredTurn].direction)
        crossState.endRot += 180;
}

void Vehicle::setCornerPosition()
{
    float tempLength;
    float s;

    if (direction)
    {
        tempLength = Vec3::length(curRoad->getEndJointWidth(direction) - nextRoadJoint);
        s = xPos / tempLength;

        if (s > 1)
            s = 1;

        pos = Vec3::lerp(curRoad->getEndJointWidth(direction), nextRoadJoint, s);
    }
    else
    {
        tempLength = Vec3::length(curRoad->getBegJointWidth(direction) - nextRoadJoint);
        s = xPos / tempLength;

        if (s > 1)
            s = 1;

        pos = Vec3::lerp(curRoad->getBegJointWidth(direction), nextRoadJoint, s);
    }

    rot = Vec3(0, lerpAngle(crossState.begRot, crossState.endRot, s), 0);
    crossState.crossProgress = s;
}

void Vehicle::enterNewRoad()
{
    blinker.which = 0;

    if (backVeh != nullptr)
    {
        backVeh->isFirstVeh = true;
        backVeh->frontVeh = nullptr;
    }

    if (direction)
    {
        curRoad->vehiclesBeg.pop();
    }
    else
    {
        curRoad->vehiclesEnd.pop();
    }

    xPos = 0;
    isBraking = false;

    allowedToCross = false;

    crossState.isChanging = false;
    crossState.didReachCross = false;
    crossState.isLeavingRoad = false;

    velocity = specs.cornerVelocity;

    // Preserve momentum through 2-way pass-through nodes
    if (curCross != nullptr && curCross->streets.size() <= 2)
    {
        velocity = specs.maxV * 0.8f;
    }

    curRoad = curCross->streets[desiredTurn].street;

    direction = curCross->streets[desiredTurn].direction;

    // dstToCross = curCross->getLength();

    float reserved = crossState.reservedSpaceAmount;
    if (reserved <= 0.0f)
        reserved = requiredSpaceForRoad(nextRoad);

    if (direction)
    {
        nextRoad->reservedSpaceBeg -= reserved;
    }
    else
    {
        nextRoad->reservedSpaceEnd -= reserved;
    }
    crossState.reservedSpaceAmount = 0.0f;

    curCross->allowedVeh--;
    desiredTurn = 0;

    backVeh = nullptr;

    frontVeh = nullptr;
    isFirstVeh = true;

    if (direction)
    {
        if (curRoad->vehiclesBeg.size() > 0)
        {
            isFirstVeh = false;
            frontVeh = curRoad->vehiclesBeg.back();
            frontVeh->backVeh = this;
        }
    }
    else
    {
        if (curRoad->vehiclesEnd.size() > 0)
        {
            isFirstVeh = false;
            frontVeh = curRoad->vehiclesEnd.back();
            frontVeh->backVeh = this;
        }
    }

    if (direction)
    {
        curRoad->vehiclesBeg.push(this);
    }
    else
    {
        curRoad->vehiclesEnd.push(this);
    }

    nextRoad = nullptr;
    curCross = nullptr;
}

void Vehicle::Blinker::updateBlinkers(const float delta)
{
    time += delta;

    if (time > duration)
    {
        time = 0;
        isLighting = !isLighting;
    }
}

float Vehicle::getDst() const
{
    if (frontVeh != nullptr)
        return frontVeh->xPos - xPos - frontVeh->specs.vehicleLength / 2.0;

    return curRoad->getLength() - xPos;
}

bool Vehicle::isEnoughSpace() const
{
    if (nextRoad == nullptr || curCross == nullptr || desiredTurn >= (int)curCross->streets.size())
        return false;

    const bool enterDir = curCross->streets[desiredTurn].direction;
    const float requiredSpace = requiredSpaceForRoad(nextRoad);
    const float immediateSpace = nextRoad->freeSpace(enterDir);

    if (immediateSpace > requiredSpace)
        return true;

    // Bridge connectors can be short; allow look-ahead through simple 2-way pass-through nodes.
    // This prevents long vehicles from deadlocking at ramp transitions.
    if (nextRoad->getLength() < requiredSpace + 0.35f)
    {
        Cross *exitCross = enterDir ? nextRoad->crossEnd : nextRoad->crossBeg;
        if (exitCross != nullptr && exitCross->streets.size() <= 2)
        {
            float downstreamSpace = 0.0f;
            for (const auto &street : exitCross->streets)
            {
                if (street.street == nextRoad)
                    continue;

                float candidate = street.street->freeSpace(street.direction);
                if (candidate > downstreamSpace)
                    downstreamSpace = candidate;
            }

            if (immediateSpace + std::max(0.0f, downstreamSpace) > requiredSpace)
                return true;
        }
    }

    return false;
}

Car::Car(Driveable *spawnRoad) : Vehicle(spawnRoad)
{
    velocity = randFloat(2, 5);
}

void Car::update(float delta)
{
    Vehicle::update(delta);
}

void Car::draw()
{
    translate(0, -0.02, 0);

    // ========== BLINKERS (preserved logic) ==========
    if (blinker.which < 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0, 0.055, -0.052);
        setColor(blinkerColor);
        drawCube(0.22, 0.02, 0.008);
        popMatrix();
    }
    if (blinker.which > 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0, 0.055, 0.052);
        setColor(blinkerColor);
        drawCube(0.22, 0.02, 0.008);
        popMatrix();
    }

    // ========== BRAKE LIGHT GLOW ==========
    if (isBraking)
    {
        setColor(1, 0, 0);
        pushMatrix();
        translate(-0.05, 0.085, 0);
        drawCube(0.07, 0.003, 0.04);
        popMatrix();
        pushMatrix();
        translate(-0.05, 0.055, 0.042);
        drawCube(0.12, 0.008, 0.008);
        popMatrix();
        pushMatrix();
        translate(-0.05, 0.055, -0.042);
        drawCube(0.12, 0.008, 0.008);
        popMatrix();
    }

    // ========== WHEELS WITH RIMS ==========
    for (int lr = -1; lr <= 1; lr += 2) // left/right
    {
        for (int fb = -1; fb <= 1; fb += 2) // front/back
        {
            float wx = fb * 0.065f;
            float wz = lr * 0.052f;
            // Tire (dark rubber)
            setColor(0.10f, 0.10f, 0.10f);
            pushMatrix();
            translate(wx, 0.016f, wz);
            drawCube(0.032f, 0.032f, 0.016f);
            popMatrix();
            // Rim (silver hub)
            setColor(0.65f, 0.65f, 0.68f);
            pushMatrix();
            translate(wx, 0.016f, wz + lr * 0.009f);
            drawCube(0.018f, 0.018f, 0.003f);
            popMatrix();
        }
    }

    // ========== UNDERCARRIAGE (dark shadow strip) ==========
    setColor(0.08f, 0.08f, 0.08f);
    pushMatrix();
    translate(0, 0.008f, 0);
    drawCube(0.18f, 0.008f, 0.08f);
    popMatrix();

    // ========== MAIN BODY ==========
    setColor(color);
    pushMatrix();
    translate(0, 0.05f, 0);

    // Lower body (wider, main chassis)
    drawCube(0.21f, 0.05f, 0.105f);

    // ---- Hood (front, slightly lower — slopes down) ----
    setColor(color);
    pushMatrix();
    translate(0.07f, 0.015f, 0);
    drawCube(0.06f, 0.02f, 0.10f);
    popMatrix();

    // ---- Trunk (rear, slightly lower) ----
    pushMatrix();
    translate(-0.07f, 0.012f, 0);
    drawCube(0.06f, 0.018f, 0.10f);
    popMatrix();

    // ---- Side skirts (thin dark strips along bottom edge) ----
    setColor(color * 0.5f);
    pushMatrix();
    translate(0, -0.022f, 0.053f);
    drawCube(0.19f, 0.006f, 0.004f);
    popMatrix();
    pushMatrix();
    translate(0, -0.022f, -0.053f);
    drawCube(0.19f, 0.006f, 0.004f);
    popMatrix();

    // ========== CABIN / ROOF ==========
    drawRoof();

    popMatrix();

    // ========== FRONT GRILLE ==========
    setColor(0.18f, 0.18f, 0.20f);
    pushMatrix();
    translate(0.107f, 0.042f, 0);
    drawCube(0.004f, 0.025f, 0.06f);
    popMatrix();
    // Chrome grille accent
    setColor(0.70f, 0.70f, 0.72f);
    pushMatrix();
    translate(0.108f, 0.048f, 0);
    drawCube(0.002f, 0.008f, 0.055f);
    popMatrix();
    pushMatrix();
    translate(0.108f, 0.038f, 0);
    drawCube(0.002f, 0.008f, 0.055f);
    popMatrix();

    // ========== HEADLIGHTS (day-phase aware) ==========
    int carPhase = Simulator::getInstance().getDayPhase();
    bool nightDriving = (carPhase == 0 || carPhase == 1 || carPhase == 5 || carPhase == 6);

    // Housing
    setColor(0.80f, 0.80f, 0.82f);
    pushMatrix();
    translate(0.107f, 0.05f, 0.038f);
    drawCube(0.006f, 0.018f, 0.022f);
    popMatrix();
    pushMatrix();
    translate(0.107f, 0.05f, -0.038f);
    drawCube(0.006f, 0.018f, 0.022f);
    popMatrix();
    // Lens
    if (nightDriving)
        setColor(1.0f, 0.98f, 0.85f); // bright ON
    else
        setColor(0.95f, 0.93f, 0.80f); // dim OFF
    pushMatrix();
    translate(0.109f, 0.05f, 0.038f);
    drawCube(0.003f, 0.014f, 0.018f);
    popMatrix();
    pushMatrix();
    translate(0.109f, 0.05f, -0.038f);
    drawCube(0.003f, 0.014f, 0.018f);
    popMatrix();

    // Headlight beam (only at night — subtle forward projection)
    if (nightDriving)
    {
        setColor(0.90f, 0.85f, 0.60f);
        pushMatrix();
        translate(0.14f, 0.045f, 0.038f);
        drawCube(0.05f, 0.008f, 0.015f);
        popMatrix();
        pushMatrix();
        translate(0.14f, 0.045f, -0.038f);
        drawCube(0.05f, 0.008f, 0.015f);
        popMatrix();
    }

    // ========== FOG LIGHTS ==========
    if (nightDriving)
        setColor(0.95f, 0.90f, 0.70f);
    else
        setColor(0.90f, 0.88f, 0.75f);
    pushMatrix();
    translate(0.108f, 0.032f, 0.035f);
    drawCube(0.003f, 0.006f, 0.008f);
    popMatrix();
    pushMatrix();
    translate(0.108f, 0.032f, -0.035f);
    drawCube(0.003f, 0.006f, 0.008f);
    popMatrix();

    // ========== TAILLIGHTS (brighter at night) ==========
    if (nightDriving)
        setColor(0.80f, 0.05f, 0.05f);
    else
        setColor(0.55f, 0.02f, 0.02f);
    pushMatrix();
    translate(-0.107f, 0.05f, 0.038f);
    drawCube(0.005f, 0.018f, 0.018f);
    popMatrix();
    pushMatrix();
    translate(-0.107f, 0.05f, -0.038f);
    drawCube(0.005f, 0.018f, 0.018f);
    popMatrix();
    // Amber strip
    setColor(0.85f, 0.45f, 0.05f);
    pushMatrix();
    translate(-0.107f, 0.055f, 0.030f);
    drawCube(0.004f, 0.005f, 0.006f);
    popMatrix();
    pushMatrix();
    translate(-0.107f, 0.055f, -0.030f);
    drawCube(0.004f, 0.005f, 0.006f);
    popMatrix();

    // ========== LICENSE PLATE (rear) ==========
    setColor(0.90f, 0.90f, 0.85f);
    pushMatrix();
    translate(-0.108f, 0.035f, 0);
    drawCube(0.003f, 0.012f, 0.03f);
    popMatrix();

    // ========== BUMPERS ==========
    setColor(color * 0.55f);
    pushMatrix();
    translate(0.112f, 0.032f, 0);
    drawCube(0.006f, 0.022f, 0.095f);
    popMatrix();
    pushMatrix();
    translate(-0.112f, 0.032f, 0);
    drawCube(0.006f, 0.022f, 0.095f);
    popMatrix();

    // ========== SIDE MIRRORS ==========
    setColor(color * 0.85f);
    pushMatrix();
    translate(0.04f, 0.068f, 0.056f);
    drawCube(0.012f, 0.008f, 0.008f);
    popMatrix();
    pushMatrix();
    translate(0.04f, 0.068f, -0.056f);
    drawCube(0.012f, 0.008f, 0.008f);
    popMatrix();
    // Mirror glass
    setColor(0.50f, 0.55f, 0.60f);
    pushMatrix();
    translate(0.04f, 0.068f, 0.061f);
    drawCube(0.008f, 0.006f, 0.002f);
    popMatrix();
    pushMatrix();
    translate(0.04f, 0.068f, -0.061f);
    drawCube(0.008f, 0.006f, 0.002f);
    popMatrix();
}

void Car::drawRoof()
{
    // Trapezoidal cabin with proper windshield angles
    Vec3 a1(0, 0, -0.052f);
    Vec3 a2(0.020f, 0.048f, -0.040f);
    Vec3 a3(0.072f, 0.048f, -0.040f);
    Vec3 a4(0.110f, 0, -0.052f);
    Vec3 a5(0, 0, 0.052f);
    Vec3 a6(0.020f, 0.048f, 0.040f);
    Vec3 a7(0.072f, 0.048f, 0.040f);
    Vec3 a8(0.110f, 0, 0.052f);

    pushMatrix();
    translate(-0.072f, 0.025f, 0);
    beginDraw(QUADS);

    // Roof panel (body color)
    setColor(color);
    drawQuad(a2, a6, a7, a3);

    // Windshield (front, dark tinted)
    setColor(0.12f, 0.20f, 0.28f);
    drawQuad(a4, a8, a7, a3);

    // Rear window (slightly lighter tint)
    setColor(0.15f, 0.22f, 0.30f);
    drawQuad(a1, a2, a6, a5);

    // Side windows (lighter tint, visible interior)
    setColor(0.18f, 0.30f, 0.38f);
    drawQuad(a1, a5, a8, a4); // bottom edge (actually skip this - it's the base)

    // Left side window
    setColor(0.15f, 0.28f, 0.35f);
    drawQuad(a5, a8, a7, a6);

    // Right side window
    drawQuad(a1, a2, a3, a4);

    endDraw();

    // Window trim (thin chrome line around windows)
    setColor(0.65f, 0.65f, 0.68f);
    // Front pillar trim
    pushMatrix();
    translate(0.093f, 0.024f, 0);
    drawCube(0.002f, 0.044f, 0.002f);
    popMatrix();

    popMatrix();
}

Bus::Bus(Driveable *spawnRoad) : Vehicle(spawnRoad)
{
    specs.maxV = randFloat(0.85, 1.15);
    velocity = randFloat(2, 5);

    // Keep visual size, but reduce logical traffic footprint so buses don't deadlock
    // on short bridge connector roads.
    specs.vehicleLength = 0.52;
    specs.remainDst = randFloat(0.09, 0.11);
    specs.stopTime = randFloat(0.28, 0.40);
    specs.cornerVelocity = 1.55f;

    color = Vec3(0.7, 0.7, 0);
}

void Bus::update(const float delta)
{
    Vehicle::update(delta);

    if (crossState.didReachCross)
    {
        float s = crossState.crossProgress * 2.0 - 1.0;
        s = 1 - abs(s);
        float a = Vec3::angleDiff(crossState.begRot, crossState.endRot);
        a /= 4.0;

        busAngle = lerpAngle(0, a, s);
    }
    else
    {
        busAngle = 0;
    }
}

void Bus::draw()
{
    pushMatrix();
    translate(0, 0.085f, 0);

    // Day phase for night effects
    int busPhase = Simulator::getInstance().getDayPhase();
    bool busNight = (busPhase == 0 || busPhase == 1 || busPhase == 5 || busPhase == 6);
    // Window glass color: warm interior at night, dark tint at day
    Vec3 busGlassColor = busNight ? Vec3(0.65f, 0.55f, 0.35f) : Vec3(0.12f, 0.22f, 0.30f);

    // ========== WHEELS WITH RIMS (8 wheels: dual rear axle) ==========
    for (int lr = -1; lr <= 1; lr += 2)
    {
        // Front axle
        setColor(0.08f, 0.08f, 0.08f);
        pushMatrix();
        translate(0.28f, -0.065f, lr * 0.075f);
        drawCube(0.045f, 0.045f, 0.016f);
        popMatrix();
        setColor(0.55f, 0.55f, 0.58f);
        pushMatrix();
        translate(0.28f, -0.065f, lr * 0.084f);
        drawCube(0.025f, 0.025f, 0.003f);
        popMatrix();

        // Middle axle
        setColor(0.08f, 0.08f, 0.08f);
        pushMatrix();
        translate(0.0f, -0.065f, lr * 0.075f);
        drawCube(0.045f, 0.045f, 0.016f);
        popMatrix();
        setColor(0.55f, 0.55f, 0.58f);
        pushMatrix();
        translate(0.0f, -0.065f, lr * 0.084f);
        drawCube(0.025f, 0.025f, 0.003f);
        popMatrix();

        // Rear axle (dual)
        setColor(0.08f, 0.08f, 0.08f);
        pushMatrix();
        translate(-0.28f, -0.065f, lr * 0.075f);
        drawCube(0.045f, 0.045f, 0.016f);
        popMatrix();
        pushMatrix();
        translate(-0.22f, -0.065f, lr * 0.075f);
        drawCube(0.045f, 0.045f, 0.016f);
        popMatrix();
        setColor(0.55f, 0.55f, 0.58f);
        pushMatrix();
        translate(-0.28f, -0.065f, lr * 0.084f);
        drawCube(0.025f, 0.025f, 0.003f);
        popMatrix();
    }

    // ========== UNDERCARRIAGE ==========
    setColor(0.06f, 0.06f, 0.06f);
    pushMatrix();
    translate(0, -0.05f, 0);
    drawCube(0.65f, 0.015f, 0.10f);
    popMatrix();

    // ========== REAR SEGMENT ==========
    pushMatrix();
    rotateY(-busAngle / 1.3f);
    translate(-0.2f, 0, 0);

    // Body shell
    setColor(color);
    drawCube(0.32f, 0.16f, 0.145f);

    // Floor line (dark strip)
    setColor(color * 0.4f);
    pushMatrix();
    translate(0, -0.06f, 0);
    drawCube(0.33f, 0.015f, 0.146f);
    popMatrix();

    // Roof strip (lighter accent)
    setColor(color * 1.15f);
    pushMatrix();
    translate(0, 0.075f, 0);
    drawCube(0.33f, 0.01f, 0.146f);
    popMatrix();

    // Individual windows (rear segment — 4 windows per side)
    for (int side = -1; side <= 1; side += 2)
    {
        float wz = side * 0.074f;
        for (int w = 0; w < 4; w++)
        {
            float wx = -0.12f + w * 0.07f;
            // Frame
            setColor(0.55f, 0.55f, 0.58f);
            pushMatrix();
            translate(wx, 0.025f, wz);
            drawCube(0.055f, 0.07f, 0.002f);
            popMatrix();
            // Glass
            setColor(busGlassColor);
            pushMatrix();
            translate(wx, 0.025f, wz + side * 0.001f);
            drawCube(0.048f, 0.06f, 0.001f);
            popMatrix();
        }
    }

    // Rear emergency door
    setColor(0.20f, 0.20f, 0.22f);
    pushMatrix();
    translate(-0.161f, -0.01f, 0);
    drawCube(0.003f, 0.12f, 0.08f);
    popMatrix();
    // Emergency door window
    setColor(0.15f, 0.25f, 0.32f);
    pushMatrix();
    translate(-0.162f, 0.02f, 0);
    drawCube(0.002f, 0.05f, 0.05f);
    popMatrix();

    // Rear taillights
    setColor(0.60f, 0.02f, 0.02f);
    pushMatrix();
    translate(-0.162f, 0.0f, 0.055f);
    drawCube(0.003f, 0.04f, 0.015f);
    popMatrix();
    pushMatrix();
    translate(-0.162f, 0.0f, -0.055f);
    drawCube(0.003f, 0.04f, 0.015f);
    popMatrix();

    popMatrix();

    // ========== FRONT SEGMENT ==========
    pushMatrix();
    rotateY(busAngle / 4.0f);
    translate(0.2f, 0, 0);

    // Body shell
    setColor(color);
    drawCube(0.32f, 0.16f, 0.145f);

    // Floor line
    setColor(color * 0.4f);
    pushMatrix();
    translate(0, -0.06f, 0);
    drawCube(0.33f, 0.015f, 0.146f);
    popMatrix();

    // Roof strip
    setColor(color * 1.15f);
    pushMatrix();
    translate(0, 0.075f, 0);
    drawCube(0.33f, 0.01f, 0.146f);
    popMatrix();

    // Individual windows (front segment — 4 windows per side)
    for (int side = -1; side <= 1; side += 2)
    {
        float wz = side * 0.074f;
        for (int w = 0; w < 4; w++)
        {
            float wx = -0.10f + w * 0.07f;
            setColor(0.55f, 0.55f, 0.58f);
            pushMatrix();
            translate(wx, 0.025f, wz);
            drawCube(0.055f, 0.07f, 0.002f);
            popMatrix();
            setColor(busGlassColor);
            pushMatrix();
            translate(wx, 0.025f, wz + side * 0.001f);
            drawCube(0.048f, 0.06f, 0.001f);
            popMatrix();
        }
    }

    // Large windshield
    if (busNight)
        setColor(0.55f, 0.48f, 0.30f); // warm interior visible
    else
        setColor(0.10f, 0.18f, 0.25f);
    pushMatrix();
    translate(0.161f, 0.02f, 0);
    drawCube(0.003f, 0.10f, 0.12f);
    popMatrix();
    // Windshield frame
    setColor(0.20f, 0.20f, 0.22f);
    pushMatrix();
    translate(0.160f, 0.02f, 0);
    drawCube(0.002f, 0.11f, 0.13f);
    popMatrix();

    // ---- Destination LED display (orange on black) ----
    setColor(0.08f, 0.08f, 0.08f);
    pushMatrix();
    translate(0.162f, 0.06f, 0);
    drawCube(0.003f, 0.025f, 0.10f);
    popMatrix();
    setColor(0.95f, 0.60f, 0.08f);
    pushMatrix();
    translate(0.163f, 0.06f, 0);
    drawCube(0.002f, 0.018f, 0.08f);
    popMatrix();

    // ---- Route number (side display) ----
    setColor(0.08f, 0.08f, 0.08f);
    pushMatrix();
    translate(0.12f, 0.06f, 0.074f);
    drawCube(0.04f, 0.02f, 0.002f);
    popMatrix();
    setColor(0.90f, 0.55f, 0.05f);
    pushMatrix();
    translate(0.12f, 0.06f, 0.075f);
    drawCube(0.035f, 0.015f, 0.001f);
    popMatrix();

    // ---- Front door (passenger entry) ----
    setColor(0.15f, 0.15f, 0.18f);
    pushMatrix();
    translate(0.10f, -0.01f, 0.074f);
    drawCube(0.06f, 0.12f, 0.002f);
    popMatrix();
    setColor(0.12f, 0.22f, 0.30f);
    pushMatrix();
    translate(0.10f, 0.02f, 0.075f);
    drawCube(0.04f, 0.06f, 0.001f);
    popMatrix();

    // Headlights
    setColor(0.82f, 0.82f, 0.84f);
    pushMatrix();
    translate(0.162f, -0.02f, 0.055f);
    drawCube(0.004f, 0.025f, 0.025f);
    popMatrix();
    pushMatrix();
    translate(0.162f, -0.02f, -0.055f);
    drawCube(0.004f, 0.025f, 0.025f);
    popMatrix();
    if (busNight)
        setColor(1.0f, 0.98f, 0.85f); // bright
    else
        setColor(0.95f, 0.93f, 0.80f);
    pushMatrix();
    translate(0.164f, -0.02f, 0.055f);
    drawCube(0.002f, 0.020f, 0.020f);
    popMatrix();
    pushMatrix();
    translate(0.164f, -0.02f, -0.055f);
    drawCube(0.002f, 0.020f, 0.020f);
    popMatrix();

    // Headlight beams at night
    if (busNight)
    {
        setColor(0.85f, 0.80f, 0.55f);
        pushMatrix();
        translate(0.20f, -0.025f, 0.055f);
        drawCube(0.06f, 0.010f, 0.018f);
        popMatrix();
        pushMatrix();
        translate(0.20f, -0.025f, -0.055f);
        drawCube(0.06f, 0.010f, 0.018f);
        popMatrix();
    }

    // Front bumper
    setColor(0.20f, 0.20f, 0.22f);
    pushMatrix();
    translate(0.165f, -0.05f, 0);
    drawCube(0.008f, 0.03f, 0.13f);
    popMatrix();

    // Side mirrors
    setColor(color * 0.8f);
    pushMatrix();
    translate(0.14f, 0.03f, 0.08f);
    drawCube(0.015f, 0.01f, 0.012f);
    popMatrix();
    pushMatrix();
    translate(0.14f, 0.03f, -0.08f);
    drawCube(0.015f, 0.01f, 0.012f);
    popMatrix();

    popMatrix();

    // ========== ARTICULATION CONNECTOR (rubber bellows) ==========
    setColor(0.25f, 0.25f, 0.20f);
    drawCube(0.16f, 0.14f, 0.13f);
    // Bellows folds (ribbed texture)
    setColor(0.20f, 0.20f, 0.18f);
    for (int r = -2; r <= 2; r++)
    {
        pushMatrix();
        translate(r * 0.025f, 0, 0);
        drawCube(0.005f, 0.13f, 0.132f);
        popMatrix();
    }

    // ========== MARKER LIGHTS (roofline) ==========
    setColor(0.90f, 0.60f, 0.10f);
    pushMatrix();
    rotateY(busAngle / 4.0f);
    translate(0.2f, 0.082f, 0.065f);
    drawCube(0.28f, 0.005f, 0.005f);
    popMatrix();
    pushMatrix();
    rotateY(busAngle / 4.0f);
    translate(0.2f, 0.082f, -0.065f);
    drawCube(0.28f, 0.005f, 0.005f);
    popMatrix();

    // ========== BLINKERS ==========
    pushMatrix();
    if (blinker.which < 0 && blinker.isLighting)
    {
        setColor(blinkerColor);
        translate(0, -0.035f, -0.06f);
        drawCube(0.73f, 0.008f, 0.008f);
    }
    if (blinker.which > 0 && blinker.isLighting)
    {
        setColor(blinkerColor);
        translate(0, -0.035f, 0.06f);
        drawCube(0.73f, 0.008f, 0.008f);
    }
    popMatrix();

    // ========== BRAKE LIGHTS ==========
    if (isBraking)
    {
        pushMatrix();
        setColor(1, 0, 0);
        rotateY(-busAngle / 1.3f);

        pushMatrix();
        translate(-0.35f, 0.05f, 0);
        drawCube(0.12f, 0.003f, 0.06f);
        popMatrix();
        pushMatrix();
        translate(-0.35f, -0.02f, 0.055f);
        drawCube(0.12f, 0.008f, 0.008f);
        popMatrix();
        pushMatrix();
        translate(-0.35f, -0.02f, -0.055f);
        drawCube(0.12f, 0.008f, 0.008f);
        popMatrix();

        popMatrix();
    }

    popMatrix();
}

Bike::Bike(Driveable *spawnRoad) : Vehicle(spawnRoad)
{
    velocity = randFloat(2.5, 5.5);
    specs.maxV = randFloat(1.5, 2.2);
    specs.minV = randFloat(0.04, 0.1);
    specs.cornerVelocity = 1.6;
    specs.stopTime = randFloat(0.2, 0.4);
    specs.acceleration = randFloat(0.3, 0.5);
    specs.vehicleLength = 0.1;
    specs.remainDst = randFloat(0.04, 0.06);
}

void Bike::update(float delta)
{
    Vehicle::update(delta);
}

void Bike::draw()
{
    translate(0, -0.02, 0);

    // ========== BLINKERS ==========
    if (blinker.which < 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0, 0.04, -0.015);
        setColor(blinkerColor);
        drawCube(0.06, 0.01, 0.005);
        popMatrix();
    }
    if (blinker.which > 0 && blinker.isLighting)
    {
        pushMatrix();
        translate(0, 0.04, 0.015);
        setColor(blinkerColor);
        drawCube(0.06, 0.01, 0.005);
        popMatrix();
    }

    // BRAKE LIGHT
    if (isBraking)
    {
        setColor(1, 0, 0);
        pushMatrix();
        translate(-0.04, 0.04, 0);
        drawCube(0.01, 0.01, 0.02);
        popMatrix();
    }

    // WHEELS (2 wheels inline)
    setColor(0.10f, 0.10f, 0.10f);
    pushMatrix();
    translate(0.045f, 0.016f, 0);
    drawCube(0.032f, 0.032f, 0.01f);
    popMatrix();
    pushMatrix();
    translate(-0.045f, 0.016f, 0);
    drawCube(0.032f, 0.032f, 0.01f);
    popMatrix();

    // Rims
    setColor(0.65f, 0.65f, 0.68f);
    pushMatrix();
    translate(0.045f, 0.016f, 0.006f);
    drawCube(0.018f, 0.018f, 0.002f);
    popMatrix();
    pushMatrix();
    translate(0.045f, 0.016f, -0.006f);
    drawCube(0.018f, 0.018f, 0.002f);
    popMatrix();
    pushMatrix();
    translate(-0.045f, 0.016f, 0.006f);
    drawCube(0.018f, 0.018f, 0.002f);
    popMatrix();
    pushMatrix();
    translate(-0.045f, 0.016f, -0.006f);
    drawCube(0.018f, 0.018f, 0.002f);
    popMatrix();

    // BODY
    setColor(color);
    pushMatrix();
    translate(0, 0.035f, 0);
    drawCube(0.08f, 0.03f, 0.02f);
    popMatrix();

    // HANDLEBARS
    setColor(0.1f, 0.1f, 0.1f);
    pushMatrix();
    translate(0.03f, 0.055f, 0);
    drawCube(0.01f, 0.01f, 0.05f);
    popMatrix();

    // HEADLIGHT
    int carPhase = Simulator::getInstance().getDayPhase();
    bool nightDriving = (carPhase == 0 || carPhase == 1 || carPhase == 5 || carPhase == 6);
    if (nightDriving)
        setColor(1.0f, 0.98f, 0.85f);
    else
        setColor(0.95f, 0.93f, 0.80f);
    pushMatrix();
    translate(0.041f, 0.04f, 0);
    drawCube(0.002f, 0.01f, 0.01f);
    popMatrix();

    if (nightDriving)
    {
        setColor(0.90f, 0.85f, 0.60f);
        pushMatrix();
        translate(0.08f, 0.035f, 0);
        drawCube(0.08f, 0.008f, 0.015f);
        popMatrix();
    }
}
