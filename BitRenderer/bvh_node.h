/*
 * BVH树节点类
 */
#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable.h"
#include "hittable_list.h"

class BVHNode : public Hittable
{
public:

    BVHNode(const HittableList& list) 
        : BVHNode(list.get_objects(), 0, list.get_objects().size()) {}

    BVHNode(const std::vector<std::shared_ptr<Hittable>>& src_objects, size_t start, size_t end)
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

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override
    {
        if (!bbox_.hit(r, ray_t))
            return false;

        bool hit_left = left_->hit(r, ray_t, rec);
        // 若击中左节点，更新击中时光线位置t为光线的最大击中距离，再算击中右节点
        bool hit_right = right_->hit(r, Interval(ray_t.get_min(), hit_left ? rec.t : ray_t.get_max()), rec);

        return hit_left || hit_right;
    }

    AABB get_bbox() const override { return bbox_; }

private:

    std::shared_ptr<Hittable> left_;
    std::shared_ptr<Hittable> right_;
    AABB bbox_;

    static bool box_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, 
        int axis_index)
    {
        return a->get_bbox().axis(axis_index).get_min() < b->get_bbox().axis(axis_index).get_min();
    }

    static bool box_x_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
    {
        return box_compare(a, b, 2);
    }
};

#endif // !BVH_NODE_H

