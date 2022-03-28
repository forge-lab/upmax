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

#include "Alg_MSU3.h"

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
StatusCode MSU3::MSU3_iterative() {

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

  for (;;) {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True) {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printBound(newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1) {
        for (int i = 0; i < objFunction.size(); i++)
          assumptions.push(~objFunction[i]);
      } else {
        //assert(lbCost == newCost);
        printAnswer(_OPTIMUM_);
        return enumerate_opt(solver, assumptions);
        //return _OPTIMUM_;
      }
    }

    if (res == l_False) {
      lbCost++;
      nbCores++;
      if (verbosity > 0)
        printf("c LB : %-12" PRIu64 "\n", lbCost);

      if (nbSatisfiable == 0) {
        printAnswer(_UNSATISFIABLE_);
        return _UNSATISFIABLE_;
      }

      // if (lbCost == ubCost) {
      //   assert(nbSatisfiable > 0);
      //   if (verbosity > 0)
      //     printf("c LB = UB\n");
      //   printAnswer(_OPTIMUM_);
      //   return _OPTIMUM_;
      // }

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
        else
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

StatusCode MSU3::MSU3_bmo() {
    assert(orderWeights.size() > 0);

    printf("c Number of priority objectives %ld\n",orderWeights.size());

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

  vec<Lit> lastAssumptions;
  std::set<int> lastSoft;

  vec<Encoder*> bmo_encoder;
  for (int i = 0; i < orderWeights.size(); i++){
    bmo_encoder.push(new Encoder());
    bmo_encoder[bmo_encoder.size()-1]->setIncremental(_INCREMENTAL_ITERATIVE_);
  }

  //encoder.setIncremental(_INCREMENTAL_ITERATIVE_);
  int current_bmo_function = 0;

  activeSoft.growTo(maxsat_formula->nSoft(), false);
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
    coreMapping[getAssumptionLit(i)] = i;

  for (;;) {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True) {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      saveModel(solver->model);
      printBound(newCost);

      ubCost = newCost;

      if (nbSatisfiable == 1) {
        // only add assumptions for the first level
        assumptions.clear();
        lastAssumptions.clear();

        printf("c Priority function %d/%ld with weight %ld\n", current_bmo_function+1,orderWeights.size(), orderWeights[current_bmo_function]);
        for (int i = 0; i < objFunction.size(); i++){
          if (coeffs[i] == orderWeights[current_bmo_function]){
            assumptions.push(~objFunction[i]);
            lastAssumptions.push(~objFunction[i]); // this takes care of the case where the opt is 0 for this function
          }
        }
        //printf("assumptions = %d\n",assumptions.size());
      } else {
        //assert(lbCost == newCost);
        if (current_bmo_function == orderWeights.size()-1){
          printAnswer(_OPTIMUM_);
          return enumerate_opt(solver, assumptions);
        } else {
          // next function
          lbCost = 0;
          
          // reset soft clause and fix to 0 r_i that did not appear in a core
          for (int i = 0; i < maxsat_formula->nSoft(); i++) {
            if (!activeSoft[i]){
                if (maxsat_formula->getSoftClause(i).weight == orderWeights[current_bmo_function]){
                  vec<Lit> uclause;
                  uclause.push(~getRelaxationLit(i));
                  solver->addClause(uclause);
                }
            } else activeSoft[i] = false;
          }

          // fix the values for the last cardinality constraint
          for (int i = 0; i < lastAssumptions.size(); i++){
            vec<Lit> uclause;
            uclause.push(lastAssumptions[i]);
            solver->addClause(uclause);
          }

          lastAssumptions.clear();
          lastSoft.clear();

          current_bmo_function++;
          assumptions.clear();
          encodingAssumptions.clear();

          printf("c Priority function %d/%ld with weight %ld\n", current_bmo_function+1,orderWeights.size(),orderWeights[current_bmo_function]);
          for (int i = 0; i < objFunction.size(); i++){
            if (coeffs[i] == orderWeights[current_bmo_function]){
              assumptions.push(~objFunction[i]);
              lastAssumptions.push(~objFunction[i]); // this takes care of the case where the opt is 0 for this function
            }
          }
        }
        //return _OPTIMUM_;
      }
    }

    if (res == l_False) {
      lbCost++;
      nbCores++;
      if (verbosity > 0)
        printf("c LB : %-12" PRIu64 "\n", lbCost);

      if (nbSatisfiable == 0) {
        printAnswer(_UNSATISFIABLE_);
        return _UNSATISFIABLE_;
      }

      // if (lbCost == ubCost) {
      //   assert(nbSatisfiable > 0);
      //   if (verbosity > 0)
      //     printf("c LB = UB\n");
      //   printAnswer(_OPTIMUM_);
      //   return _OPTIMUM_;
      // }

      sumSizeCores += solver->conflict.size();

      if (solver->conflict.size() == 0) {
        printAnswer(_UNSATISFIABLE_);
        return _UNSATISFIABLE_;
      }

      joinObjFunction.clear();
      for (int i = 0; i < solver->conflict.size(); i++) {
        if (coreMapping.find(solver->conflict[i]) != coreMapping.end()) {
          //if (activeSoft[coreMapping[solver->conflict[i]]]) continue;
          //printf("active soft = %d\n",coreMapping[solver->conflict[i]]);
          assert(!activeSoft[coreMapping[solver->conflict[i]]]);
          activeSoft[coreMapping[solver->conflict[i]]] = true;
          lastSoft.insert(coreMapping[solver->conflict[i]]);
          joinObjFunction.push(
              getRelaxationLit(coreMapping[solver->conflict[i]]));
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < maxsat_formula->nSoft(); i++) {
        if (activeSoft[i])
          currentObjFunction.push(getRelaxationLit(i));
        else {
          if (coeffs[i] == orderWeights[current_bmo_function]) 
            assumptions.push(~getAssumptionLit(i));
        }
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
               objFunction.size());

      if (!bmo_encoder[current_bmo_function]->hasCardEncoding()) {
        if (lbCost != (unsigned)currentObjFunction.size()) {
          bmo_encoder[current_bmo_function]->buildCardinality(solver, currentObjFunction, lbCost);
          bmo_encoder[current_bmo_function]->incUpdateCardinality(solver, currentObjFunction, lbCost,
                                       encodingAssumptions);
        }
      } else {
        // Incremental construction of the encoding.
        if (joinObjFunction.size() > 0)
          bmo_encoder[current_bmo_function]->joinEncoding(solver, joinObjFunction, lbCost);

        // The right-hand side is constrained using assumptions.
        // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
        bmo_encoder[current_bmo_function]->incUpdateCardinality(solver, currentObjFunction, lbCost,
                                     encodingAssumptions);
      }
      
      lastAssumptions.clear();
      encodingAssumptions.copyTo(lastAssumptions);
      if (lastAssumptions.size() == 0){
        // if there is nothing to restrict use the literals of the function
        currentObjFunction.copyTo(lastAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }
  return _ERROR_;
}

// Public search method
StatusCode MSU3::search() {
  if (maxsat_formula->getProblemType() == _WEIGHTED_) {

     bool is_bmo = isBMO();
     if (is_bmo){
      printConfiguration();
      return MSU3_bmo();
     } else {

      if(print) {
        printf("Error: Currently algorithm MSU3 does not support weighted "
               "MaxSAT instances.\n");
        printf("s UNKNOWN\n");
      }
      throw MaxSATException(__FILE__, __LINE__, "MSU3 does not support weighted");
      return _UNKNOWN_;
     }
  }

  if (incremental_strategy == _INCREMENTAL_ITERATIVE_) {
    if (encoding != _CARD_TOTALIZER_) {
      if(print) {
        printf("Error: Currently iterative encoding in PartMSU3 only "
             "supports the Totalizer encoding.\n");
        printf("s UNKNOWN\n");
      }
      throw MaxSATException(__FILE__, __LINE__, "PartMSU3 only supports Totalizer");
      return _UNKNOWN_;
    }
  }

  printConfiguration();
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
Solver *MSU3::rebuildSolver() {

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
void MSU3::initRelaxation() {
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
void MSU3::print_MSU3_configuration() {
  printf("c |  Algorithm: %23s                                             "
         "                      |\n",
         "MSU3");
}
