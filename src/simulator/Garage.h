///   File: Garage.h


#ifndef GARAGE_H
#define GARAGE_H

#include "Road.h"
#include "Vehicle.h"

class Garage : public Driveable
{
public:
    bool checkReadyToSpot() const;
    bool checkReadyToDelete() const;

private:
    float frecSpot;
    float curTimeSpot;

    float frecDelete;
    float curTimeDelete;

    void draw();
    void update(const float delta);

    Vehicle* spotVeh();
    Vehicle* deleteVeh();

    bool isReadyToSpot;
    bool isReadyToDelete;

    int spottedVehicles;
    int maxVehicles;

    // ---- Gate animation ----
    enum GateState { GATE_CLOSED, GATE_OPENING, GATE_OPEN, GATE_CLOSING };
    GateState gateStateOut;
    float gateAngleOut;
    float gateTimerOut;

    GateState gateStateIn;
    float gateAngleIn;
    float gateTimerIn;
    static const float GATE_SPEED;    // degrees per second
    static const float GATE_HOLD;     // seconds gate stays open

    friend Simulator;
    friend ObjectsLoader;

protected:
    Garage(Vec3 p, Cross *c);

    std::string itos(const int x);
    virtual Vehicle *createVehicle() = 0;

    static int vehiclesCounter;
};

class GarageCar : public Garage
{
public:
    GarageCar(Vec3 p, Cross *c);

protected:
    Vehicle *createVehicle();
};

class GarageBus : public Garage
{
public:
    GarageBus(Vec3 p, Cross *c);

protected:
    Vehicle *createVehicle();
};

class GarageBike : public Garage
{
public:
    GarageBike(Vec3 p, Cross *c);

protected:
    Vehicle *createVehicle();
};

#endif // GARAGE_H
