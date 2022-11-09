/*!
 * \author Ruben Martins - rubenm@andrew.cmu.edu
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2022, Ruben Martins, Vasco Manquinho, Ines Lynce
 * UpMax,    Copyright (c) 2022, Pedro Orvalho, Vasco Manquinho, Ruben Martins
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

#include "Alg_UpMSU3.h"

using namespace upmax;

/*_________________________________________________________________________________________________
  |
  |  MSU3_iterative : [void] ->  [void]
  |
  |  Description:
  |
  |    Incremental iterative encoding for the MSU3 algorithm.
  |    The cardinality constraint is build incrementally in an iterative
  |    fashion.
  |    This algorithm is similar to the non-incremental version of MSU3
  |    algorithm.
  |    It is advised to first check the non-incremental version and then check
  |    the differences.
  |
  |  For further details see:
  |    *  Ruben Martins, Saurabh Joshi, Vasco M. Manquinho, Inês Lynce:
  |       Incremental Cardinality Constraints for MaxSAT. CP 2014: 531-548
  |
  |  Pre-conditions:
  |    * Assumes Totalizer is used as the cardinality encoding.
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
StatusCode UpMSU3::MSU3_iterative() {

  if (encoding != _CARD_TOTALIZER_) {
    if(print) {
      printf("Error: Currently algorithm MSU3 with iterative encoding only "
             "supports the totalizer encoding.\n");
      printf("s UNKNOWN\n");
    }
    throw MaxSATException(__FILE__, __LINE__, "MSU3 only supports totalizer");
    return _UNKNOWN_;
  }

  lbool res = l_True;
  initRelaxation();
  solver = rebuildSolver();
  vec<Lit> assumptions;
  vec<Lit> joinObjFunction;
  vec<Lit> currentObjFunction;
  vec<Lit> encodingAssumptions;
  encoder.setIncremental(_INCREMENTAL_ITERATIVE_);

  activeSoft.growTo(maxsat_formula->nSoft(), false);
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
    coreMapping[getAssumptionLit(i)] = i;

  int current_partition = 0;
  vec<bool> activeSoftPartition;
  activeSoftPartition.growTo(maxsat_formula->nSoft(), false);
  _partitions = soft_partitions.size();
  
  printf("c #Soft Partitions = %d\n",_partitions);
  printf("c Partition #%d= %d\n",current_partition+1,soft_partitions[current_partition].size());

   for (int i = 0; i < soft_partitions[current_partition].size(); i++){
    activeSoftPartition[soft_partitions[current_partition][i]] = true;
    assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
   }

  // TODO: check if the hard clauses are satisfiable

  for (;;) {

    if (_limit != -1 && current_partition+1 != _partitions)
      solver->setConfBudget(_limit);
    else
      solver->budgetOff();
    res = searchSATSolver(solver, assumptions);
    if (res != l_False) {

      current_partition++;

      if (res == l_True){
        nbSatisfiable++;
        uint64_t newCost = computeCostModel(solver->model);
        if (newCost <= ubCost){
          saveModel(solver->model);
          printBound(newCost);
          ubCost = newCost;
        }

        if (ubCost == 0){
          printAnswer(_OPTIMUM_);
          return _OPTIMUM_;
        }
      }

      if (current_partition == _partitions){
        printAnswer(_OPTIMUM_);
        return _OPTIMUM_; 
      } else {

        printf("c Partition #%d= %d\n",current_partition+1,soft_partitions[current_partition].size());

        for (int i = 0; i < soft_partitions[current_partition].size(); i++){
          activeSoftPartition[soft_partitions[current_partition][i]] = true;
          assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
        }

      }
    } else {
      lbCost++;
      nbCores++;
      if (verbosity > 0)
        printf("c LB : %-12" PRIu64 "\n", lbCost);

      sumSizeCores += solver->conflict.size();

      if (solver->conflict.size() == 0) {
        printAnswer(_UNSATISFIABLE_);
        return _UNSATISFIABLE_;
      }

      joinObjFunction.clear();
      for (int i = 0; i < solver->conflict.size(); i++) {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end()) {
          assert(!activeSoft[coreMapping[solver->conflict[i]]]);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
          joinObjFunction.push(
              getRelaxationLit(coreMapping[solver->conflict[i]]));
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < maxsat_formula->nSoft(); i++) {
        if (activeSoft[i])
          currentObjFunction.push(getRelaxationLit(i));
        else if (!activeSoft[i] && activeSoftPartition[i])
          assumptions.push(~getAssumptionLit(i));
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      if (!encoder.hasCardEncoding()) {
        if (lbCost != (unsigned)currentObjFunction.size()) {
          encoder.buildCardinality(solver, currentObjFunction, lbCost);
          encoder.incUpdateCardinality(solver, currentObjFunction, lbCost,
                                       encodingAssumptions);
        }
      } else {
        // Incremental construction of the encoding.
        if (joinObjFunction.size() > 0)
          encoder.joinEncoding(solver, joinObjFunction, lbCost);

        // The right-hand side is constrained using assumptions.
        // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
        encoder.incUpdateCardinality(solver, currentObjFunction, lbCost,
                                     encodingAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
  return _ERROR_;
}

// Public search method
StatusCode UpMSU3::search() {
  
  printConfiguration();
  createPartitions();

  return MSU3_iterative();
}

/************************************************************************************************
 //
 // Rebuild MaxSAT solver
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  rebuildSolver : [void]  ->  [Solver *]
  |
  |  Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula.
  |
  |________________________________________________________________________________________________@*/
