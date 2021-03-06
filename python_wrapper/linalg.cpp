#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "iterable_converter.h"
#include "op_wrap.h"

#include "dense_operator.h"
#include "sparse_operator.h"
namespace p = boost::python;

void export_linalg() {
    using namespace tbem;

    p::class_<OperatorI, boost::noncopyable>("OperatorI", p::no_init);

    export_operator<DenseOperator>(
        p::class_<DenseOperator, p::bases<OperatorI>>("DenseOperator",
            p::init<size_t, size_t, std::vector<double>>())
        .def("data", &DenseOperator::data,
             p::return_value_policy<p::return_by_value>())
    );

    VectorFromIterable().from_python<std::vector<DenseOperator>>();
    p::def("compose_dense_ops", &compose_dense_ops); 

    export_operator<SparseOperator>(
        p::class_<SparseOperator, p::bases<OperatorI>>("SparseOperator", p::no_init)
        .def("nnz", &SparseOperator::nnz)
        .def("to_dense", &SparseOperator::to_dense)
        .add_property("values", make_getter(
                &SparseOperator::values, 
                p::return_value_policy<p::return_by_value>()))
        .add_property("column_indices", make_getter(
                &SparseOperator::column_indices, 
                p::return_value_policy<p::return_by_value>()))
        .add_property("row_ptrs", make_getter(
                &SparseOperator::row_ptrs, 
                p::return_value_policy<p::return_by_value>()))
    );
}
