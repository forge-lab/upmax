/*!
 * \author Ruben Martins - ruben@sat.inesc-id.pt
 *
 * @section LICENSE
 *
 * MiniSat,  Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 *           Copyright (c) 2007-2010, Niklas Sorensson
 * Open-WBO, Copyright (c) 2013-2015, Ruben Martins, Vasco Manquinho, Ines Lynce
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

#ifndef ParserPartition_h
#define ParserPartition_h

#include <stdio.h>
#include <set>

#include "MaxSATFormula.h"
#include "ParserMaxSAT.h"
#include "core/SolverTypes.h"
#include "utils/ParseUtils.h"

#ifdef HAS_EXTRA_STREAMBUFFER
#include "utils/StreamBuffer.h"
#endif

using NSPACE::mkLit;
using NSPACE::StreamBuffer;

namespace openwbo {

//=================================================================================================
// DIMACS Parser:

template <class B, class MaxSATFormula>
static void parsePwcnf(B &in, MaxSATFormula *maxsat_formula) {
  vec<Lit> lits;
  std::set<int> ids;
  uint64_t hard_weight = UINT64_MAX;
  for (;;) {
    skipWhitespace(in);
    if (*in == EOF)
      break;
    else if (*in == 'p') {
      if (eagerMatch(in, "p pwcnf")) {
        maxsat_formula->setProblemType(_WEIGHTED_);
        parseInt(in); // Variables
        parseInt(in); // Clauses
        if (*in != '\r' && *in != '\n') {
          hard_weight = parseWeight(in);
          maxsat_formula->setHardWeight(hard_weight);
        }
        skipWhitespace(in);
        parseInt(in); // Partitions
      } else
        printf("c PARSE ERROR! Unexpected char: %c\n", *in),
            printf("s UNKNOWN\n"), exit(_ERROR_);
    } else if (*in == 'c' || *in == 'p')
      skipLine(in);
    else {
      int partition = parseInt(in);
      skipWhitespace(in);
      uint64_t weight = readClause(in, maxsat_formula, lits);
      if (weight < hard_weight ||
          maxsat_formula->getProblemType() == _UNWEIGHTED_) {
        assert(weight > 0);
        // Updates the maximum weight of soft clauses.
        maxsat_formula->setMaximumWeight(weight);
        // Updates the sum of the weights of soft clauses.
        maxsat_formula->updateSumWeights(weight);
        maxsat_formula->addSoftClause(weight, lits);
        maxsat_formula->setSoftClausePartition(partition);
        ids.insert(partition);
      } else {
        maxsat_formula->addHardClause(lits);
        maxsat_formula->setHardClausePartition(partition);
        ids.insert(partition);
      }
    }
  }
  std::set<int>::iterator itr;

  int max = 0;
  for (itr = ids.begin(); 
       itr != ids.end(); itr++) 
  {
    //cout << *itr << " ";
    if (*itr > max) max = *itr;
  }
  //maxsat_formula->setPartitions(ids.size());
  maxsat_formula->setPartitions(max);
}

// Inserts problem into solver.
//
template <class MaxSATFormula>
static void parsePwcnfFormula(gzFile input_stream,
                               MaxSATFormula *maxsat_formula) {
  StreamBuffer in(input_stream);
  parsePwcnf(in, maxsat_formula);
  if (maxsat_formula->getMaximumWeight() == 1)
    maxsat_formula->setProblemType(_UNWEIGHTED_);
  else
    maxsat_formula->setProblemType(_WEIGHTED_);
  // maxsat_formula->setInitialVars(maxsat_formula->nVars());
}

//=================================================================================================
} // namespace openwbo

#endif
