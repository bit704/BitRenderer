/*
 * BVH树节点类
 */
#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable_list.h"

class BVHNode : public Hittable
{
private:
    shared_ptr<Hittable> left_, right_;
    AABB bbox_;

public:
    BVHNode() = delete;

    BVHNode(const HittableList& list) 
        : BVHNode(list.get_objects(), 0, list.get_objects().size()) {}

    BVHNode(const std::vector<shared_ptr<Hittable>>& src_objects, size_t start, size_t end)
    {
        int axis = random_int(0, 2);
        auto comparator = (axis == 0) ? box_x_compare
            : (axis == 1) ? box_y_compare
            : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1)
        {
            left_ = right_ = src_objects[start];
        }
        else if (object_span == 2) 
        {
            if (comparator(src_objects[start], src_objects[start + 1]))
            {
                left_ = src_objects[start];
                right_ = src_objects[start + 1];
            }
            else
            {
                left_ = src_objects[start + 1];
                right_ = src_objects[start];
            }
        }
        else 
        {
            auto objects = src_objects;
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span / 2;
            left_ = std::make_shared<BVHNode>(objects, start, mid);
            right_ = std::make_shared<BVHNode>(objects, mid, end);
        }

        bbox_ = AABB(left_->get_bbox(), right_->get_bbox());
    }

    BVHNode(const BVHNode&) = delete;
    BVHNode& operator=(const BVHNode&) = delete;

    BVHNode(BVHNode&&) = delete;
    BVHNode& operator=(BVHNode&&) = delete;

public:
    bool hit(const Ray& r, const Interval& interval, HitRecord& rec) 
        const override
    {
        if (!bbox_.hit(r, interval))
            return false;

        bool hit_left = left_->hit(r, interval, rec);
        // 若击中左节点，更新击中时光线位置t为光线的最大击中距离，再算击中右节点
        bool hit_right = right_->hit(r, Interval(interval.get_min(), hit_left ? rec.t : interval.get_max()), rec);

        return hit_left || hit_right;
    }

    AABB get_bbox() 
        const override
    {
        return bbox_; 
    }

private:
    static bool box_compare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, 
        int axis_index)
    {
        return a->get_bbox().axis(axis_index).get_min() < b->get_bbox().axis(axis_index).get_min();
    }

    static bool box_x_compare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 2);
    }
};

#endif // !BVH_NODE_H

