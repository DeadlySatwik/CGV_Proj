///   File: PedestrianSignal.cpp
///   Pedestrian crossing signal cycle implementation.

#include "PedestrianSignal.h"

PedestrianSignal::PedestrianSignal()
    : currentState(DONT_WALK),
      stateTimer(0.0f),
      walkDuration(8.0f),
      flashDuration(3.0f),
      dontWalkDuration(12.0f),
      phaseOffset(0.0f),
      offsetApplied(false)
{
}

void PedestrianSignal::setDurations(float walkDur, float flashDur, float dontWalkDur)
{
    walkDuration     = walkDur;
    flashDuration    = flashDur;
    dontWalkDuration = dontWalkDur;
}

void PedestrianSignal::setPhaseOffset(float offset)
{
    phaseOffset = offset;
    offsetApplied = false;
}

float PedestrianSignal::getCurrentStateDuration() const
{
    switch (currentState)
    {
    case WALK:      return walkDuration;
    case FLASHING:  return flashDuration;
    case DONT_WALK: return dontWalkDuration;
    }
    return dontWalkDuration;
}

float PedestrianSignal::getTimeRemaining() const
{
    return getCurrentStateDuration() - stateTimer;
}

float PedestrianSignal::getCycleDuration() const
{
    return walkDuration + flashDuration + dontWalkDuration;
}

void PedestrianSignal::advanceState()
{
    switch (currentState)
    {
    case DONT_WALK:
        currentState = WALK;
        break;
    case WALK:
        currentState = FLASHING;
        break;
    case FLASHING:
        currentState = DONT_WALK;
        break;
    }
    stateTimer = 0.0f;
}

void PedestrianSignal::update(float delta)
{
    // Apply phase offset on first update by fast-forwarding
    if (!offsetApplied && phaseOffset > 0.0f)
    {
        offsetApplied = true;
        float remaining = phaseOffset;
        while (remaining > 0.0f)
        {
            float dur = getCurrentStateDuration() - stateTimer;
            if (remaining >= dur)
            {
                remaining -= dur;
                advanceState();
            }
            else
            {
                stateTimer += remaining;
                remaining = 0.0f;
            }
        }
        return; // Don't double-advance on the first frame
    }

    stateTimer += delta;
    if (stateTimer >= getCurrentStateDuration())
    {
        advanceState();
    }
}
