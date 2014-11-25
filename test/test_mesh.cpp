#include "UnitTest++.h"
#include "util.h"
#include "numerics.h"
#include "mesh.h"
#include "mesh_gen.h"

double perimeter(Mesh<2> m) {
    double p = 0;
    for (auto f: m.facets) {
        p += dist(f.vertices[0], f.vertices[1]);
    }
    return p;
}

TEST(Mesh2D) {
    auto m = circle_mesh({0,0}, 1.0);
    CHECK_CLOSE(m.facets[0].vertices[0][0], 1.0, 1e-15);
    auto m2 = m.refine_repeatedly(3);
    CHECK_EQUAL(m2.facets.size(), 32);
    double length = perimeter(m2);
    CHECK_CLOSE(length, 2 * M_PI, 1e-1);
}

TEST(CircleMesh) {
    std::array<double, 2> center = {20.0, 0.0};
    auto src_circle = circle_mesh(center, 19.0).refine_repeatedly(4);
    CHECK_CLOSE(src_circle.facets[16].vertices[0][0], 20.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[32].vertices[0][0], 1.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[48].vertices[0][0], 20.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[0].vertices[0][0], 39.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[16].vertices[0][1], 19.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[32].vertices[0][1], 0.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[48].vertices[0][1], -19.0, 1e-4);
    CHECK_CLOSE(src_circle.facets[0].vertices[0][1], 0.0, 1e-4);
}

double surface_area(Mesh<3> m) {
    double sa = 0;
    for (auto f: m.facets) {
        sa += tri_area(f.vertices);
    }
    return sa;
}

TEST(Mesh3D) {
    auto m = sphere_mesh({0,0,0}, 1.0); 
    auto m2 = m.refine_repeatedly(4);
    double sa = surface_area(m2);
    CHECK_CLOSE(sa, 4 * M_PI, 1e-1);
}

int main(int, char const *[])
{
    int retval = UnitTest::RunAllTests();
    return retval;
}
