#include "octree.h"
#include "algs.h"
#include <assert.h>

Box bounding_box(const std::array<std::vector<double>,3>& x, int begin, int end)
{
    assert(begin < end);
    Box ext;
    ext.min_corner = {x[0][begin], x[1][begin], x[2][begin]};
    ext.max_corner = {x[0][begin], x[1][begin], x[2][begin]};
    for (int i = begin + 1; i < end; ++i) {
        for (unsigned int d = 0; d < 3; ++d) {
            ext.min_corner[d] = std::min(ext.min_corner[d], x[d][i]);
            ext.max_corner[d] = std::max(ext.max_corner[d], x[d][i]);
        }
    }

    for (int d = 0; d < 3; ++d) {
        ext.center[d] = (ext.max_corner[d] + ext.min_corner[d]) / 2.0;
        ext.half_width[d] = (ext.max_corner[d] - ext.min_corner[d]) / 2.0;
    }
    /* std::cout << max_corner << " " << min_corner << std::endl; */
    return ext;
}

Octree::Octree(std::array<std::vector<double>,3>& p_elements,
               unsigned int max_elements_per_cell):
    max_elements_per_cell(max_elements_per_cell),
    elements(std::move(p_elements)),
    morton_codes(elements[0].size())
{ 
    bounds = bounding_box(elements, 0, n_elements());

    // 
    // Fudge the width for the box used to calculate morton codes
    // so that no point is exactly on the outer boundary of the box.
    // This allows ignoring the edge case.
    morton_bounds = bounds;
    for (int d = 0; d < 3; d++) {
        morton_bounds.half_width[d] *= 1.001;
    }

    //TODO: To parallelize this, don't build the tree top-down. Build it
    //from the bottom-up. Every element knows its position in the tree already,
    //I just need to compute the morton code on each level to determine that.
    //The problem becomes adaptivity. To achieve adaptivity, I could 
    //temporarily build a hash table mapping cells to a list of elements and 
    //then statically convert that into a top-down tree. But, because the 
    //lower nodes are already constructed, constructing the upper nodes
    //is not dependent on them -- in other words, this construction is
    //completely parallelizable.
    for (unsigned int i = 0; i < n_elements(); i++) {
        morton_codes[i] = compute_morton(elements[0][i],
                                         elements[1][i],
                                         elements[2][i]);
    }
    sort_elements();

    OctreeCell root = {0, {0,0,0}, bounds,
                       0, (unsigned int)morton_codes.size(),
                       compute_morton(bounds.min_corner),
                       compute_morton(bounds.max_corner),
                       {-1, -1, -1, -1, -1, -1, -1, -1},
                       false};
    if (n_elements() > max_elements_per_cell) {
        root.children = build_children(root);
    } else {
        root.is_leaf = true;
    }
    cells.push_back(root);
}

uint64_t Octree::compute_morton(std::array<double,3> pt) {
    return compute_morton(pt[0], pt[1], pt[2]);
}

uint64_t Octree::compute_morton(double ptx, double pty, double ptz) {
    unsigned int x = to_octree_space(ptx, morton_bounds.center[0],
                                     morton_bounds.half_width[0], deepest);
    unsigned int y = to_octree_space(pty, morton_bounds.center[1],
                                     morton_bounds.half_width[1], deepest);
    unsigned int z = to_octree_space(ptz, morton_bounds.center[2],
                                     morton_bounds.half_width[2], deepest);
    return morton_encode(z, y, x);
}

void Octree::sort_elements() {
    permutation = sort_permutation(morton_codes, 
                              [](uint64_t a, uint64_t b) {return a < b;});
    morton_codes = apply_permutation(morton_codes, permutation);
    for (int d = 0; d < 3; d++) {
        elements[d] = apply_permutation(elements[d], permutation);
    }
}

int Octree::build_child(OctreeCell& cur_cell, int i, int j, int k) {
    int child_idx = 4 * i + 2 * j + k;


    //Compute morton code of each corner.
    auto morton_steps = ((cur_cell.max_code - cur_cell.min_code) / 8);
    auto min_code = cur_cell.min_code + child_idx * morton_steps;
    auto max_code = cur_cell.max_code - (7 - child_idx) * morton_steps;
    assert(max_code >= min_code);


    unsigned int begin = std::lower_bound(morton_codes.begin() + cur_cell.begin,
                                morton_codes.begin() + cur_cell.end, min_code)
             - morton_codes.begin();
    unsigned int end = std::upper_bound(morton_codes.begin() + cur_cell.begin,
                               morton_codes.begin() + cur_cell.end, max_code) 
             - morton_codes.begin();
    assert(end >= begin);
    // std::cout << cur_cell.begin << " " << cur_cell.end << " " << begin << " " << end << std::endl;

    // Every cell must have at least two points in it.
    if (end - begin == 0) {
        return -1;
    }

    //TODO: If computing bounding boxes from pts becomes too much work,
    //the current bounding box can be computed from the children's bounding
    //boxes.
    Box box;
    if (end - begin == 1) {
        //TODO: Extract this get_box or get_single_pt_box function or both.
        for (int d = 0; d < 3; d++) {
            box.center[d] = (elements[d][begin] +
                             cur_cell.bounds.center[d]) / 2.0;
            box.half_width[d] = std::fabs(elements[d][begin] - 
                                          cur_cell.bounds.center[d]) / 2.5;
            box.min_corner[d] = box.center[d] - box.half_width[d];
            box.max_corner[d] = box.center[d] + box.half_width[d];
        }
    } else {
        box = bounding_box(elements, begin, end);
    }

    auto level = cur_cell.level + 1;

    auto loc = cur_cell.loc;
    loc[0] = loc[0] * 2 + i;
    loc[1] = loc[1] * 2 + j;
    loc[2] = loc[2] * 2 + k;

    OctreeCell new_cell = {level, loc, box, begin, end, min_code, max_code,
                          {-1, -1, -1, -1, -1, -1, -1, -1}, false};
    
    //build children recursively -- depth first
    if (end - begin > max_elements_per_cell) {
        new_cell.children = build_children(new_cell);
    } else {
        new_cell.is_leaf = true;
    }
    cells.push_back(new_cell);
    return cells.size() - 1;
}

std::array<int,8> Octree::build_children(OctreeCell& cur_cell) {
    std::array<int,8> child_indices;
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            for(int k = 0; k < 2; k++) {
                child_indices[4 * i + 2 * j + k] = build_child(cur_cell, i, j, k);
            }
        }
    }
    return child_indices;
}

OctreeCell& Octree::get_root() {
    return cells[get_root_index()];
}

int Octree::get_root_index() const {
    return cells.size() - 1;
}
