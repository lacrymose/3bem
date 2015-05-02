#ifndef __QQQQQQQ2222222_GEOMETRY_H
#define __QQQQQQQ2222222_GEOMETRY_H
#include "vec_ops.h"

namespace tbem {

template <typename T>
Vec<T,3> outer_product(const Vec<double,3>& a, const T& b) {
    return {b * a[0], b * a[1], b * a[2]};
}

template <typename T>
Vec<T,2> outer_product(const Vec<double,2>& a, const T& b) {
    return {b * a[0], b * a[1]};
}

template <typename T>
Vec3<T> cross(const Vec3<T>& x, const Vec3<T>& y) {
    return {
        x[1] * y[2] - x[2] * y[1],
        x[2] * y[0] - x[0] * y[2],
        x[0] * y[1] - x[1] * y[0]
    };
}

template <typename T, size_t dim>
T dot_product(const Vec<T,dim>& x, const Vec<T,dim>& y) {
    return sum(x * y);
}

template <typename T, size_t dim>
T hypot2(const Vec<T,dim>& v) {
    return dot_product(v, v);
}

template <typename T, size_t dim>
T hypot(const Vec<T,dim>& v) {
    return std::sqrt(hypot2(v));
}

template <typename T, size_t dim>
void normalize(Vec<T,dim>& v) {
    v /= hypot(v);
}

template <typename T, size_t dim>
Vec<T,dim> normalized(const Vec<T,dim>& v) {
    Vec<T,dim> res = v;
    normalize(res);
    return res;
}

template <typename T, size_t dim>
inline T dist2(const Vec<T,dim>& v0, const Vec<T,dim>& v1) {
    return hypot2(v1 - v0);
}

template <typename T, size_t dim>
inline T dist(const Vec<T,dim>& v0, const Vec<T,dim>& v1) {
    return hypot(v1 - v0);
}

template <typename T, size_t dim>
inline Vec<T,dim> unit(const int k);

template <>
inline Vec3<double> unit<double,3>(const int k) {
    Vec3<double> e_k = {0.0, 0.0, 0.0};
    e_k[k] = 1.0;
    return e_k;
}

template <>
inline Vec2<double> unit<double,2>(const int k) {
    Vec2<double> e_k = {0.0, 0.0};
    e_k[k] = 1.0;
    return e_k;
}
inline Vec3<double> 
unscaled_normal(const std::array<Vec3<double>,3>& corners) {
    return cross(corners[2] - corners[0], corners[2] - corners[1]);
}

inline Vec2<double> 
unscaled_normal(const std::array<Vec2<double>,2>& corners) {
    return {
        -(corners[1][1] - corners[0][1]),
        corners[1][0] - corners[0][0]
    };
}

inline Vec3<double> 
tri_normal(const std::array<Vec3<double>,3>& corners) {
    auto unscaled = unscaled_normal(corners);
    return normalized(unscaled);
}

inline double tri_area(const Vec3<double>& unscaled_normal) {
    return 0.5 * hypot(unscaled_normal);
}

inline double tri_area(const std::array<Vec3<double>,3>& corners) {
    return tri_area(unscaled_normal(corners));
}

enum Side {FRONT, INTERSECT, BEHIND};

/* Determine which side of the plane/line defined by triangle/segment 
 * the provided point is on.
 */
template <size_t dim>
Side which_side_point(const std::array<Vec<double,dim>,dim>& face,
                const Vec<double,dim>& pt) {
    auto normal = unscaled_normal(face);
    double dot_val = dot_product(pt - face[0], normal);
    if (dot_val > 0) { return FRONT; }
    else if (dot_val < 0) { return BEHIND; }
    else { return INTERSECT; }
}

/* Returns the side of a plane that a triangle/segment is on. */
template <size_t dim>
Side facet_side(std::array<Side,dim> s);

template <>
inline Side facet_side<2>(std::array<Side,2> s) {
    if (s[0] == s[1]) { return s[0]; } 
    else if(s[0] == INTERSECT) { return s[1]; }
    else if(s[1] == INTERSECT) { return s[0]; }
    else { return INTERSECT; }
}

template <>
inline Side facet_side<3>(std::array<Side,3> s) {
    auto edge0 = facet_side<2>({s[0], s[1]});
    auto edge1 = facet_side<2>({s[0], s[2]});
    auto edge2 = facet_side<2>({s[1], s[2]});
    if (edge0 == INTERSECT && edge1 == edge2) {
        return edge1;
    }
    if (edge1 == INTERSECT && edge2 == edge0) {
        return edge2;
    }
    if (edge2 == INTERSECT && edge0 == edge1) {
        return edge0;
    }
    return edge0;
}


/* Determine the side of the plane/line defined by triangle/segment
 * that the given triangle/segment is on
 */
template <size_t dim>
Side which_side_facet(const std::array<Vec<double,dim>,dim>& plane,
    const std::array<Vec<double,dim>,dim>& face) {

    std::array<Side,dim> sides;
    for (int d = 0; d < dim; d++) {
        sides[d] = which_side_point<dim>(plane, face[d]);
    }
    return facet_side<dim>(sides);
}

} // end namespace tbem

#endif