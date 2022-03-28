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

#include "Alg_UpWMSU3.h"

using namespace upmax;

/*_________________________________________________________________________________________________
  |
  |  UpWMSU3_none : [void] ->  [void]
  |
  |  Description:
  |
  |    Non-incremental UpWMSU3 algorithm.
  |    Similar to the MSU3 algorithm but uses PB constraints instead of
  |    cardinality constraints.
  |
  |  For further details see:
  |    *  Joao Marques-Silva, Jordi Planes: On Using Unsatisfiability for
  |       Solving Maximum Satisfiability. CoRR abs/0712.1097 (2007)
  |
  |  Post-conditions:
  |    * 'ubCost' is updated.
  |    * 'lbCost' is updated.
  |    * 'nbSatisfiable' is updated.
  |    * 'nbCores' is updated.
  |
  |________________________________________________________________________________________________@*/
StatusCode UpWMSU3::UpWMSU3_none()
{
  //nbInitialVariables = maxsat_formula->nVars();
  lbool res = l_True;

  initRelaxation();
  solver = rebuildSolver();
  encoder.setIncremental(_INCREMENTAL_NONE_);

  activeSoft.growTo(maxsat_formula->nSoft(), false);
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
    coreMapping[maxsat_formula->getSoftClause(i).assumption_var] = i;

  assumptions.clear();

  _current_partition = 0;
  initAssumptionsPartition(assumptions);
  printf("c Partition #%d= %d\n",_current_partition+1,soft_partitions[_current_partition].size());

  for (;;)
  {

    if (_limit != -1 && _current_partition+1 != _partitions)
      solver->setConfBudget(_limit);
    else
      solver->budgetOff();
    res = searchSATSolver(solver, assumptions);
    if (res != l_False)
    {
      if (res == l_True){
        nbSatisfiable++;
        uint64_t newCost = computeCostModel(solver->model);
        if (newCost < ubCost || nbSatisfiable == 1)
        {
          saveModel(solver->model);
          printf("o %" PRIu64 "\n", newCost);
          ubCost = newCost;
        }

        if (ubCost == 0)
          //|| lbCost == ubCost ||
          //  (currentWeight == 1 && nbSatisfiable > 1))
        {
          assert(nbSatisfiable > 0);
          printAnswer(_OPTIMUM_);
          return _OPTIMUM_;
        }
      }

      _current_partition++;

      // for (int i = 0; i < maxsat_formula->nSoft(); i++)
      //   if (maxsat_formula->getSoftClause(i).weight >= currentWeight && !activeSoft[i])
      //     assumptions.push(~maxsat_formula->getSoftClause(i).assumption_var);

      if (_current_partition == _partitions){
        printAnswer(_OPTIMUM_);
        return _OPTIMUM_; 
      } else {

        printf("c Partition #%d= %d\n",_current_partition+1,soft_partitions[_current_partition].size());
        initAssumptionsPartition(assumptions);

      }



    } else {

      nbCores++;

      // // Assumes that the first SAT call is done with the soft clauses disabled.
      // if (nbSatisfiable == 0)
      // {
      //   printAnswer(_UNSATISFIABLE_);
      //   exit(_UNSATISFIABLE_);
      // }

      // if (lbCost == ubCost)
      // {
      //   assert(nbSatisfiable > 0); // Otherwise, the problem is UNSAT.
      //   if (verbosity > 0) printf("c LB = UB\n");
      //   printAnswer(_OPTIMUM_);
      //   exit(_OPTIMUM_);
      // }

      sumSizeCores += solver->conflict.size();
      if (solver->conflict.size() == 0) nbCores--;

      for (int i = 0; i < solver->conflict.size(); i++)
      {
        int index_soft = coreMapping[solver->conflict[i]];
        assert(!activeSoft[index_soft]);
        activeSoft[index_soft] = true;
      }

      objFunction.clear();
      coeffs.clear();
      assumptions.clear();
      for (int i = 0; i < maxsat_formula->nSoft(); i++)
      {
        // If a soft clause is active then is added to the 'currentObjFunction'
        if (activeSoft[i])
        {
          objFunction.push(maxsat_formula->getSoftClause(i).relaxation_vars[0]);
          coeffs.push(maxsat_formula->getSoftClause(i).weight);
        }
        // Otherwise, the soft clause is not relaxed and the assumption is used
        // to get an unsat core.
        else if (maxsat_formula->getSoftClause(i).weight >= currentWeight && _activeSoftPartition[i])
          assumptions.push(~maxsat_formula->getSoftClause(i).assumption_var);
      }

      if (verbosity > 0)
        printf("c Relaxed soft clauses %d / %d\n", objFunction.size(), maxsat_formula->nSoft());

      delete solver;
      solver = rebuildSolver();

      lbCost++;
      while (!subsetSum(coeffs, lbCost))
      {
        lbCost++;
      }

      if (verbosity > 0) printf("c LB : %-12" PRIu64 "\n", lbCost);
      encoder.encodePB(solver, objFunction, coeffs, lbCost);
    }
  }

  return _ERROR_;
}

