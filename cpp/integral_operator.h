#ifndef __ALJSDLAJKHAH_INTEGRAL_OPERATOR_H
#define __ALJSDLAJKHAH_INTEGRAL_OPERATOR_H
#include "sparse_operator.h"
#include "galerkin_operator.h"
#include "interpolation_operator.h"
#include "nbody_operator.h"
#include "integral_term.h"
#include "nearfield_operator.h"

namespace tbem {

/* This functions in this file creates integral operators for galerkin boundary
 * integral equations
 *
 * The general form is
 * A = G(FI + N + C)
 * where 
 * G is the galerkin op,
 * F is the farfield nbody op,
 * I is the farfield interpolation op,
 * N is the nearfield op,
 * C is the nearfield correction op (to remove overlap with farfield)
 */

template <size_t dim, size_t R, size_t C>
struct IntegralOperator: public OperatorI {
    const SparseOperator nearfield;
    const SparseOperator farfield_correction;
    const SparseOperator galerkin;
    const DenseOperator farfield;
    const SparseOperator interp;

    IntegralOperator(const SparseOperator& nearfield,
        const SparseOperator& farfield_correction,
        const SparseOperator& galerkin,
        const DenseOperator& farfield,
        const SparseOperator& interp):
        nearfield(nearfield),
        farfield_correction(farfield_correction),
        galerkin(galerkin),
        farfield(farfield),
        interp(interp)
    {}

    virtual size_t n_rows() const {return galerkin.n_rows();} 
    virtual size_t n_cols() const {return nearfield.n_cols();}
    virtual std::vector<double> apply(const std::vector<double>& x) const {
        auto nbody_far = galerkin.apply(farfield.apply(interp.apply(x)));
        auto eval = nearfield.apply(x);
        auto correction = farfield_correction.apply(x);
        for (size_t i = 0; i < eval.size(); i++) {
            eval[i] += nbody_far[i] + correction[i];
        }
        return eval;
    }

    /* The nearfield matrix can be useful for preconditioning as it approximates
     * the most important parts of the full matrix but is still sparse.
     */
    const SparseOperator& get_nearfield_matrix() {
        return nearfield;
    }
};


template <size_t dim, size_t R, size_t C>
IntegralOperator<dim,R,C> integral_operator(const Mesh<dim>& obs_mesh,
    const Mesh<dim>& src_mesh, const IntegrationMethodI<dim,R,C>& mthd,
    const Mesh<dim>& all_mesh) 
{
    auto obs_pts = galerkin_obs_pts(obs_mesh, mthd.get_obs_quad(), all_mesh);
    auto nearfield = make_nearfield_operator(obs_pts, src_mesh, mthd, all_mesh);
    auto far_correction = make_farfield_correction_operator(
        obs_pts, src_mesh, mthd, all_mesh
    );

    auto nbody_data = nbody_data_from_bem(obs_mesh, src_mesh,
        mthd.get_obs_quad(), mthd.get_src_quad());
    auto farfield = make_direct_nbody_operator(nbody_data, mthd.get_kernel());

    auto obs_quad = mthd.get_obs_quad();
    auto src_quad = mthd.get_src_quad();
    auto galerkin = make_galerkin_operator(R, obs_mesh, obs_quad);
    auto interp = make_interpolation_operator(C, src_mesh, src_quad);

    return IntegralOperator<dim,R,C>(
        galerkin.right_multiply(nearfield),
        galerkin.right_multiply(far_correction),
        galerkin, farfield, interp
    );
}

template <size_t dim, size_t R, size_t C>
DenseOperator dense_integral_operator(const Mesh<dim>& obs_mesh,
    const Mesh<dim>& src_mesh, const IntegrationMethodI<dim,R,C>& mthd,
    const Mesh<dim>& all_mesh)
{
    auto op = integral_operator(obs_mesh, src_mesh, mthd, all_mesh);
    auto out = op.farfield_correction.add_with_dense(
        op.nearfield.add_with_dense(
            op.interp.left_multiply_with_dense(
                op.galerkin.right_multiply_with_dense(
                    op.farfield
                )
            )
        )
    );
    return out;
}

template <size_t dim, size_t R, size_t C>
DenseOperator dense_interior_operator(const std::vector<Vec<double,dim>>& locs,
    const std::vector<Vec<double,dim>>& normals, const Mesh<dim>& src_mesh,
    const IntegrationMethodI<dim,R,C>& mthd, const Mesh<dim>& all_mesh)
{
    auto obs_pts = interior_obs_pts(locs, normals, all_mesh);
    auto nearfield = make_nearfield_operator(obs_pts, src_mesh, mthd, all_mesh);
    auto far_correction = make_farfield_correction_operator(
        obs_pts, src_mesh, mthd, all_mesh
    );

    auto nbody_src = nbody_src_from_bem(src_mesh, mthd.get_src_quad());
    NBodyData<dim> nbody_data{
        locs, normals, nbody_src.locs, nbody_src.normals, nbody_src.weights
    };
    auto farfield = make_direct_nbody_operator(nbody_data, mthd.get_kernel());
    auto interp = make_interpolation_operator(C, src_mesh, mthd.get_src_quad());

    auto out = far_correction.add_with_dense(
        nearfield.add_with_dense(
            interp.left_multiply_with_dense(
                farfield
            )
        )
    );
    return out;
}

} // end namespace tbem

#endif
