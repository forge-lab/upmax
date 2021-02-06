/*!
 * \author Ruben Martins - rubenm@andrew.cmu.edu
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

#include "Alg_PWCNFMSU3.h"

#include <gmpxx.h>
#include <iostream>

#include <algorithm>
#include <list>
#include <unordered_map>

#define NO_PAIR -1
#define ERASE -2

#define SPARSITY_HEURISTIC 0.04
#define CLAUSE_LIMIT 1000000
#define PARTITION_RATIO 0.8

using namespace openwbo;

StatusCode PWCNFMSU3::search_single() {

  
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
        assert(lbCost == newCost);
        return enumerate_opt(solver, assumptions);
        //printAnswer(_OPTIMUM_);
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

      // if (verbosity > 0)
      //   printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
      //          objFunction.size());

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

#define REUSE

int PWCNFMSU3::merge_part(int mode, vec<Lit>& encodingAssumptions) {

  int a = INT32_MAX;
  int b = INT32_MAX;
  int p1 = -1;
  int p2 = -1;

  if (mode == _SIZE_){
    for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
      if (soft_partitions[i].size() < a && active_partitions[i]){
        b = a;
        a = soft_partitions[i].size();
        p2 = p1;
        p1 = i;
      } else if (soft_partitions[i].size() < b && active_partitions[i]){
        b = soft_partitions[i].size();
        p2 = i;
      }
    }
  } else if (mode == _CORES_ ){
    for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
      if (lb_partitions[i] < a && active_partitions[i]){
        b = a;
        a = lb_partitions[i];
        p2 = p1;
        p1 = i;
      } else if (lb_partitions[i] < b && active_partitions[i]){
        b = lb_partitions[i];
        p2 = i;
      }
    }
  }

  assert (p1 != -1 && p2 != -1);

  // printf("c Partition #%d LB=%llu size=%d\n",p1+1,lb_partitions[p1],soft_partitions[p1].size());
  // printf("c Partition #%d LB=%llu size=%d\n",p2+1,lb_partitions[p2],soft_partitions[p2].size());
  printf("c Merging partition #%d with #%d | Size= %d | LB= %llu\n",p1+1,p2+1,soft_partitions[p2].size()+soft_partitions[p1].size(),lb_partitions[p2]+lb_partitions[p1]);

    // merge a with b
    lb_partitions[p2] = lb_partitions[p1]+lb_partitions[p2];
    for (int i = 0; i < soft_partitions[p1].size(); i++){
      soft_partitions[p2].push(soft_partitions[p1][i]);
    }
    
    vec<Lit> currentObjFunction;  

    if (!encoder_partitions[p2]->hasCardEncoding()) {
      for (int i = 0; i < soft_partitions[p2].size(); i++){
        if (activeSoft[soft_partitions[p2][i]]){
          currentObjFunction.push(getRelaxationLit(soft_partitions[p2][i]));
        }      
      }
      
      for (int i = 0; i < soft_partitions[p1].size(); i++){
        if (activeSoft[soft_partitions[p1][i]]){
          lits_partitions[p2].push(getRelaxationLit(soft_partitions[p1][i]));
        }
      }


        assert(lb_partitions[p2] <= (unsigned)currentObjFunction.size());
        if (lb_partitions[p2] != (unsigned)currentObjFunction.size() && currentObjFunction.size() > 0) {
          encoder_partitions[p2]->buildCardinality(solver, currentObjFunction, lb_partitions[p2]);
          encoder_partitions[p2]->incUpdateCardinality(solver, currentObjFunction, lb_partitions[p2],
                                       encodingAssumptions);
        }

        // there was no encoding created
        lits_partitions[p1].clear();

      } else {
        vec<Lit> joinObjFunction;

#ifndef REUSE
        currentObjFunction.clear();
        for (int i = 0; i < soft_partitions[p2].size(); i++){
          if (activeSoft[soft_partitions[p2][i]])
            currentObjFunction.push(getRelaxationLit(soft_partitions[p2][i]));
        } 

        for (int i = 0; i < soft_partitions[p1].size(); i++){
          if (activeSoft[soft_partitions[p1][i]])
            joinObjFunction.push(getRelaxationLit(soft_partitions[p1][i]));
        }
#else
        // Merging with the outputs preserves the structure of the cardinality constraint
        // This should be more efficient than merging with the original literals
        // TODO: would also require the outputs to grow -- current encoding does not suppor this?
        currentObjFunction.clear();
        for (int i = 0; i < lits_partitions[p2].size(); i++){
            currentObjFunction.push(lits_partitions[p2][i]);
        }

        if (encoder_partitions[p1]->hasCardEncoding()){
          // update the constraint to have the maximum needed outputs
          // TODO: this is a source of inefficiency
          vec<Lit> tmp;
          encoder_partitions[p1]->incUpdateCardinality(solver, lits_partitions[p1], encoder_partitions[p1]->outputs().size(),
                                       tmp);

          for (int i = 0; i < encoder_partitions[p1]->outputs().size(); i++){
              joinObjFunction.push(encoder_partitions[p1]->outputs()[i]);
              currentObjFunction.push(encoder_partitions[p1]->outputs()[i]);
              lits_partitions[p2].push(encoder_partitions[p1]->outputs()[i]);
          }

        } else {
          for (int i = 0; i < soft_partitions[p1].size(); i++){
            if (activeSoft[soft_partitions[p1][i]]){
              joinObjFunction.push(getRelaxationLit(soft_partitions[p1][i]));
              currentObjFunction.push(getRelaxationLit(soft_partitions[p1][i]));
              lits_partitions[p2].push(getRelaxationLit(soft_partitions[p1][i]));
            }
          }
          lits_partitions[p1].clear();
        }

#endif                  

        // Incremental construction of the encoding.
        if (joinObjFunction.size() > 0)
          encoder_partitions[p2]->joinEncoding(solver, joinObjFunction, lb_partitions[p2]);

        // The right-hand side is constrained using assumptions.
        // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
        encoder_partitions[p2]->incUpdateCardinality(solver, currentObjFunction, lb_partitions[p2],
                                     encodingAssumptions);

}

    //soft_partitions[p1].clear();
    active_partitions[p1] = false;
    merged_partitions[p2].push(p1);

// The below does not work since the outputs need to grow in advance with the current code
// #ifdef REUSE
//     // increase the size of the cardinality constraints from the previous partitions if needed
//     for (int i = 0; i < merged_partitions[p2].size(); i++){
//       vec<Lit> obj;
//       vec<Lit> assp;
//       for (int j = 0; j < lits_partitions[merged_partitions[p2][i]].size(); j++){
//         obj.push(lits_partitions[merged_partitions[p2][i]][j]);
//       }
//       if (obj.size() > 0){
//         printf("Updating #Partition:%d\n",merged_partitions[p2][i]+1);
//         uint64_t lb = lits_partitions[merged_partitions[p2][i]].size();
//         if(lb_partitions[p2] < lb) lb = lb_partitions[p2];
//         encoder_partitions[merged_partitions[p2][i]]->incUpdateCardinality(solver, obj, lb,
//                                        assp);
//       }
//     }
// #endif

  return p2;
}

StatusCode PWCNFMSU3::enumerate_opt(Solver* solver, vec<Lit>& assumptions) {

  _n_opt_sols = 1;

  if (_all_opt_sols){

    // formula is in a satisfiable state
    assert (solver->model.size() > 0);
    vec<Lit> block_model;

    lbool res = l_True;
    while (res == l_True){
      // block model
      block_model.clear();
      for (int i = 0; i < maxsat_formula->nSoft(); i++) {
        for (int j = 0; j < maxsat_formula->getSoftClause(i).relaxation_vars.size(); j++){
          int v = var(maxsat_formula->getSoftClause(i).relaxation_vars[j]);
          block_model.push(mkLit(v,solver->model[v]==l_True));
        }
      }
      solver->addClause(block_model);

      res = searchSATSolver(solver, assumptions);
      _n_opt_sols++;
    }
  }

  printf("c Optimal Solutions: %d\n",_n_opt_sols);
  return _OPTIMUM_;

}


StatusCode PWCNFMSU3::search_part() {
  
  int current_partition = 0;
  int remaining_partitions = 0;

  vec< vec<Lit> > assumptions_partitions;

  lbool res = l_True;
  initRelaxation();
  solver = rebuildSolver();
  if (_limit != -1)
    solver->setConfBudget(_limit);
  vec<Lit> assumptions;
  vec<Lit> joinObjFunction;
  vec<Lit> currentObjFunction;
  vec<Lit> encodingAssumptions;
  encoder.setIncremental(_INCREMENTAL_ITERATIVE_);
  uint64_t lbCost_local = 0;
  remaining_partitions = _partitions;

  for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
    lits_partitions.push();
    new (&lits_partitions[i]) vec<Lit>();

    assumptions_partitions.push();
    new (&assumptions_partitions[i]) vec<Lit>();

    if (soft_partitions[i].size() > 0) active_partitions.push(true);
    else active_partitions.push(false);

    merged_partitions.push();
    new (&merged_partitions[i]) vec<int>();

    encoder_partitions.push(new Encoder(incremental_strategy, encoding));
    lb_partitions.push(0);
  }

  activeSoft.growTo(maxsat_formula->nSoft(), false);
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
    coreMapping[getAssumptionLit(i)] = i;

  while (soft_partitions[current_partition].size() == 0 && current_partition < soft_partitions.size()){
      current_partition++;
  }

  // First phase: saturation of each partition
  printf("c [Begin] Saturation phase\n");
  for (;;) {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True) {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost){
        saveModel(solver->model);
        printBound(newCost);
        ubCost = newCost;
      }

      //consider next partition
      current_partition++;
      while (soft_partitions[current_partition].size() == 0 && current_partition < soft_partitions.size()){
        current_partition++;
      }

      if (current_partition != soft_partitions.size()){
        assumptions.clear();
        lbCost_local = 0;
        printf("c Partition #%d | Size= %d\n",current_partition+1,soft_partitions[current_partition].size());
        // consider next partition        
        for (int i = 0; i < soft_partitions[current_partition].size(); i++)
         assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);

      } else {
        // First phase: end
        // Second phase: merging
        printf("c [End] Saturation phase: LB= %llu\n",lbCost);
        break;
      }


    } else if (res == l_False) {
      lbCost++;
      lbCost_local++;
      lb_partitions[current_partition]++;
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
          lits_partitions[current_partition].push(getRelaxationLit(coreMapping[solver->conflict[i]]));
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < soft_partitions[current_partition].size(); i++){
        if (!activeSoft[soft_partitions[current_partition][i]])
        //   currentObjFunction.push(getRelaxationLit(soft_partitions[current_partition-1][i]));
        // else
          assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
      }

#ifdef REUSE
      for (int i = 0; i < lits_partitions[current_partition].size(); i++){
        currentObjFunction.push(lits_partitions[current_partition][i]);
      }
#else
      for (int i = 0; i < soft_partitions[current_partition].size(); i++){
        if (activeSoft[soft_partitions[current_partition][i]])
          currentObjFunction.push(getRelaxationLit(soft_partitions[current_partition][i]));
      }
#endif         

      // if (verbosity > 0)
      //   printf("c Relaxed soft clauses %d / %d\n", currentObjFunction.size(),
      //          soft_partitions[current_partition-1].size());

      if (!encoder_partitions[current_partition]->hasCardEncoding()) {

        if (lbCost_local != (unsigned)currentObjFunction.size()) {
          encoder_partitions[current_partition]->buildCardinality(solver, currentObjFunction, lbCost_local);
          encoder_partitions[current_partition]->incUpdateCardinality(solver, currentObjFunction, lbCost_local,
                                       encodingAssumptions);
        }
      } else {
        // Incremental construction of the encoding.
        if (joinObjFunction.size() > 0)
          encoder_partitions[current_partition]->joinEncoding(solver, joinObjFunction, lbCost_local);

        // The right-hand side is constrained using assumptions.
        // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
        encoder_partitions[current_partition]->incUpdateCardinality(solver, currentObjFunction, lbCost_local,
                                     encodingAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }

  if (_mode == _SATURATION_ONLY_)
    return _UNKNOWN_;

  // Second phase: choose two partitions, merge, search
  encodingAssumptions.clear();
  current_partition = merge_part(_mode, encodingAssumptions);
  remaining_partitions--;
  if (remaining_partitions == 1 &&  _limit != -1){
          // disable conflict limit
          solver->setConfBudget(-1);
  }
  printf("c Remaining #Partitions= %d\n",remaining_partitions);

  assumptions.clear();
  for (int i = 0; i < soft_partitions[current_partition].size(); i++){
        if (!activeSoft[soft_partitions[current_partition][i]])
          assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
  }

  for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);

  
  for (;;) {

    res = searchSATSolver(solver, assumptions);
    if (res == l_True) {
      nbSatisfiable++;
      uint64_t newCost = computeCostModel(solver->model);
      if (newCost < ubCost){
        saveModel(solver->model);
        printBound(newCost);
        ubCost = newCost;
      }

      if (remaining_partitions == 1){
        //printAnswer(_OPTIMUM_);
        //return _OPTIMUM_;
        return enumerate_opt(solver, assumptions);
      } else {
        // merge next partitions
        encodingAssumptions.clear();
        current_partition = merge_part(_mode, encodingAssumptions);
        remaining_partitions--;
        if (remaining_partitions == 1 &&  _limit != -1){
          // disable conflict limit
          solver->setConfBudget(-1);
        }
        printf("c Remaining #Partitions= %d\n",remaining_partitions);

        assumptions.clear();
        for (int i = 0; i < soft_partitions[current_partition].size(); i++){
              if (!activeSoft[soft_partitions[current_partition][i]])
                assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
        }

        for (int i = 0; i < encodingAssumptions.size(); i++)
              assumptions.push(encodingAssumptions[i]);

      }

    } else if (res == l_False) {
      lbCost++;
      lb_partitions[current_partition]++;
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
          lits_partitions[current_partition].push(getRelaxationLit(coreMapping[solver->conflict[i]]));
          //printf("%d ",var(lits_partitions[current_partition][lits_partitions[current_partition].size()-1])+1);
        }
      }

      currentObjFunction.clear();
      assumptions.clear();
      for (int i = 0; i < soft_partitions[current_partition].size(); i++){
        if (!activeSoft[soft_partitions[current_partition][i]])
          assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[current_partition][i]).assumption_var);
      }


#ifdef REUSE
      //printf("***");
      for (int i = 0; i < lits_partitions[current_partition].size(); i++){
        currentObjFunction.push(lits_partitions[current_partition][i]);
        //printf("%d ",var(lits_partitions[current_partition][i])+1);
      }
      //printf("\n");

      // printf("joinObjFunction.size() %d\n",joinObjFunction.size());
      // for (int i = 0; i < joinObjFunction.size(); i++){
      //   currentObjFunction.push(joinObjFunction[i]);
      // }

      // printf("+++");
      // for (int i = 0; i < soft_partitions[current_partition].size(); i++){
      //   if (activeSoft[soft_partitions[current_partition][i]]){
      //     //currentObjFunction.push(getRelaxationLit(soft_partitions[current_partition][i]));
      //     printf("%d ",var(getRelaxationLit(soft_partitions[current_partition][i]))+1);
      //   }
      // }
      // printf("\n");
#else
      //printf("+++");
      for (int i = 0; i < soft_partitions[current_partition].size(); i++){
        if (activeSoft[soft_partitions[current_partition][i]]){
          currentObjFunction.push(getRelaxationLit(soft_partitions[current_partition][i]));
          //printf("%d ",var(getRelaxationLit(soft_partitions[current_partition][i]))+1);
        }
      }
      //printf("\n");
#endif  


      if (!encoder_partitions[current_partition]->hasCardEncoding()) {
        if (lb_partitions[current_partition] != (unsigned)currentObjFunction.size()) {
          encoder_partitions[current_partition]->buildCardinality(solver, currentObjFunction, lb_partitions[current_partition]);
          encoder_partitions[current_partition]->incUpdateCardinality(solver, currentObjFunction, lb_partitions[current_partition],
                                       encodingAssumptions);
        }
      } else {
        // Incremental construction of the encoding.

        if (joinObjFunction.size() > 0)
          encoder_partitions[current_partition]->joinEncoding(solver, joinObjFunction, lb_partitions[current_partition]);

        // The right-hand side is constrained using assumptions.
        // NOTE: 'encodingAsssumptions' is modified in 'incrementalUpdate'.
        encoder_partitions[current_partition]->incUpdateCardinality(solver, currentObjFunction, lb_partitions[current_partition],
                                     encodingAssumptions);
      }

      for (int i = 0; i < encodingAssumptions.size(); i++)
        assumptions.push(encodingAssumptions[i]);
    }
  }

  return _ERROR_;
  
}

void PWCNFMSU3::createPartitions() {
  _partitions = 0;

  for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
    soft_partitions.push();
    new (&soft_partitions[i]) vec<int>();
  }

  for (int i = 0; i < maxsat_formula->nSoft(); i++){
    int v = maxsat_formula->getSoftClause(i).getPartition();
    if (v == -1) v = maxsat_formula->nPartitions();
    soft_partitions[v].push(i);
  }  

  for (int i = 0; i < maxsat_formula->nPartitions()+1; i++){
    if (soft_partitions[i].size() > 0)
      _partitions++;
  }

}

StatusCode PWCNFMSU3::search() {
  if (maxsat_formula->getProblemType() == _WEIGHTED_) {
    if(print) {
      printf("Error: Currently algorithm MSU3 does not support weighted MaxSAT "
             "instances.\n");
      printf("s UNKNOWN\n");
    }
    throw MaxSATException(__FILE__, __LINE__, "MSU3 does not support weighted");
    return _UNKNOWN_;
  }

  if (incremental_strategy == _INCREMENTAL_ITERATIVE_) {
    if (encoding != _CARD_TOTALIZER_) {
      if(print) {
        printf("Error: Currently iterative encoding in PartMSU3 only "
               "supports the Totalizer encoding.\n");
        printf("s UNKNOWN\n");
      }
      throw MaxSATException(__FILE__, __LINE__, "MSU3 only supports totalizer");
      return _UNKNOWN_;
    }

    print_PWCNFMSU3_configuration();
    createPartitions();
    printf("c Number of partitions = %d\n",_partitions);

    if (_partitions <= 1)
      return search_single();
    else 
      return search_part();
  } else {
    if(print) {
      printf("Error: No incremental strategy.\n");
      printf("s UNKNOWN\n");
    }
    throw MaxSATException(__FILE__, __LINE__, "No incremental strategy");
    return _UNKNOWN_;
  }
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
Solver *PWCNFMSU3::rebuildSolver() {
  Solver *S = newSATSolver();

  reserveSATVariables(S, maxsat_formula->nVars());

  for (int i = 0; i < maxsat_formula->nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < maxsat_formula->nHard(); i++)
    S->addClause(getHardClause(i).clause);

  vec<Lit> clause;
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    clause.clear();
    getSoftClause(i).clause.copyTo(clause);
    for (int j = 0; j < getSoftClause(i).relaxation_vars.size(); j++)
      clause.push(getSoftClause(i).relaxation_vars[j]);

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
void PWCNFMSU3::initRelaxation() {
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    Lit l = maxsat_formula->newLiteral();
    getSoftClause(i).relaxation_vars.push(l);
    getSoftClause(i).assumption_var = l;
    objFunction.push(l);
  }
}

void PWCNFMSU3::print_PWCNFMSU3_configuration() {
  printf("c |  Algorithm: %23s                                             "
         "                      |\n",
         "PWCNFMSU3");
  printf("c merge heuristic= %d\n",_mode);
  printf("c conflict limit= %d\n",_limit); 
}