// Public search method
StatusCode UpWMSU3::search()
{
  if (maxsat_formula->getProblemType() == _UNWEIGHTED_)
  {
    printf("Error: Currently algorithm UpWMSU3 does not support unweighted MaxSAT"
           " instances.\n");
    printf("s UNKNOWN\n");
    return _ERROR_;
  }

  if (bmo_strategy) is_bmo = isBMO();
  printConfiguration();
  if (!is_bmo) currentWeight = 1; // No weight strategy none.

  createPartitions();
  _activeSoftPartition.growTo(maxsat_formula->nSoft(), false);
  _partitions = soft_partitions.size();
  printf("c #Soft Partitions = %d\n",_partitions);

  return UpWMSU3_none();
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
Solver *UpWMSU3::rebuildSolver()
{

  Solver *S = newSATSolver();

  for (int i = 0; i < maxsat_formula->nVars(); i++)
    newSATVariable(S);

  for (int i = 0; i < maxsat_formula->nHard(); i++)
    S->addClause(maxsat_formula->getHardClause(i).clause);

  vec<Lit> clause;
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
  {
    clause.clear();
    maxsat_formula->getSoftClause(i).clause.copyTo(clause);
    for (int j = 0; j < maxsat_formula->getSoftClause(i).relaxation_vars.size(); j++)
      clause.push(maxsat_formula->getSoftClause(i).relaxation_vars[j]);

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
void UpWMSU3::initRelaxation()
{
  for (int i = 0; i < maxsat_formula->nSoft(); i++)
  {
    Lit l = maxsat_formula->newLiteral();
    maxsat_formula->getSoftClause(i).relaxation_vars.push(l);
    maxsat_formula->getSoftClause(i).assumption_var = l;
  }
}

void UpWMSU3::initAssumptionsPartition(vec<Lit> &assumps){
    for (int i = 0; i < soft_partitions[_current_partition].size(); i++){
      if (!activeSoft[soft_partitions[_current_partition][i]])
        assumptions.push(~maxsat_formula->getSoftClause(soft_partitions[_current_partition][i]).assumption_var);
      _activeSoftPartition[soft_partitions[_current_partition][i]] = true;
    }
}


void UpWMSU3::createPartitions() {
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

  std::vector< std::pair <int,int> > vect;
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

// Returns true if there is a subset of set[] with sum equal to given sum
bool UpWMSU3::subsetSum(vec<uint64_t> &set, int64_t sum)
{
  int n = set.size();
  
  // Declaring the array dynamically to avoid stack frame limit
  bool ** subset = new bool*[sum+1];
  for (int i = 0; i <= sum; i++)
    subset[i] = new bool[n+1];

  // The value of subset[i][j] will be true if there is a subset of set[0..j-1]
  //  with sum equal to i
  
  // If sum is 0, then answer is true
  for (int i = 0; i <= n; i++)
  {
    //assert (i < n+1);
    //printf("i %d\n",i);
    subset[0][i] = true;
  }

  // If sum is not 0 and set is empty, then answer is false
  for (int i = 1; i <= sum; i++)
    subset[i][0] = false;

  // Fill the subset table in bottom up manner
  for (int i = 1; i <= sum; i++)
  {
    for (int j = 1; j <= n; j++)
    {
      subset[i][j] = subset[i][j - 1];
      if (i >= set[j - 1])
        subset[i][j] = subset[i][j] || subset[i - set[j - 1]][j - 1];
    }
  }

  bool is_subset = subset[sum][n];
  for (int i = 0; i <= sum; i++)
    delete [] subset[i];
  delete [] subset;

  return is_subset;
}

// Print WSMU3 configuration.
void UpWMSU3::print_UpWMSU3_configuration()
{
  if (is_bmo)
  {
    if (bmo_strategy)
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "On");
    else
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "Off");

    printf("c |  BMO search: %22s                      "
           "                                             |\n",
           "Yes");
    print_Card_configuration(encoder.getCardEncoding());
  }
  else
  {
    if (bmo_strategy)
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "On");
    else
      printf("c |  BMO strategy: %20s                      "
             "                                             |\n",
             "Off");

    printf("c |  BMO search: %22s                      "
           "                                             |\n",
           "No");
    print_PB_configuration(encoder.getPBEncoding());
  }
}
