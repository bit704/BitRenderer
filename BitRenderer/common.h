/*
 * 工具常量和函数
 */
#ifndef COMMON_H
#define COMMON_H

#include <cmath>
#include <limits>
#include <sstream>

const double kInfinitDouble = std::numeric_limits<double>::infinity();
const double kPI = 3.1415926535897932385;
const double kEpsilon = 1e-8; // 比较浮点数的阈值

inline double degrees_to_radians(double degrees)
{
    return degrees * kPI / 180.0;
}

inline double random_double()
{
    return rand() / (RAND_MAX + 1.0); // [0,1)
}

inline double random_double(double min, double max)
{
    return min + (max - min) * random_double(); // [min,max)
}

inline int random_int(int min, int max)
{
    return static_cast<int>(random_double(min, max + 1));
}

// 给输出数字加入千位分隔符
inline std::string format_num(long long num)
{
    static std::ostringstream oss;
    oss.str("");
    oss.imbue(std::locale("en_US.UTF-8"));
    oss << num;
    return oss.str();
}


#endif // !COMMON_H