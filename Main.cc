/*!
 * \author Ruben Martins - rubenm@andrew.cmu.edu
 *
 * @section LICENSE
 *
 * MiniSat,  Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 *           Copyright (c) 2007-2010, Niklas Sorensson
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

#include "utils/Options.h"
#include "utils/ParseUtils.h"
#include "utils/System.h"
#include <errno.h>
#include <signal.h>
#include <zlib.h>

#include <fstream>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include "core/Solver.h"

#include "MaxSAT.h"
#include "MaxTypes.h"
#include "ParserMaxSAT.h"
#include "ParserPB.h"
#include "ParserPartitions.h"

#include "MaxSAT_Partition.h"

// Algorithms
#include "algorithms/Alg_MSU3.h"
#include "algorithms/Alg_OLL.h"
#include "algorithms/Alg_WBO.h"
#include "algorithms/Alg_UpOLL.h"
#include "algorithms/Alg_UpMSU3.h"
#include "algorithms/Alg_UpWBO.h"
#include "algorithms/Alg_WMSU3.h"
#include "algorithms/Alg_UpWMSU3.h"

#define VER1_(x) #x
#define VER_(x) VER1_(x)
#define SATVER VER_(SOLVERNAME)
#define VER VER_(VERSION)

using NSPACE::cpuTime;
using NSPACE::OutOfMemoryException;
using NSPACE::IntOption;
using NSPACE::BoolOption;
using NSPACE::StringOption;
using NSPACE::IntRange;
using NSPACE::parseOptions;
using namespace upmax;

//=================================================================================================

static MaxSAT *mxsolver;

static void SIGINT_exit(int signum) {
  mxsolver->printAnswer(_UNKNOWN_);
  exit(_UNKNOWN_);
}

//=================================================================================================
#if !defined(_MSC_VER) && !defined(__MINGW32__)
void limitMemory(uint64_t max_mem_mb)
{
// FIXME: OpenBSD does not support RLIMIT_AS. Not sure how well RLIMIT_DATA works instead.
#if defined(__OpenBSD__)
#define RLIMIT_AS RLIMIT_DATA
#endif

    // Set limit on virtual memory:
    if (max_mem_mb != 0){
        rlim_t new_mem_lim = (rlim_t)max_mem_mb * 1024*1024;
        rlimit rl;
        getrlimit(RLIMIT_AS, &rl);
        if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max){
            rl.rlim_cur = new_mem_lim;
            if (setrlimit(RLIMIT_AS, &rl) == -1)
                printf("c WARNING! Could not set resource limit: Virtual memory.\n");
        }
    }

#if defined(__OpenBSD__)
#undef RLIMIT_AS
#endif
}
#else
void limitMemory(uint64_t /*max_mem_mb*/)
{
    printf("c WARNING! Memory limit not supported on this architecture.\n");
}
#endif


#if !defined(_MSC_VER) && !defined(__MINGW32__)
void limitTime(uint32_t max_cpu_time)
{
    if (max_cpu_time != 0){
        rlimit rl;
        getrlimit(RLIMIT_CPU, &rl);
        if (rl.rlim_max == RLIM_INFINITY || (rlim_t)max_cpu_time < rl.rlim_max){
            rl.rlim_cur = max_cpu_time;
            if (setrlimit(RLIMIT_CPU, &rl) == -1)
                printf("c WARNING! Could not set resource limit: CPU-time.\n");
        }
    }
}
#else
void limitTime(uint32_t /*max_cpu_time*/)
{
    printf("c WARNING! CPU-time limit not supported on this architecture.\n");
}
#endif

//=================================================================================================
// Main:

