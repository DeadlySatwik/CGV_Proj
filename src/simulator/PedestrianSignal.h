///   File: PedestrianSignal.h
///   Pedestrian crossing signal state machine (WALK / DONT_WALK / FLASHING).

#ifndef PEDESTRIANSIGNAL_H
#define PEDESTRIANSIGNAL_H

class PedestrianSignal
{
public:
    enum SignalState
    {
        DONT_WALK = 0,   // pedestrians must not enter crossing
        WALK      = 1,   // pedestrians may begin crossing
        FLASHING  = 2    // warning: finish crossing, don't start
    };

    PedestrianSignal();

    /// Set cycle durations in seconds
    void setDurations(float walkDur, float flashDur, float dontWalkDur);

    /// Advance the signal by delta seconds
    void update(float delta);

    /// Current signal state
    SignalState getState() const { return currentState; }

    /// True only during WALK (safe for new pedestrians to start)
    bool canPedestrianStartCrossing() const { return currentState == WALK; }

    /// True during WALK or FLASHING (pedestrian has legal right to be in crossing)
    bool pedestrianHasSignalRight() const { return currentState == WALK || currentState == FLASHING; }

    /// Set a phase offset so crossings don't all sync
    void setPhaseOffset(float offset);

    /// Get time remaining in current phase
    float getTimeRemaining() const;

    /// Total cycle duration
    float getCycleDuration() const;

private:
    SignalState currentState;
    float stateTimer;
    float walkDuration;
    float flashDuration;
    float dontWalkDuration;
    float phaseOffset;
    bool offsetApplied;

    void advanceState();
    float getCurrentStateDuration() const;
};

#endif // PEDESTRIANSIGNAL_H
