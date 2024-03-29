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

#ifndef MaxTypes_h
#define MaxTypes_h

#include <sstream>

namespace upmax {

#define _MAX_CLAUSES_ 3000000

/** This class catches the exception that is used across the solver to indicate errors */
class MaxSATException
{
  std::stringstream s;
public:
  MaxSATException(const char* file, const int line, const char* msg)
  {
    s << file << ":" << line << ":" << msg;
  }
  const char* getMsg() const {return s.str().c_str();}
};

enum { _FORMAT_MAXSAT_ = 0, _FORMAT_PB_ = 1, _FORMAT_PWCNF_ = 3 };
enum { _VERBOSITY_MINIMAL_ = 0, _VERBOSITY_SOME_ };
enum { _UNWEIGHTED_ = 0, _WEIGHTED_ };
enum { _WEIGHT_NONE_ = 0, _WEIGHT_NORMAL_, _WEIGHT_DIVERSIFY_ };
enum {
  _ALGORITHM_WBO_ = 0,
  _ALGORITHM_MSU3_,
  _ALGORITHM_OLL_,
};
enum StatusCode {
  _SATISFIABLE_ = 10,
  _UNSATISFIABLE_ = 20,
  _OPTIMUM_ = 30,
  _UNKNOWN_ = 40,
  _ERROR_ = 50
};
enum {
  _INCREMENTAL_NONE_ = 0,
  _INCREMENTAL_BLOCKING_,
  _INCREMENTAL_WEAKENING_,
  _INCREMENTAL_ITERATIVE_
};
enum { _SIZE_, _CORES_, _SATURATION_ONLY_ };
enum { _CARD_CNETWORKS_ = 0, _CARD_TOTALIZER_, _CARD_MTOTALIZER_ };
enum { _AMO_LADDER_ = 0 };
enum { _PB_SWC_ = 0, _PB_GTE_, _PB_ADDER_ };
enum { _PART_SEQUENTIAL_ = 0, _PART_SEQUENTIAL_SORTED_, _PART_BINARY_ };

}
#endif