int main(int argc, char **argv) {
  printf(
      "c\nc UpMax:\t User partitioning for MaxSAT -- based on %s (%s version)\n",
      SATVER, VER);
  printf("c Version:\t February 2022\n");
  printf("c Authors:\t Pedro Orvalho, Vasco Manquinho,  Ruben Martins\n");
  printf("c Contact:\t rubenm@andrew.cmu.edu\n");
  try {
    NSPACE::setUsageHelp("c USAGE: %s [options] <input-file>\n\n");

#if defined(__linux__)
    fpu_control_t oldcw, newcw;
    _FPU_GETCW(oldcw);
    newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
    _FPU_SETCW(newcw);
    printf(
        "c WARNING: for repeatability, setting FPU to use double precision\n");
#endif

    BoolOption allopts("UpMax", "all-opt", "Finds all optimal solutions.\n", false);
    BoolOption allvars("UpMax", "all-vars", "Uses all variables to find optimal solutions.\n", false);

    BoolOption printmodel("UpMax", "print-model", "Print model.\n", true);

    StringOption printsoft("UpMax", "print-unsat-soft", "Print unsatisfied soft claues in the optimal assignment.\n", NULL);

    IntOption verbosity("UpMax", "verbosity",
                        "Verbosity level (0=minimal, 1=more).\n", 0,
                        IntRange(0, 1));

    StringOption json ("UpMax", "json", "JSON search statistics output destination\n", NULL);

    IntOption pwcnf_limit("UpMax","limit","Conflict limit for each partition.\n",-1,IntRange(-1,INT32_MAX));
    IntOption pwcnf_mode("UpMax","merge","Merge heuristic (0=partition size,1=core size,2=saturation only).\n",0,IntRange(0,2));

    IntOption cpu_lim("UpMax", "cpu-lim",
                      "Limit on CPU time allowed in seconds.\n", 0,
                      IntRange(0, INT32_MAX));

    IntOption mem_lim("UpMax", "mem-lim",
                      "Limit on memory usage in megabytes.\n", 0,
                      IntRange(0, INT32_MAX));

    BoolOption upmax("UpMax", "upmax", "User partitioning.\n", true);

    StringOption upfile("UpMax", "upfile", "PWCNF file with automatic partition.\n", NULL);    

    BoolOption wcnf("UpMax", "wcnf", "Transform PWCNF in WCNF file.\n", false); 

    IntOption algorithm("UpMax", "algorithm",
                        "Search algorithm "
                        "(0=wbo,1=msu3,2=oll)\n",
                        1, IntRange(0, 2));

    
    IntOption graph_type("UpMax", "graph-type",
                         "Graph type (0=vig, 1=cvig, 2=res, 3=random).",
                         2, IntRange(0, 3));

    BoolOption bmo("UpMax", "bmo", "BMO search.\n", true);

    IntOption cardinality("Encodings", "cardinality",
                          "Cardinality encoding (0=cardinality networks, "
                          "1=totalizer, 2=modulo totalizer).\n",
                          1, IntRange(0, 2));

    IntOption amo("Encodings", "amo", "AMO encoding (0=Ladder).\n", 0,
                  IntRange(0, 0));

    IntOption pb("Encodings", "pb", "PB encoding (0=SWC,1=GTE,2=Adder).\n", 1,
                 IntRange(0, 2));

    IntOption formula("UpMax", "formula",
                      "Type of formula (0=WCNF, 1=OPB, 2=PWCNF).\n", 0, IntRange(0, 2));

    IntOption weight(
        "WBO", "weight-strategy",
        "Weight strategy (0=none, 1=weight-based, 2=diversity-based).\n", 2,
        IntRange(0, 2));

    BoolOption symmetry("WBO", "symmetry", "Symmetry breaking.\n", true);

    IntOption symmetry_lim(
        "WBO", "symmetry-limit",
        "Limit on the number of symmetry breaking clauses.\n", 500000,
        IntRange(0, INT32_MAX));
   
    parseOptions(argc, argv, true);

    // Try to set resource limits:
    if (cpu_lim != 0) limitTime(cpu_lim);
    if (mem_lim != 0) limitMemory(mem_lim);

    double initial_time = cpuTime();
    MaxSAT *S = NULL;

    signal(SIGXCPU, SIGINT_exit);
    signal(SIGTERM, SIGINT_exit);

    if (argc == 1) {
      printf("c Warning: no filename.\n");
    }

    gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
    if (in == NULL)
      printf("c ERROR! Could not open file: %s\n",
             argc == 1 ? "<stdin>" : argv[1]),
          printf("s UNKNOWN\n"), exit(_ERROR_);

    MaxSATFormula *maxsat_formula = new MaxSATFormula();

    if ((int)formula == _FORMAT_MAXSAT_) {
      parseMaxSATFormula(in, maxsat_formula);
      maxsat_formula->setFormat(_FORMAT_MAXSAT_);
    } else if ((int)formula == _FORMAT_PB_){
      ParserPB *parser_pb = new ParserPB();
      parser_pb->parsePBFormula(argv[1], maxsat_formula);
      maxsat_formula->setFormat(_FORMAT_PB_);
    } else {
      parsePwcnfFormula(in, maxsat_formula);
      maxsat_formula->setFormat(_FORMAT_PWCNF_);
    }
    
    gzclose(in);

    printf("c |                                                                "
           "                                       |\n");
    printf("c ========================================[ Problem Statistics "
           "]===========================================\n");
    printf("c |                                                                "
           "                                       |\n");

    if (maxsat_formula->getFormat() == _FORMAT_MAXSAT_)
      printf(
          "c |  Problem Format:  %17s                                         "
          "                          |\n",
          "MaxSAT");
    else
      printf(
          "c |  Problem Format:  %17s                                         "
          "                          |\n",
          "PB");

    if (maxsat_formula->getProblemType() == _UNWEIGHTED_)
      printf("c |  Problem Type:  %19s                                         "
             "                          |\n",
             "Unweighted");
    else
      printf("c |  Problem Type:  %19s                                         "
             "                          |\n",
             "Weighted");

    printf("c |  Number of variables:  %12d                                    "
           "                               |\n",
           maxsat_formula->nVars());
    printf("c |  Number of hard clauses:    %7d                                "
           "                                   |\n",
           maxsat_formula->nHard());
    printf("c |  Number of soft clauses:    %7d                                "
           "                                   |\n",
           maxsat_formula->nSoft());
    printf("c |  Number of cardinality:     %7d                                "
           "                                   |\n",
           maxsat_formula->nCard());
    printf("c |  Number of PB :             %7d                                "
           "                                   |\n",
           maxsat_formula->nPB());
    double parsed_time = cpuTime();

    printf("c |  Parse time:           %12.2f s                                "
           "                                 |\n",
           parsed_time - initial_time);
    printf("c |                                                                "
           "                                       |\n");

    
    if (upfile != NULL){
        MaxSAT_Partition * mp = new MaxSAT_Partition();
        mp->loadFormula(maxsat_formula);
        mp->init();
        if (wcnf){
            mp->split(PWCNF_MODE);
            mp->printPWCNFtoFile((const char *) upfile, wcnf);    
        } else {
            int graphType = graph_type;
            if (graphType == 3) {
                // random
                mp->split(RAND_MODE);
            } else mp->split(UNFOLDING_MODE, graphType);
            mp->printPWCNFtoFile((const char *) upfile);
        }
        exit(_UNKNOWN_);
    }


    switch ((int)algorithm) {
    case _ALGORITHM_WBO_:
      if (upmax){
        S = new UpWBO(verbosity, pwcnf_mode, pwcnf_limit);
      } else {
        S = new WBO(verbosity, weight, symmetry, symmetry_lim);
      }
      break;

    
    case _ALGORITHM_MSU3_:
        if (upmax){
            if(maxsat_formula->getProblemType() == _UNWEIGHTED_)
                S = new UpMSU3(verbosity, pwcnf_mode, pwcnf_limit);
            else
                S = new UpWMSU3(verbosity);
        } else {
            if(maxsat_formula->getProblemType() == _UNWEIGHTED_)
                S = new MSU3(verbosity);
            else
                S = new WMSU3(verbosity);
        }
      break;

    case _ALGORITHM_OLL_:
        if (upmax){
            S = new UpOLL(verbosity, pwcnf_mode, pwcnf_limit);
        } else {
            S = new OLL(verbosity, cardinality);
        }
      break;

    default:
      printf("c Error: Invalid MaxSAT algorithm.\n");
      printf("s UNKNOWN\n");
      exit(_ERROR_);
    }

    if (S->getMaxSATFormula() == NULL)
      S->loadFormula(maxsat_formula);
    S->setPrintModel(printmodel);
    S->setPrintSoft((const char *)printsoft);
    S->setJson((const char *) json);
    S->setInitialTime(initial_time);
    S->_all_opt_sols = allopts;
    S->_all_var_sols = allvars;

    mxsolver = S;
    mxsolver->setPrint(true);
    int ret = (int)mxsolver->search();
    delete S;
    return ret;
  } catch (OutOfMemoryException &) {
    sleep(1);
    printf("c Error: Out of memory.\n");
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  } catch(MaxSATException &e) {
    sleep(1);
    printf("c Error: MaxSAT Exception: %s\n", e.getMsg());
    printf("s UNKNOWN\n");
    exit(_ERROR_);
  }
}
