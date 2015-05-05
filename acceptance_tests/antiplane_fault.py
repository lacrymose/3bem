from tbempy import *
from tbempy.TwoD import *
from dislocation import *
import numpy as np
import scipy.sparse.linalg as sp_la
# import matplotlib.pyplot as plt

def full_space():
    fault = line_mesh([0, -1], [0, 0])
    slip = np.ones(fault.n_dofs())
    surface = line_mesh([-10, -0.5], [10, -0.5]).refine_repeatedly(8)

    qs = QuadStrategy(5, 8, 4.0, 1e-13);

    double_kernel = LaplaceDouble()
    double_layer = make_boundary_integral(surface, fault, double_kernel)

    def fnc(x):
        richardson_dir = [1, 0]
        if (x[0] < 0):
            richardson_dir = [-1, 0]
        if x[0] == 0.0:
            return 0.0
        obs = ObsPt(0.001, x, [0, 1], richardson_dir)

        op = mesh_to_points_operator(double_layer, qs, [obs])
        val = op.apply(slip).storage[0]
        exact = np.arctan(0.5 / x[0]) / np.pi
        assert(np.abs(exact - val) < 2e-15)
        return val
    u = interpolate(surface, fnc)

def get_qs():
    return QuadStrategy(5, 9, 3.0, 1e-5)

def solve_half_space(slip, fault, surface):
    constraint_matrix = faulted_surface_constraints(2, surface, fault)
    qs = get_qs()

    hypersingular_kernel = LaplaceHypersingular()
    hypersingular_mthd = make_adaptive_integration_mthd(qs, hypersingular_kernel)
    rhs_op = integral_operator(surface, fault, hypersingular_mthd)
    rhs = -(rhs_op.apply(slip))
    rhs_condensed = condense_vector(constraint_matrix, rhs)

    lhs_op = integral_operator(surface, surface, hypersingular_mthd)

    np_rhs = rhs_condensed.storage
    def mv(v):
        distributed = distribute_vector(constraint_matrix, v, surface.n_dofs())
        applied = lhs_op.apply(distributed)
        condensed = condense_vector(constraint_matrix, applied)
        print("ITERATION: " + str(mv.it))
        mv.it += 1
        return condensed.storage
    mv.it = 0

    A = sp_la.LinearOperator((np_rhs.shape[0], np_rhs.shape[0]),
                             matvec = mv, dtype = np.float64)
    res = sp_la.gmres(A, np_rhs, tol = 1e-6)
    soln = distribute_vector(constraint_matrix, res[0], surface.n_dofs())
    return soln

def half_space(refine):
    fault = line_mesh([0, -1], [0, 0])
    slip = np.ones(fault.n_dofs())
    surface = line_mesh([-50, 0.0], [50, 0.0]).refine_repeatedly(refine)
    soln = solve_half_space(slip, fault, surface).storage
    xs = surface.facets[:, :, 0].reshape((surface.n_facets() * 2))
    indices = [i for i in range(len(xs)) if 0 < np.abs(xs[i]) < 10]
    xs = xs[indices]
    exact = np.arctan(1.0 / xs) / np.pi
    error = np.sqrt(np.mean((exact - soln[indices]) ** 2))
    return error

def half_space_interior(refine):
    fault = line_mesh([0, -1], [0, 0])
    slip = VectorX([1.0] * fault.n_dofs())
    surface = line_mesh([-50, 0.0], [50, 0.0]).refine_repeatedly(refine)
    qs = get_qs()

    soln = solve_half_space(slip, fault, surface)

    xs = np.linspace(-5, 5, 100)
    ys = np.linspace(-5, 0, 100)

    pts_x = []
    pts_y = []
    for x in xs:
        for y in ys:
            pts_x.append(ObsPt(0.001, [x, y], [1, 0], [0, -1]))
            pts_y.append(ObsPt(0.001, [x, y], [0, 1], [0, -1]))

    double_kernel = LaplaceDouble()
    hypersingular_kernel = LaplaceHypersingular()
    double_mthd = make_adaptive_integration_mthd(qs, double_kernel)
    hypersingular_mthd = make_adaptive_integration_mthd(qs, hypersingular_kernel)

    disp = mesh_to_points_operator(pts_x, fault, double_mthd).apply(slip) +\
           mesh_to_points_operator(pts_x, surface, double_mthd).apply(soln)

    tracx = mesh_to_points_operator(pts_x, fault, hypersingular_mthd).apply(slip) +\
           mesh_to_points_operator(pts_x, surface, hypersingular_mthd).apply(soln)
    tracy = mesh_to_points_operator(pts_y, fault, hypersingular_mthd).apply(slip) +\
           mesh_to_points_operator(pts_y, surface, hypersingular_mthd).apply(soln)

    x = np.array([p.loc[0] for p in pts_x])
    y = np.array([p.loc[1] for p in pts_x])

    d = 1
    s = 1
    shear_modulus = 1.0
    exact_uz = (-s / (2 * np.pi)) * (
        np.arctan((y - d) / x) -
        np.arctan((y + d) / x))
    exact_tracx = -(s * shear_modulus) / (2 * np.pi) * (
        ((y + d) / (x ** 2 + (y + d) ** 2)) -
        ((y - d) / (x ** 2 + (y - d) ** 2)))
    exact_tracy = (s * shear_modulus) / (2 * np.pi) * (
        (x / (x ** 2 + (y + d) ** 2)) -
        (x / (x ** 2 + (y - d) ** 2)))
    l2_error_disp = np.sqrt(np.mean((disp.storage - exact_uz) ** 2))
    l2_error_tracx = np.sqrt(np.mean((tracx.storage - exact_tracx) ** 2))
    l2_error_tracy = np.sqrt(np.mean((tracy.storage - exact_tracy) ** 2))

    return l2_error_disp, l2_error_tracx, l2_error_tracy

def test_halfspace():
    error = half_space(5)
    assert(error < 0.025)

def test_halfspace_interior():
    disp_error, tracx_error, tracy_error = half_space_interior(7)
    assert(disp_error < 3e-4)
    assert(tracx_error < 2e-3)
    assert(tracy_error < 2e-3)

def test_convergence():
    prev = half_space(4)
    for i in range(5,8):
        error = half_space(i)
        ratio = prev / error
        assert(ratio > 2.0)

if __name__ == "__main__":
    half_space(7)
    half_space_interior(7)
