/*!
 * \author Ruben Martins - rubenm@cs.cmu.edu
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2021, Ruben Martins, Vasco Manquinho, Ines Lynce
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

#ifndef PWCNFMSU3_H_
#define PWCNFMSU3_H_

#ifdef SIMP
#include "simp/SimpSolver.h"
#else
#include "core/Solver.h"
#endif

#include "../Encoder.h"
#include "../MaxSAT_Partition.h"
#include <algorithm>
#include <deque>
#include <map>
#include <set>

namespace openwbo {

class PWCNFMSU3 : public MaxSAT_Partition {

public:
  PWCNFMSU3(int verb = _VERBOSITY_SOME_, int mode = _SIZE_, int limit = -1) {
    solver = NULL;
    verbosity = verb;
    incremental_strategy = _INCREMENTAL_ITERATIVE_;
    encoding = _CARD_TOTALIZER_;
    encoder.setCardEncoding(encoding);
    _partitions = 0;
    _mode = mode;
    _limit = limit;
  }

  virtual ~PWCNFMSU3() {
    if (this->solver != NULL) {
      delete this->solver;
    }
  }

  StatusCode search();

  // Print solver configuration.
  void printConfiguration() {

    if(!print) return;

    printf("c ==========================================[ Solver Settings "
           "]============================================\n");
    printf("c |                                                                "
           "                                       |\n");

    print_PWCNFMSU3_configuration();
    print_Card_configuration(encoding);
  }

  // void createGraph() {
  //   if (nPartitions() == 0) {
  //     split(UNFOLDING_MODE, graph_type);
  //   }
  // }

protected:
  // Print PartMSU3 configuration.
  void print_PWCNFMSU3_configuration();

  // Rebuild MaxSAT solver
  //
  Solver *rebuildSolver(); // Rebuild MaxSAT solver.

  StatusCode search_part(); // MSU3 that uses a binary tree to guide the partition
                          // merging process
  StatusCode search_single();

  // Other
  void initRelaxation(); // Relaxes soft clauses.
  void createPartitions();
  int merge_part(int mode, vec<Lit>& assump);

  StatusCode bmo_part();
  StatusCode bmo_single();

  //StatusCode enumerate_opt(Solver* solver, vec<Lit>& assumptions);

  Solver *solver; // SAT Solver used as a black box.
  Encoder encoder; // Interface for the encoder of constraints to CNF.

  // Controls the incremental strategy used by MSU3 algorithms.
  int incremental_strategy;
  // Controls the cardinality encoding used by MSU3 algorithms.
  int encoding;

  // Literals to be used in the constraint that excludes models.
  vec<Lit> objFunction;
  vec<int> coeffs; // Coefficients of the literals that are used in the
                   // constraint that excludes models.

  std::map<Lit, int> coreMapping; // Mapping between the assumption literal and
                                  // the respective soft clause.

  // Soft clauses that are currently in the MaxSAT formula.
  vec<bool> activeSoft;


  vec< vec<int> > soft_partitions;
  vec<uint64_t> lb_partitions;
  vec<Encoder*> encoder_partitions;
  vec< vec<int> > merged_partitions;
  vec< vec<Lit> > lits_partitions;
  vec<bool> active_partitions;  
  
  int _partitions;
  int _mode;
  int _limit;
};

} // namespace openwbo

#endif /* PARTMSU3_H_ */
