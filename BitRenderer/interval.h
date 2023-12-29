/*
* 间隔类
*/
#ifndef INTERVAL_H
#define INTERVAL_H

#include "common.h"

class Interval 
{
public:

    Interval() : min_(+kInfinitDouble), max_(-kInfinitDouble) {}

    Interval(double _min, double _max) : min_(_min), max_(_max) {}

    // 并集运算
    Interval(const Interval& a, const Interval& b)
        : min_(fmin(a.min_, b.min_)), max_(fmax(a.max_, b.max_)) {}

    bool contains(double x) const 
    {
        return min_ <= x && x <= max_;
    }

    bool surrounds(double x) const
    {
        return min_ < x && x < max_;
    }

    double get_size() const
    {
        return max_ - min_;
    }

    Interval expand(double delta) const
    {
        auto padding = delta / 2;
        return Interval(min_ - padding, max_ + padding);
    }

    double get_min() const
    {
        return min_;
    }

    double get_max() const
    {
        return max_;
    }

    void set_min(double min)
    {
        min_ = min;
    }

    void set_max(double max)
    {
        max_ = max;
    }

private:

    double min_, max_;
};

#endif // !INTERVAL_H
