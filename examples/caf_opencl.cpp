/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENCE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include <vector>
#include <iomanip>
#include <numeric>
#include <iostream>

#include "caf/all.hpp"
#include "caf/opencl/spawn_cl.hpp"
#include "util.h"

using namespace std;
using namespace caf;

namespace {

constexpr size_t matrix_size = 20;
constexpr const char* kernel_name = "matrix_mult";

// opencl kernel, multiplies matrix1 and matrix2
// last parameter is, by convention, the output parameter
constexpr const char* kernel_source = R"__(
  __kernel void matrix_mult(__global float* matrix1,
                __global float* matrix2,
                __constant int* hi,
                __global float* output) {
    // we only use square matrices, hence: width == height
    size_t size = get_global_size(0); // == get_global_size_(1);
    size_t x = get_global_id(0);
    size_t y = get_global_id(1);
    float result = 0;
    for (size_t idx = 0; idx < size; ++idx) {
      result += 1.0 / (matrix1[idx + y * size] + 1.0) * matrix2[x + idx * size];
    }
    output[x+y*size] = result;
  }
)__";

} // namespace <anonymous>

void print_as_matrix(const vector<float>& matrix) {
  for (size_t column = 0; column < matrix_size; ++column) {
    for (size_t row = 0; row < matrix_size; ++row) {
      cout << fixed << setprecision(2) << setw(9)
         << matrix[row + column * matrix_size];
    }
    cout << endl;
  }
}

void multiplier(event_based_actor* self) {
  // the opencl actor only understands vectors
  // so these vectors represent the matrices
  vector<float> m1(matrix_size * matrix_size);
  vector<float> m2(matrix_size * matrix_size);

  // fill each with ascending values
  iota(m1.begin(), m1.end(), 0);
  iota(m2.begin(), m2.end(), 0);

  // print "source" matrix
  cout << "calculating square of matrix:" << endl;
  // print_as_matrix(m1);
  cout << endl;

  // spawn an opencl actor
  // template parameter: signature of opencl kernel using proper return type
  //           instead of output parameter (implicitly
  //           mapped to the last kernel argument)
  // 1st arg: source code of one or more kernels
  // 2nd arg: name of the kernel to use
  // 3rd arg: global dimension arguments for opencl's enqueue;
  //      creates matrix_size * matrix_size global work items
  // 4th arg: offsets for global dimensions (optional)
  // 5th arg: local dimensions (optional)
  // 6th arg: number of elements in the result buffer
  auto worker = spawn_cl<float*(float*,float*,int*)>(kernel_source,
                          kernel_name,
                          {matrix_size, matrix_size},
                          {},
                          {},
                          matrix_size * matrix_size);
  // send both matrices to the actor and wait for a result
  const vector<int> c{10};
  self->sync_send(worker, move(m1), move(m2), move(c)).then(
    [=](const vector<float>& result) {
      // cout << "result: " << endl;
      // print_as_matrix(result);
    }
  );
}

int main() {
  announce<vector<float>>();
  announce<vector<int>>();
  spawn(multiplier);
  await_all_actors_done();
  shutdown();
  return 0;
}
