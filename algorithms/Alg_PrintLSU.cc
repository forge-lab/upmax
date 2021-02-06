/*!
 * \author Ruben Martins - ruben@sat.inesc-id.pt
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2017, Ruben Martins, Vasco Manquinho, Ines Lynce
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

#include "Alg_PrintLSU.h"

using namespace openwbo;

StatusCode PrintLSU::normalSearch() {

  initRelaxation();
  solver = rebuildSolver();
  if (lower_bound == 0){

    printf("p cnf %d %d\n",maxsat_formula->nVars(), maxsat_formula->nHard()+maxsat_formula->nSoft());
    for (int i = 0; i < maxsat_formula->nHard(); i++){
      for (int j = 0; j < maxsat_formula->getHardClause(i).clause.size(); j++){
        if (sign(maxsat_formula->getHardClause(i).clause[j])) printf("-");
          printf("%d ",var(maxsat_formula->getHardClause(i).clause[j])+1);
        printf("0\n");
      }
    }

    for (int i = 0; i < maxsat_formula->nSoft(); i++){
      for (int j = 0; j < maxsat_formula->getSoftClause(i).clause.size(); j++){
        if (sign(maxsat_formula->getSoftClause(i).clause[j])) printf("-");
          printf("%d ",var(maxsat_formula->getSoftClause(i).clause[j])+1);
        printf("0\n");
      }
    }
    
  } else {

    if (maxsat_formula->getProblemType() == _WEIGHTED_) {
            if (!encoder.hasPBEncoding()){
              // check if GTE encoding will generate too many clauses
              // TODO: generalize this to all PB encodings
              if (encoder.getPBEncoding() == _PB_GTE_){
                int expected_clauses = encoder.predictPB(solver, objFunction, coeffs, lower_bound);
                if (expected_clauses >= _MAX_CLAUSES_) {
                  printf("c Warn: changing to Adder encoding.\n");
                  encoder.setPBEncoding(_PB_ADDER_);
                } else printf("c GTE auxiliary #clauses = %d\n",expected_clauses);
              }
              encoder.encodePB(solver, objFunction, coeffs, lower_bound);
            }
    } else {
      // Unweighted.
            if (!encoder.hasCardEncoding())
              encoder.encodeCardinality(solver, objFunction, lower_bound);
    }
  }
  solver->printFormula();

  return _ERROR_;
}

// Public search method
StatusCode PrintLSU::search() {
  return normalSearch();
}

/************************************************************************************************
 //
 // Rebuild MaxSAT solver
 //
 ************************************************************************************************/

/*_________________________________________________________________________________________________
  |
  |  rebuildSolver : (minWeight : int)  ->  [Solver *]
  |
  |  Description:
  |
  |    Rebuilds a SAT solver with the current MaxSAT formula.
  |    If a weight is specified, then it only considers soft clauses with weight
  |    smaller than the specified weight.
  |    NOTE: a weight is specified in the 'bmo' approach.
  |
  |________________________________________________________________________________________________@*/
Solver *PrintLSU::rebuildSolver(uint64_t min_weight) {

  vec<bool> seen;
  seen.growTo(maxsat_formula->nVars(), false);

  Solver *S = newSATSolver();

  reserveSATVariables(S, maxsat_formula->nVars());

  for (int i = 0; i < maxsat_formula->nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < maxsat_formula->nHard(); i++)
    S->addClause(maxsat_formula->getHardClause(i).clause);

  for (int i = 0; i < maxsat_formula->nPB(); i++) {
    Encoder *enc = new Encoder(_INCREMENTAL_NONE_, _CARD_MTOTALIZER_,
                               _AMO_LADDER_, _PB_GTE_);

    // Make sure the PB is on the form <=
    // if (maxsat_formula->getPBConstraint(i)->_sign)
    //  maxsat_formula->getPBConstraint(i)->changeSign();
    assert(maxsat_formula->getPBConstraint(i)->_sign);

    enc->encodePB(S, maxsat_formula->getPBConstraint(i)->_lits,
                  maxsat_formula->getPBConstraint(i)->_coeffs,
                  maxsat_formula->getPBConstraint(i)->_rhs);

    delete enc;
  }

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

  vec<Lit> clause;
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    if (maxsat_formula->getSoftClause(i).weight < min_weight)
      continue;

    clause.clear();
    maxsat_formula->getSoftClause(i).clause.copyTo(clause);

    for (int j = 0; j < maxsat_formula->getSoftClause(i).relaxation_vars.size();
         j++) {
      clause.push(maxsat_formula->getSoftClause(i).relaxation_vars[j]);
    }

    S->addClause(clause);
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
  |  initRelaxation : (objective : vec<Lit>&) (weights : vec<int>&)  ->  [void]
  |
  |  Description:
  |
  |    Initializes the relaxation variables by adding a fresh variable to the
  |    'relaxationVars' of each soft clause.
  |
  |  Post-conditions:
  |    * 'objFunction' contains all relaxation variables that were added to soft
  |       clauses.
  |    * 'coeffs' contains the weights of all soft clauses.
  |
  |________________________________________________________________________________________________@*/
void PrintLSU::initRelaxation() {
  for (int i = 0; i < maxsat_formula->nSoft(); i++) {
    Lit l = maxsat_formula->newLiteral();
    maxsat_formula->getSoftClause(i).relaxation_vars.push(l);
    objFunction.push(l);
    coeffs.push(maxsat_formula->getSoftClause(i).weight);
  }
}