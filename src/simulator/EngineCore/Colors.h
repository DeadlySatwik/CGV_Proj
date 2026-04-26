///   File: Colors.h


#ifndef COLORS_H
#define COLORS_H

#include <cstdlib>
#include "Vec3.h"

class Colors
{
public:
    static Vec3 getRandomColor();

private:
    Vec3 *colors;
    int amount;
    Colors();

    Vec3 pickRandom();
};

#endif // COLORS_H
