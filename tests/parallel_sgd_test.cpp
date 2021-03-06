/**
 * @file parallel_sgd_test.cpp
 * @author Shikhar Bhardwaj
 * @author Marcus Edel
 * @author Conrad Sanderson
 *
 * ensmallen is free software; you may redistribute it and/or modify it under
 * the terms of the 3-clause BSD license.  You should have received a copy of
 * the 3-clause BSD license along with ensmallen.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */

#include <ensmallen.hpp>
#include "catch.hpp"

#include "test_function_tools.hpp"

using namespace std;
using namespace arma;
using namespace ens;
using namespace ens::test;

// These tests are only compiled if OpenMP is used.
#ifdef ENS_USE_OPENMP

/**
 * Test the correctness of the Parallel SGD implementation using a specified
 * sparse test function, with guaranteed disjoint updates between different
 * threads.
 */
TEST_CASE("SimpleParallelSGDTest", "[ParallelSGDTest]")
{
  SparseTestFunction f;

  ConstantStep decayPolicy(0.4);

  // The batch size for this test should be chosen according to the threads
  // available on the system. If the update does not touch each datapoint, the
  // test will fail.

  size_t threadsAvailable = omp_get_max_threads();

  for (size_t i = threadsAvailable; i > 0; --i)
  {
    omp_set_num_threads(i);

    size_t batchSize = std::ceil((float) f.NumFunctions() / i);

    ParallelSGD<ConstantStep> s(10000, batchSize, 1e-5, true, decayPolicy);

    arma::mat coordinates = f.GetInitialPoint();
    double result = s.Optimize(f, coordinates);

    // The final value of the objective function should be close to the optimal
    // value, that is the sum of values at the vertices of the parabolas.
    REQUIRE(result == Approx(123.75).epsilon(0.0001));

    // The co-ordinates should be the vertices of the parabolas.
    REQUIRE(coordinates[0] == Approx(2.0).epsilon(0.0002));
    REQUIRE(coordinates[1] == Approx(1.0).epsilon(0.0002));
    REQUIRE(coordinates[2] == Approx(1.5).epsilon(0.0002));
    REQUIRE(coordinates[3] == Approx(4.0).epsilon(0.0002));
  }
}

/**
 * When run with a single thread, parallel SGD should be identical to normal
 * SGD.
 */
TEST_CASE("ParallelSGDGeneralizedRosenbrockTest", "[ParallelSGDTest]")
{
  // Loop over several variants.
  for (size_t i = 10; i < 50; i += 5)
  {
    // Create the generalized Rosenbrock function.
    GeneralizedRosenbrockFunction f(i);

    ConstantStep decayPolicy(0.001);

    ParallelSGD<ConstantStep> s(0, f.NumFunctions(), 1e-12, true, decayPolicy);

    arma::mat coordinates = f.GetInitialPoint();

    omp_set_num_threads(1);
    double result = s.Optimize(f, coordinates);

    REQUIRE(result == Approx(0.0).margin(1e-8));
    for (size_t j = 0; j < i; ++j)
      REQUIRE(coordinates[j] == Approx(1.0).epsilon(0.0001));
  }
}

#endif

/**
 * Test the correctness of the Exponential backoff stepsize decay policy.
 */
TEST_CASE("ExponentialBackoffDecayTest", "[ParallelSGDTest]")
{
  ExponentialBackoff decayPolicy(100, 100, 0.9);

  // At the first iteration, stepsize should be unchanged
  REQUIRE(decayPolicy.StepSize(1) == 100);
  // At the 99th iteration, stepsize should be unchanged
  REQUIRE(decayPolicy.StepSize(99) == 100);
  // At the 100th iteration, stepsize should be changed
  REQUIRE(decayPolicy.StepSize(100) == 90);
  // At the 210th iteration, stepsize should be unchanged
  REQUIRE(decayPolicy.StepSize(210) == 90);
  // At the 211th iteration, stepsize should be changed
  REQUIRE(decayPolicy.StepSize(211) == 81);
}
