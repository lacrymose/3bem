#include "UnitTest++.h"
#include "fmm.h"
#include "laplace_kernels.h"
#include "elastic_kernels.h"
#include "identity_kernels.h"
#include "nbody_operator.h"
#include "test_shared.h"

using namespace tbem;

template <size_t dim>
NBodyData<dim> ones_data(size_t n) 
{
    auto src_pts = random_pts<dim>(n);
    auto obs_pts = random_pts<dim>(n);
    auto normals = random_pts<dim>(n);
    std::vector<double> weights(n, 1.0);
    return NBodyData<dim>{src_pts, normals, obs_pts, normals, weights};
}

TEST(MakeSurroundingSurface) 
{
    auto surface = make_surrounding_surface<2>(4);
    CHECK_CLOSE(surface.pts[0], (Vec<double,2>{1.0, 0.0}), 1e-12);
    CHECK_CLOSE(surface.pts[1], (Vec<double,2>{0.0, 1.0}), 1e-12);
    CHECK_CLOSE(surface.pts[3], (Vec<double,2>{0.0, -1.0}), 1e-12);
    CHECK_CLOSE(surface.normals[0], (Vec<double,2>{1.0, 0.0}), 1e-12);
    CHECK_CLOSE(surface.normals[1], (Vec<double,2>{0.0, 1.0}), 1e-12);
} 

template <size_t dim, size_t R, size_t C>
void test_kernel(const NBodyData<dim>& data, const Kernel<dim,R,C>& K,
    size_t order, double allowed_error) 
{
    size_t n_per_cell = std::max<size_t>(50, order);
    BlockVectorX x(C, VectorX(data.src_weights));
    FMMOperator<dim,R,C> tree(K, data, {3.5, order, n_per_cell, 0.05});
    TIC
    auto out = tree.apply(x);
    TOC("FMM");

    BlockDirectNBodyOperator<dim,R,C> exact_op{data, K};
    auto exact = exact_op.apply(x);
    for (size_t d = 0; d < R; d++) {
        for (size_t i = 0; i < data.obs_locs.size(); i++) {
            auto error = std::fabs((out[d][i] - exact[d][i]) / exact[d][i]);
            if (error > allowed_error) {
                std::cout << out[d][i] << " " << exact[d][i] << std::endl;
                std::cout << data.obs_locs[i] << std::endl;
            }
            CHECK_CLOSE(error, 0, allowed_error);
        }
    }
}


template <size_t dim, size_t R, size_t C>
void test_kernel(const Kernel<dim,R,C>& K, size_t order, double allowed_error) 
{
    size_t n = 2000;
    auto data = ones_data<dim>(n);
    return test_kernel(data, K, order, allowed_error);    
}

TEST(Farfield) 
{
    size_t n = 2000;
    auto src_pts = random_pts<2>(n);
    auto obs_pts = random_pts<2>(n);
    for (size_t i = 0; i < n; i++) {
        obs_pts[i][0] += 4.0;
    }
    auto normals = random_pts<2>(n);
    std::vector<double> weights(n, 1.0);
    NBodyData<2> data{src_pts, normals, obs_pts, normals, weights};
    test_kernel(data, LaplaceDouble<2>(), 26, 1e-4);
}

TEST(SingleLayer2DFMM) 
{
    test_kernel(LaplaceSingle<2>(), 15, 1e-4);
}

TEST(DoubleLayer2DFMM) 
{
    test_kernel(LaplaceDouble<2>(), 30, 1e-4);
}

TEST(HypersingularLayer2DFMM) 
{
    test_kernel(LaplaceHypersingular<2>(), 30, 1e-4);
}

TEST(SingleLayer3DFMM) 
{
    test_kernel(LaplaceSingle<3>(), 35, 1e-4);
}

TEST(DoubleLayer3DFMM) 
{
    test_kernel(LaplaceDouble<3>(), 200, 1e-4);
}

TEST(ElasticDisplacement2DFMM)
{
    test_kernel(ElasticDisplacement<2>(30e9, 0.25), 15, 1e-4);
}
TEST(ElasticTraction2DFMM)
{
    test_kernel(ElasticTraction<2>(30e9, 0.25), 40, 1e-4);
}
TEST(ElasticHypersingular2DFMM)
{
    test_kernel(ElasticHypersingular<2>(30e9, 0.25), 40, 1e-4);
}

int main(int, char const *[])
{
    // return UnitTest::RunAllTests();
    return RunOneTest("Farfield");
}
