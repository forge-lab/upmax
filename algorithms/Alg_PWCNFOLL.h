/*!
 * \author Ruben Martins - rubenm@andrew.cmu.edu
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2022, Ruben Martins, Vasco Manquinho, Ines Lynce
 * UpMax, Copyright (c) 2022, Pedro Orvalho, Vasco Manquinho, Ruben Martins
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef Alg_PWCNFOLL_h
#define Alg_PWCNFOLL_h

#include "core/Solver.h"

#include "../Encoder.h"
#include "../MaxSAT_Partition.h"
#include <map>
#include <set>

namespace openwbo {

//=================================================================================================
class PWCNFOLL : public MaxSAT_Partition {

public:
  //PWCNFOLL(int verb = _VERBOSITY_MINIMAL_, int enc = _CARD_TOTALIZER_) {
  PWCNFOLL(int verb = _VERBOSITY_SOME_, int mode = _SIZE_, int limit = -1) {
    solver = NULL;
    verbosity = verb;
    incremental_strategy = _INCREMENTAL_ITERATIVE_;
    encoding = _CARD_TOTALIZER_;
    encoder.setCardEncoding(_CARD_TOTALIZER_);
    min_weight = 1;
    _limit = limit;
  }
  ~PWCNFOLL() {
    if (solver != NULL)
      delete solver;
  }

  StatusCode search();

  // Print solver configuration.
  void printConfiguration() {

    if(!print) return;

    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");
    printf("c |  Algorithm: %23s                                             "
           "                      |\n",
           "PWCNF OLL");
    print_Card_configuration(encoding);
    printf("c |                                                                "
           "                                       |\n");
  }

protected:
  // Rebuild MaxSAT solver
  //
  Solver *rebuildSolver(); // Rebuild MaxSAT solver.

  // Other
  void initRelaxation(); // Relaxes soft clauses.

  StatusCode unweighted();
  StatusCode weighted();

  Solver *solver;  // SAT Solver used as a black box.
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  // Controls the incremental strategy used by MSU3 algorithms.
  int incremental_strategy;
  // Controls the cardinality encoding used by MSU3 algorithms.
  int encoding;

  // Literals to be used in the constraint that excludes models.
  vec<Lit> objFunction;
  vec<uint64_t> coeffs; // Coefficients of the literals that are used in the
                        // constraint that excludes models.

  std::map<Lit, int> coreMapping; // Mapping between the assumption literal and
                                  // the respective soft clause.

  std::map<Lit, int>
      coreCardinality; // Mapping between the assumption literal and
                       // the respective soft clause.

  // lit -> <ID, bound, weight>
  std::map<Lit, std::pair<std::pair<int, uint64_t>, uint64_t>> boundMapping;

  // Soft clauses that are currently in the MaxSAT formula.
  vec<bool> activeSoft;

  uint64_t findNextWeightDiversity(uint64_t weight,
                                   std::set<Lit> &cardinality_assumptions);
  uint64_t findNextWeight(uint64_t weight,
                          std::set<Lit> &cardinality_assumptions);

  uint64_t min_weight;

  int _partitions;
  int _mode;
  int _limit;

  void createPartitions();

  vec< vec<int> > soft_partitions;
  vec< vec<int> > soft_partitions_tmp;
};
} // namespace openwbo

#endif