Solver *UpMSU3::rebuildSolver() {

  Solver *S = newSATSolver();

  reserveSATVariables(S, maxsat_formula->nVars());

  for (int i = 0; i < maxsat_formula->nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < maxsat_formula->nHard(); i++)
    S->addClause(getHardClause(i).clause);

  vec<Lit> clause;
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    clause.clear();
    Soft &s = getSoftClause(i);
    s.clause.copyTo(clause);
    for (int j = 0; j < s.relaxation_vars.size(); j++)
      clause.push(s.relaxation_vars[j]);

    S->addClause(clause);
  }

  // printf("c #PB: %d\n", maxsat_formula->nPB());
  for (int i = 0; i < maxsat_formula->nPB(); i++) {
    Encoder *enc = new Encoder(_INCREMENTAL_NONE_, _CARD_MTOTALIZER_,
                               _AMO_LADDER_, _PB_GTE_);

    // Make sure the PB is on the form <=
    if (!maxsat_formula->getPBConstraint(i)->_sign)
      maxsat_formula->getPBConstraint(i)->changeSign();

    enc->encodePB(S, maxsat_formula->getPBConstraint(i)->_lits,
                  maxsat_formula->getPBConstraint(i)->_coeffs,
                  maxsat_formula->getPBConstraint(i)->_rhs);

    // maxsat_formula->getPBConstraint(i)->print();

    delete enc;
  }

  // printf("c #Card: %d\n", maxsat_formula->nCard());
  for (int i = 0; i < maxsat_formula->nCard(); i++) {
    Encoder *enc = new Encoder(_INCREMENTAL_NONE_, _CARD_MTOTALIZER_,
                               _AMO_LADDER_, _PB_GTE_);

    if (maxsat_formula->getCardinalityConstraint(i)->_rhs == 1) {
      enc->encodeAMO(S, maxsat_formula->getCardinalityConstraint(i)->_lits);
    } else {
      enc->encodeCardinality(S,
                             maxsat_formula->getCardinalityConstraint(i)->_lits,
                             maxsat_formula->getCardinalityConstraint(i)->_rhs);
    }

    delete enc;
  }

  // if r_i is set to true than the soft clause is not satisfied
  // NOTE: this is only important if we are enumerating optimal solutions
  for (int i = 0; i < maxsat_formula->nSoft(); i++){    
    for (int j = 0; j < maxsat_formula->getSoftClause(i).clause.size(); j++){
      for (int z = 0; z < maxsat_formula->getSoftClause(i).relaxation_vars.size(); z++){
        vec<Lit> c1;
        //printf("c = %d r= %d\n",maxsat_formula->getSoftClause(i).clause.size(),maxsat_formula->getSoftClause(i).relaxation_vars.size());
        c1.push(~maxsat_formula->getSoftClause(i).clause[j]);
        c1.push(~maxsat_formula->getSoftClause(i).relaxation_vars[z]);
        S->addClause(c1);
      }
    }
  }

  return S;
}

/************************************************************************************************
 //
 // Other protected methods
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  initRelaxation : [void] ->  [void]
  |
  |  Description:
  |
  |    Initializes the relaxation variables by adding a fresh variable to the
  |    'relaxationVars' of each soft clause.
  |
  |  Post-conditions:
  |    * 'objFunction' contains all relaxation variables that were added to soft
  |      clauses.
  |
  |________________________________________________________________________________________________@*/
void UpMSU3::initRelaxation() {
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    Lit l = maxsat_formula->newLiteral();
    Soft &s = getSoftClause(i);
    s.relaxation_vars.push(l);
    s.assumption_var = l;
    objFunction.push(l);
    coeffs.push(s.weight);
  }
}

// Print MSU3 configuration.
void UpMSU3::print_MSU3_configuration() {
  printf("c |  Algorithm: %23s                                             "
         "                      |\n",
         "UpMSU3");
}

void UpMSU3::createPartitions() {
  _partitions = 0;

  for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
    soft_partitions_tmp.push();
    new (&soft_partitions_tmp[i]) vec<int>();
  }

  for (int i = 0; i < maxsat_formula->nSoft(); i++){
    int v = maxsat_formula->getSoftClause(i).getPartition();
    if (v == -1) v = maxsat_formula->nPartitions();
    assert (v >= 0 && v < soft_partitions_tmp.size());
    soft_partitions_tmp[v].push(i);
  }  

  std::vector< pair <int,int> > vect;
  for (int i = 0; i < soft_partitions_tmp.size(); i++){
    vect.push_back( std::make_pair(soft_partitions_tmp[i].size(), i) );
  }

  std::sort(vect.begin(), vect.end());
  for (int i = 0; i < vect.size(); i++){
    if (vect[i].first > 0){
      soft_partitions.push();
      new (&soft_partitions[soft_partitions.size()-1]) vec<int>();
      for (int j = 0; j < soft_partitions_tmp[vect[i].second].size(); j++){
        int v = soft_partitions_tmp[vect[i].second][j];
        soft_partitions[soft_partitions.size()-1].push(v);
      }
      soft_partitions_tmp[vect[i].second].clear();
    } else soft_partitions_tmp[vect[i].second].clear();
  }
  soft_partitions_tmp.clear();

}