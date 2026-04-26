///   File: Environment.h

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "GameObject.h"

class Tree : public GameObject
{
public:
    Tree(Vec3 position);
private:
    void draw();
    void update(const float delta) {}
    float trunkH;
    float canopyR;
    int variant;  // visual variety from hash
};

class Lamppost : public GameObject
{
public:
    Lamppost(Vec3 position);
private:
    void draw();
    void update(const float delta) {}
};

class Bench : public GameObject
{
public:
    Bench(Vec3 position, float rotationAngle = 0.0f);
private:
    void draw();
    void update(const float delta) {}
    float angle;  // rotation
};

class Dustbin : public GameObject
{
public:
    Dustbin(Vec3 position);
private:
    void draw();
    void update(const float delta) {}
};

#endif // ENVIRONMENT_H
