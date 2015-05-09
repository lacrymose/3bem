#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "iterable_converter.h"
#include "block_op_wrap.h"

#include "dense_operator.h"
#include "sparse_operator.h"
#include "block_operator.h"
#include "dense_builder.h"
namespace p = boost::python;

void export_linalg() {
    using namespace tbem;

    p::class_<DenseOperator>("DenseOperator", p::no_init)
        .def("data", &DenseOperator::data,
             p::return_value_policy<p::return_by_value>())
        .def("apply", &DenseOperator::apply)
        .def("n_rows", &DenseOperator::n_rows)
        .def("n_cols", &DenseOperator::n_cols);

    export_block_operator<BlockDenseOperator>(
        p::class_<BlockDenseOperator>("BlockDenseOperator", p::no_init)
        .def("get_block", &BlockDenseOperator::get_block,
            p::return_value_policy<p::reference_existing_object>())
    );

    export_block_operator<BlockSparseOperator>(
        p::class_<BlockSparseOperator>("BlockSparseOperator", p::no_init)
        .def("nnz", &BlockSparseOperator::nnz)
        .add_property("values", make_getter(
                &BlockSparseOperator::values, 
                p::return_value_policy<p::return_by_value>()))
        .add_property("column_indices", make_getter(
                &BlockSparseOperator::column_indices, 
                p::return_value_policy<p::return_by_value>()))
        .add_property("row_ptrs", make_getter(
                &BlockSparseOperator::row_ptrs, 
                p::return_value_policy<p::return_by_value>()))
    );
}
