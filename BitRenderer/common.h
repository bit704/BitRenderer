#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>

const double kInfinitDouble = std::numeric_limits<double>::infinity();
const double kPI = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
    return degrees * kPI / 180.0;
}

#endif // !COMMON_H