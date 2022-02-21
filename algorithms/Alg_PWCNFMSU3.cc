/*!
 * \author Ruben Martins - rubenm@andrew.cmu.edu
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2022, Ruben Martins, Vasco Manquinho, Ines Lynce
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
        printAnswer(_OPTIMUM_);
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

StatusCode PWCNFMSU3::search_part() {
  
  int current_partition = 0;
  int remaining_partitions = 0;

  vec< vec<Lit> > assumptions_partitions;

  lbool res = l_True;
  initRelaxation();
  solver = rebuildSolver();
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

    if (_limit != -1)
      solver->setConfBudget(_limit);
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
          //solver->setConfBudget(-1);
          solver->budgetOff();
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

    if (_limit != -1 && remaining_partitions != 1)
      solver->setConfBudget(_limit);
    res = searchSATSolver(solver, assumptions);
    //if (res == l_True) {
    if (res != l_False) {
      if (res != l_True) assert(remaining_partitions != 1);
      else {
        nbSatisfiable++;
        uint64_t newCost = computeCostModel(solver->model);
        if (newCost < ubCost){
          saveModel(solver->model);
          printBound(newCost);
          ubCost = newCost;
        }
      }

      if (remaining_partitions == 1){
        printAnswer(_OPTIMUM_);
        //return _OPTIMUM_;
        return enumerate_opt(solver, assumptions);
      } else {
        // merge next partitions
        encodingAssumptions.clear();
        current_partition = merge_part(_mode, encodingAssumptions);
        remaining_partitions--;
        if (remaining_partitions == 1 &&  _limit != -1){
          // disable conflict limit
          //solver->setConfBudget(-1);
          solver->budgetOff();
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


StatusCode PWCNFMSU3::bmo_single() {
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


StatusCode PWCNFMSU3::bmo_part() {
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

  std::set<int> bmo_partitions;
  std::set<int> bmo_current_partitions;

  vec<Encoder*> bmo_encoder;
  for (int i = 0; i < orderWeights.size(); i++){
    bmo_encoder.push(new Encoder());
    bmo_encoder[bmo_encoder.size()-1]->setIncremental(_INCREMENTAL_ITERATIVE_);
  }

  //encoder.setIncremental(_INCREMENTAL_ITERATIVE_);
  int current_bmo_function = 0;

  activeSoft.growTo(maxsat_formula->nSoft(), false);
  for (int i = 0; i < maxsat_formula->nSoft(); i++){
    coreMapping[getAssumptionLit(i)] = i;
    if (getSoftClause(i).weight == orderWeights[current_bmo_function]){
      bmo_partitions.insert(getSoftClause(i).getPartition());
    }
  }

  int current_partition = *bmo_partitions.begin();
  bmo_partitions.erase(current_partition);
  bmo_current_partitions.insert(current_partition);

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
        printf("c Partition id= %d ; Partitions left= %d\n",current_partition,bmo_partitions.size());
        for (int i = 0; i < objFunction.size(); i++){
          if (coeffs[i] == orderWeights[current_bmo_function] && getSoftClause(i).getPartition() == current_partition){
            assumptions.push(~objFunction[i]);
            lastAssumptions.push(~objFunction[i]); // this takes care of the case where the opt is 0 for this function
          }
        }
        //printf("assumptions = %d\n",assumptions.size());
      } else {
        //assert(lbCost == newCost);
        if (current_bmo_function == orderWeights.size()-1 && bmo_partitions.size() == 0){
          printAnswer(_OPTIMUM_);
          return enumerate_opt(solver, assumptions);
        } else {
          if (bmo_partitions.size() == 0){

            // next function
            lbCost = 0;
            bmo_partitions.clear();
            bmo_current_partitions.clear();
            
            // reset soft clause and fix to 0 r_i that did not appear in a core
            for (int i = 0; i < maxsat_formula->nSoft(); i++) {
              if (!activeSoft[i]){
                  if (maxsat_formula->getSoftClause(i).weight == orderWeights[current_bmo_function]){
                    vec<Lit> uclause;
                    uclause.push(~getRelaxationLit(i));
                    solver->addClause(uclause);
                  }
              } else activeSoft[i] = false;
              if (getSoftClause(i).weight == orderWeights[current_bmo_function+1]){
                  bmo_partitions.insert(getSoftClause(i).getPartition());
              }
            }

            current_partition = *bmo_partitions.begin();
            bmo_partitions.erase(current_partition);
            bmo_current_partitions.insert(current_partition);


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
            printf("c Partition id= %d ; Partitions left= %d\n",current_partition,bmo_partitions.size());
            for (int i = 0; i < objFunction.size(); i++){
              if (coeffs[i] == orderWeights[current_bmo_function] &&  getSoftClause(i).getPartition() == current_partition){
                assumptions.push(~objFunction[i]);
                lastAssumptions.push(~objFunction[i]); // this takes care of the case where the opt is 0 for this function
              }
            }
          } else {
            // next partition
            current_partition = *bmo_partitions.begin();
            bmo_partitions.erase(current_partition);
            bmo_current_partitions.insert(current_partition);

            printf("c Partition id= %d ; Partitions left= %d\n",current_partition,bmo_partitions.size());
            for (int i = 0; i < objFunction.size(); i++){
              if (coeffs[i] == orderWeights[current_bmo_function] && getSoftClause(i).getPartition() == current_partition){
                assumptions.push(~objFunction[i]);
                lastAssumptions.push(~objFunction[i]); // this takes care of the case where the opt is 0 for this function
              }
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
          if (coeffs[i] == orderWeights[current_bmo_function] && bmo_current_partitions.find(getSoftClause(i).getPartition())!= bmo_current_partitions.end()) 
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

StatusCode PWCNFMSU3::search() {


   if (maxsat_formula->getProblemType() == _WEIGHTED_) {
  
    bool is_bmo = isBMO();
     if (is_bmo){
      print_PWCNFMSU3_configuration();
      createPartitions();
      printf("c Number of partitions = %d\n",_partitions);

      if (_partitions <= 1)
        return bmo_single();
        else {
          return bmo_part();
        }
     } else {

    if(print) {
      printf("Error: Currently algorithm MSU3 does not support weighted MaxSAT "
             "instances.\n");
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
    coeffs.push(getSoftClause(i).weight);
  }
}

void PWCNFMSU3::print_PWCNFMSU3_configuration() {
  printf("c |  Algorithm: %23s                                             "
         "                      |\n",
         "PWCNFMSU3");
  printf("c merge heuristic= %d\n",_mode);
  printf("c conflict limit= %d\n",_limit); 
}
