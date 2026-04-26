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
    Bench(Vec3 position);
private:
    void draw();
    void update(const float delta) {}
    float angle;  // rotation
};

#endif // ENVIRONMENT_H
