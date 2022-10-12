#!/usr/bin/python
#Title			: upHitman.py
#Usage			: python upHitman.py -h
#Author			: pmorvalho
#Date			: October 03, 2022
#Description		: UpHitman: A MaxHS-like Solver for PWCNFs based on user partitioning.
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

import argparse
from sys import argv
from pwcnf import PWCNF
from pysat.examples.hitman import Hitman
from pysat.solvers import Solver

class UpHitman(object):
    """
       A MaxHS-like Solver for PWCNFs based on user partitioning of the soft clauses.
       Calls Hitman #n_partitions times, adding each set of user-defined partitions incrementally.
       Hitman (PySAT) is a cardinality-/subset-minimal hitting set enumerator.
    """
    def __init__(self, pwcnf, no_up=False):
        """
            Constructor.
        """
        self.pwcnf = pwcnf
        self.hitman = Hitman(htype="maxsat") # the default is to use MCSls, but we are using RC2
        self.solver = Solver(name='g3', bootstrap_with=self.pwcnf.get_hard())
        self.nv = self.pwcnf.get_num_variables()
        self.no_up = no_up

    def compute(self):
        """
           Computes either using user-based partitions or not
        """        
        if not self.no_up:
            return upHitman.compute_with_partitions()
        else:
            return upHitman.compute_without_partitions()
  
    def get_cost(self, m, rel_vars, ws):
        """
            Computes the total cost of the unsatisfied soft clauses based on the relaxation variables.
        """
        t_cost = 0
        for v in rel_vars:
            if -v in m:
                t_cost += ws[v]
                m.remove(-v)
                continue
            m.remove(v)               
        return m, t_cost
        
    def compute_with_partitions(self):
        """
            Runs a MaxHS-like algorithm for MaxSAT #n_partitions times, adding each set of user-defined partitions (relaxed) incrementally.
        """
        sels = []
        ws = dict()
        hs = None
        for j in range(len(self.pwcnf.get_partitions())):
            p_clauses, wghts = self.pwcnf.get_partition(j), self.pwcnf.get_partition_weights(j)
            for i in range(len(p_clauses)):
                c, w = p_clauses[i], wghts[i]
                self.nv += 1
                self.solver.add_clause(c + [-1*self.nv])
                sels += [self.nv]
                ws[self.nv] = w
                
            while True:
                hs = self.hitman.get()  # hitting the set of unsatisfiable cores
                ts = set(sels).difference(set(hs))  # soft clauses to try
                    
                if self.solver.solve(assumptions=list(ts)):
                    break
                else:
                    core = self.solver.get_core()
                    self.hitman.hit(core, weights=ws)                
            m = self.solver.get_model()
            
        return self.get_cost(m, sels, ws)

    def compute_without_partitions(self):
        """
            MaxHS-like algorithm for MaxSAT using all the soft clauses at once.
        """
        sels = []
        ws = dict()
        hs = None
        for i in range(len(self.pwcnf.get_soft())):
            c, w = self.pwcnf.get_soft_clause(i), self.pwcnf.get_soft_weight(i)
            self.nv += 1
            self.solver.add_clause(c + [-1*self.nv])
            sels += [self.nv]
            ws[self.nv] = w
                
        while True:
            hs = self.hitman.get()  # hitting the set of unsatisfiable cores
            ts = set(sels).difference(set(hs))  # soft clauses to try
                    
            if self.solver.solve(assumptions=list(ts)):
                break
            else:
                core = self.solver.get_core()
                self.hitman.hit(core, weights=ws)                

        m = self.solver.get_model()            
        return self.get_cost(m, sels, ws)    
    
def parser():
    parser = argparse.ArgumentParser(prog='upHitman.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-f', '--pwcnf', help='PWCNF formula.')
    parser.add_argument('-nup', '--no_up', action='store_true', default=False, help='Calls the MaxSAT solver (Hitman) without considering the partitions present in the formula.')
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
    upHitman = UpHitman(pwcnf, no_up=args.no_up)
    if not args.no_up:
        m, c =upHitman.compute_with_partitions()
    else:
        m, c = upHitman.compute_without_partitions()
    if m:
        print('s OPTIMUM FOUND')
        print('o', str(c))
        print()
        print(m)
    else:
        print("UNSAT")
