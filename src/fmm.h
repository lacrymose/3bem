#ifndef __FMM_H
#define __FMM_H

#include <array>
#include <vector>
#include <functional>

class OctreeCell;
class Octree;
class Box;
    
typedef std::function<double (std::array<double,3>, std::array<double,3>)> Kernel;

class FMMInfo {
public:
    FMMInfo(Kernel kernel, const Octree& src, std::vector<double>& values,
            const Octree& obs, int n_exp_pts, double mac2);

    // The squared multipole acceptance criteria. This represents how far away
    // a cell needs to be before the multipole to local operator is used 
    // instead of the particle to particle operator. See Yokota's Dual Tree
    // Traversal paper for details on the implementation of a MAC in an FMM.
    double mac2;

    // The n_exp_pts of interpolation.
    int n_exp_pts;

    // The interpolation expansion nodes in the reference cell.
    std::vector<std::array<double,3>> nodes; 

    Kernel kernel;

    // The source points and tree structure
    const Octree& src_oct;
    // The multipole expansion weights in the source cells.
    std::vector<double> multipole_weights;
    // The input point strengths.
    const std::vector<double>& values;

    // The observation points and tree structure
    const Octree& obs_oct;
    // These weight vectors should be of length N_cells * (n_exp_pts^3).
    std::vector<double> local_weights;
    // The observation effect strengths
    // obs_effect is the main output at the end of a FMM processing pass
    std::vector<double> obs_effect;


    void P2M_pts_cell(int m_cell_idx);
    void P2M_helper(int m_cell_idx);
    // The particle to multipole operator computes cell expansions from the
    // values of the particles in those cells. The recursively proceeds up the
    // tree by considering the nodes of the interpolation on child cell as the
    // particles for the next level.
    void P2M();

    //Multipole to point -- used by treecode but not FMM
    void M2P_cell_pt(const Box& m_cell_bounds,
                     int m_cell_idx, int pt_idx);

    // A treecode evaluation computes interactions between cells and points.
    // In contrast, a FMM evaluation also computes interactions between cells
    // and other cells. This results in a O(n log n) vs a O(n) method.
    // However, treecodes are simpler, and more easily parallelizable.
    void treecode_process_cell(const OctreeCell& cell, int cell_idx, int pt_idx);
    void treecode_eval_helper(const OctreeCell& cell, int pt_idx);
    void treecode_eval();

    void P2P_cell_pt(int m_cell_idx, int pt_idx);
};

//TODO: Make the functions for the different operations more uniform 
// -- global operation (P2M, treecode, FMM)
// -- cell-wise operations
// -- node-wise operations

//Particle to multipole
double interp_operator(const OctreeCell& cell,
                       const std::array<double,3>& node,
                       const std::array<double,3>& pt,
                       int n_exp_pts);
// Multipole to local and local to particle -- used by FMM but not treecode 
void M2L_cell_cell(FMMInfo& fmm_info, const Box& m_cell_bounds, int m_cell_idx, 
                   const Box& l_cell_bounds, int l_cell_idx);
void L2P_cell_pts(FMMInfo& fmm_info, int l_cell_idx);
void L2P(FMMInfo& fmm_info);

void fmm(FMMInfo& fmm_info);

#endif