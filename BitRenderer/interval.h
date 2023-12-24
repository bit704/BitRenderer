/*
* º‰∏Ù¿‡
*/
#ifndef INTERVAL_H
#define INTERVAL_H

#include "common.h"

class Interval 
{
public:

    Interval() : min_(+kInfinitDouble), max_(-kInfinitDouble) {}

    Interval(double _min, double _max) : min_(_min), max_(_max) {}

    bool contains(double x) const 
    {
        return min_ <= x && x <= max_;
    }

    bool surrounds(double x) const
    {
        return min_ < x && x < max_;
    }

    double get_min()
    {
        return min_;
    }

    double get_max()
    {
        return max_;
    }

private:

    double min_, max_;
};

#endif // !INTERVAL_H
