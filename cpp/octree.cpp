#include <assert.h>
#include <limits>
#include <algorithm>
#include "octree.h"
#include "numerics.h"
#include "vec_ops.h"
#include "util.h"
#include "geometry.h"

namespace tbem {

template <size_t dim>
size_t Octree<dim>::n_immediate_children() const {
    size_t c = 0;
    for (const auto& p: children) {
        if (p != nullptr) {
            c++;
        }
    }
    return c;
}

template <size_t dim>
size_t Octree<dim>::n_children() const {
    size_t c = 0;
    for (const auto& p: children) {
        if (p != nullptr) {
            c += 1 + p->n_children();
        }
    }
    return c;
}

template <size_t dim>
bool Octree<dim>::is_leaf() const {
    for (const auto& p: children) {
        if (p != nullptr) {
            return false;
        }
    }
    return true;
}

template <size_t dim>
size_t Octree<dim>::find_closest_nonempty_child(const Vec<double,dim>& pt) const
{
    size_t closest_child = 0;
    double dist2_to_closest_child = std::numeric_limits<double>::max();
    for (size_t c_idx = 0; c_idx < Octree<dim>::split; c_idx++) {
        auto& c = children[c_idx];
        if (c == nullptr) {
            continue;
        }
        auto dist2_child = dist2(pt, c->bounds.center);
        if (dist2_child < dist2_to_closest_child) {
            closest_child = c_idx;
            dist2_to_closest_child = dist2_child;
        }
    }
    return closest_child;
}

template struct Octree<2>;
template struct Octree<3>;

template <size_t dim>
Vec<size_t,dim> make_child_idx(size_t i) 
{
    Vec<size_t,dim> child_idx;
    for (int d = dim - 1; d >= 0; d--) {
        auto idx = i % 2;
        i = i >> 1;
        child_idx[d] = idx;
    }
    return child_idx;
}
template 
Vec<size_t,2> make_child_idx<2>(size_t i);
template 
Vec<size_t,3> make_child_idx<3>(size_t i);

template <size_t dim>
std::array<std::vector<size_t>,Octree<dim>::split> split_pts_into_children(
    const Box<dim>& bounds, const std::vector<Ball<dim>>& pts, 
    const std::vector<size_t>& indices)
{
    std::array<std::vector<size_t>,Octree<dim>::split> child_indices;
    for (size_t i = 0; i < indices.size(); i++) {
        auto child_idx = bounds.find_containing_subcell(pts[indices[i]].center);
        child_indices[child_idx].push_back(indices[i]);
    }
    return child_indices;
}

template <size_t dim>
typename Octree<dim>::ChildrenType
build_children(const Box<dim>& bounds, 
    const std::vector<Ball<dim>>& pts, const std::vector<size_t>& indices,
    size_t level, size_t min_pts_per_cell, size_t& next_index) 
{
    typename Octree<dim>::ChildrenType children;
    if (indices.size() <= min_pts_per_cell) {
        return std::move(children);
    }

    auto child_indices = split_pts_into_children(bounds, pts, indices);

    auto child_level = level + 1;
#pragma omp parallel for if(level == 0)
    for (size_t i = 0; i < Octree<dim>::split; i++) {
        if (child_indices[i].size() == 0) {
            continue;
        }
        auto idx = make_child_idx<dim>(i);
        auto child_bounds = bounds.get_subcell(idx);

        std::vector<Ball<dim>> tight_pts(child_indices[i].size());
        for (size_t pt_idx = 0; pt_idx < child_indices[i].size(); pt_idx++) {
            tight_pts[pt_idx] = pts[child_indices[i][pt_idx]];
        }
        auto tight_bounds = Box<dim>::bounding_box(tight_pts);

        auto largest_ball = std::max_element(tight_pts.begin(), tight_pts.end(),
            [](const Ball<dim>& a, const Ball<dim>& b) {
                return a.radius < b.radius;
            });

        typename Octree<dim>::ChildrenType sub_children;
        //TODO: Documentation should be precise that min_pts_per_cell is not a 
        //guarantee. Because of this shortcircuiting that prevents impossible
        //infinitely subdivision, the number of pts in a cell could be larger.
        //But, that's unlikely for any reasonable set of points.
        if (any(tight_bounds.half_width != (*largest_ball).radius)) {
            sub_children = build_children(
                child_bounds, pts, child_indices[i],
                child_level, min_pts_per_cell, next_index
            );
        }
        auto expanded_tight_bounds = tight_bounds.expand_by_max_axis_multiple(1e-5);

        size_t child_idx;
#pragma omp critical 
        {
            child_idx = next_index;
            next_index++;
        }
        children[i] = std::unique_ptr<Octree<dim>>(new Octree<dim>{
            child_bounds, expanded_tight_bounds, child_indices[i],
            child_level, child_idx, std::move(sub_children)
        });
    }
    return std::move(children);
}

template <size_t dim>
Octree<dim> make_octree(const std::vector<Ball<dim>>& pts, size_t min_pts_per_cell)
{
    assert(min_pts_per_cell > 0);
    auto box = Box<dim>::bounding_box(pts);
    auto expanded_box = box.expand_by_max_axis_multiple(1e-5);
    auto all_indices = range(pts.size());
    size_t next_index = 1;
    auto children = build_children(
        box, pts, all_indices, 0, min_pts_per_cell, next_index
    );
    // for (size_t i = 0; i < all_indices.size(); i++) {
    //     std::cout << all_indices[i] << std::endl;
    // }
    return Octree<dim>{
        expanded_box, expanded_box, all_indices, 0, 0, std::move(children)
    };
}

template <size_t dim>
Octree<dim> make_octree(const std::vector<Vec<double,dim>>& pts,
    size_t min_pts_per_cell)
{
    auto bs = balls_from_centers_radii(pts, std::vector<double>(pts.size(), 0.0));
    return make_octree(bs, min_pts_per_cell);
}

template 
Octree<2>
make_octree(const std::vector<Ball<2>>& pts, size_t min_pts_per_cell);
template 
Octree<3>
make_octree(const std::vector<Ball<3>>& pts, size_t min_pts_per_cell);
template 
Octree<2>
make_octree(const std::vector<Vec<double,2>>& pts, size_t min_pts_per_cell);
template 
Octree<3>
make_octree(const std::vector<Vec<double,3>>& pts, size_t min_pts_per_cell);

} // END namespace tbem
