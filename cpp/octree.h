#ifndef _OCTREE2_H
#define _OCTREE2_H

#include <array>
#include <vector>
#include <memory>
#include "vec.h"
#include "numbers.h"

namespace tbem { 

/* A box class defined by its center and half_width. These are used as
 * bounding boxes for the nodes in the octree hierarchy.
 */
template <size_t dim>
class Box {
public:
    const Vec<double,dim> center;
    const Vec<double,dim> half_width;
    Box<dim> get_subcell(const Vec<size_t,dim>& idx) const;
    bool in_box(const Vec<double,dim>& pt, const Vec<bool,dim>& inclusive) const;
    bool in_box_inclusive(const Vec<double,dim>& pt) const;

    static Box<dim> bounding_box(const std::vector<Vec<double,dim>>& x);
};

template <size_t dim, typename T>
struct OctreeData {
    static const size_t split = 2<<(dim-1);
    typedef std::array<std::unique_ptr<OctreeData<dim,T>>,split> ChildrenType;
    const T data;
    ChildrenType children;

    size_t count_children() const {
        size_t c = 0;
        for (const auto& p: children) {
            if (p != nullptr) {
                c++;
            }
        }
        return c;
    }
    
    size_t count_children_recursive() const {
        size_t c = 0;
        for (const auto& p: children) {
            if (p != nullptr) {
                c += 1 + p->count_children_recursive();
            }
        }
        return c;
    }

    bool is_leaf() const {
        for (const auto& p: children) {
            if (p != nullptr) {
                return false;
            }
        }
        return true;
    }
};

template <size_t dim>
struct OctreeCell {
    const Box<dim> bounds;
    const Box<dim> tight_bounds;
    const std::vector<int> indices;
    const size_t level;
};

template <size_t dim>
using Octree = OctreeData<dim,OctreeCell<dim>>;

template <size_t dim>
Vec<size_t,dim> make_child_idx(size_t i);

template <size_t dim>
Octree<dim> build_octree(const std::vector<Vec<double,dim>>& pts,
    size_t min_pts_per_cell);

} // END namespace tbem
#endif
