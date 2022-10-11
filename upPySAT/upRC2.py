#!/usr/bin/python
#Title			: upRC2.py
#Usage			: python upRC2.py -h
#Author			: pmorvalho
#Date			: October 03, 2022
#Description		: UpRC2, RC2 Solver for PWCNFs i.e. takes advantage of user-defined partitions of the WCNF formula
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

import argparse
from sys import argv
from pwcnf import PWCNF
from pysat.formula import WCNF
from pysat.examples.rc2 import RC2

class UpRC2(object):
    """
       RC2 Solver Solver for PWCNFs based on user partitioning of the soft clauses.
       Calls RC2 #n_partitions times, adding each set of user-defined partitions incrementally.
    """
    def __init__(self, pwcnf, no_up=False):
        """
            Constructor.
        """
        self.pwcnf = pwcnf
        self.solver = RC2(WCNF())
        self.no_up = no_up
        
    def compute(self):
        """
           Computes either using user-based partitions or not
        """
        if not self.no_up:
            return upRC2.compute_with_partitions()
        else:
            return upRC2.compute_without_partitions()
        
    def compute_with_partitions(self):
        """
            Calls RC2 #n_partitions times, adding each set of user-defined partitions incrementally.
        """ 
        for h in self.pwcnf.get_hard():
            self.solver.add_clause(h)
        m = None
        for j in range(len(self.pwcnf.get_partitions())):
            p_clauses, wghts = self.pwcnf.get_partition(j), self.pwcnf.get_partition_weights(j)
            for i in range(len(p_clauses)):
                c, w = p_clauses[i], wghts[i]
                self.solver.add_clause(c, weight=w)
            m = self.solver.compute()
        return m, self.solver.cost

    def compute_without_partitions(self):
        """
            Calls RC2 ignoring the user-defined partitions i.e. adds all the hard and soft clauses at once.
        """ 
        for h in self.pwcnf.get_hard():
            self.solver.add_clause(h)
        for i in range(len(self.pwcnf.get_soft())):
            c, w = self.pwcnf.get_soft_clause(i), self.pwcnf.get_soft_weight(i)
            self.solver.add_clause(c, weight=w)
        return self.solver.compute(), self.solver.cost

    
    
def parser():
    parser = argparse.ArgumentParser(prog='upRC2.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-f', '--pwcnf', help='PWCNF formula.')
    parser.add_argument('-nup', '--no_up', action='store_true', default=False, help='Calls the MaxSAT solver (RC2) using the PWCNF formula without considering the partitions present in the formula.')
    parser.add_argument('--wcnf', help='WCNF formula.')
    parser.add_argument('-v', '--verbose', action='store_true', default=False, help='Prints debugging information.')    
    args = parser.parse_args(argv[1:])
    return args

if __name__ == '__main__':
    args = parser()
    if args.pwcnf:
        pwcnf = PWCNF(from_file=args.pwcnf)
    elif args.wcnf:
        pwcnf = PWCNF(from_file=args.wcnf, wcnf=True)
        args.no_up = True
    upRC2 = UpRC2(pwcnf, no_up=args.no_up)
    m, c =upRC2.compute()
    if m:
        print('s OPTIMUM FOUND')
        print('o', str(c))
        print()
        print(m)
    else:
        print("UNSAT")
