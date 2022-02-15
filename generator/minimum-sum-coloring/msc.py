#!/usr/bin/python
#Title			: msc.py
#Usage			: python msc.py -i instance.g [-pwcnf 
#Author			: pmorvalho
#Date			: February 11, 2022
#Description           	: MaxSAT encoding for the Minimum Sum Coloring Problem 
#Notes			: It is a weighted problem. For the SAT 2022 Tool Paper evaluaiton.
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

#!/usr/bin/python    

import sys
from sys import argv
import argparse

# input f
# g num_nodes num_edges
# v1 v2
# ...

parts = 1
clauses = 1
formula = ""
num_vars = 1
n_nodes = None
n_edges = None
n_colors = None
nodes = None
edges = []
soft = None
hard = None


def parse_input(f):
    global n_nodes, n_edges, n_colors, nodes, edges, parts, hard, args
    f = open(f, "r")
    lines = f.readlines()
    _, n_nodes, n_edges = lines[0].split()
    n_nodes, n_edges = int(n_nodes), int(n_edges)
    n_colors = n_nodes
    nodes = [[[]] for _ in range(n_colors+1)]
    edges = []

    # reading edges
    for l in lines[1:]: 
        v1, v2 = l.split()
        edges.append((int(v1),int(v2)))
    # print(edges)
            
    hard = n_colors*2
        
def encoding():
    global formula
    global clauses
    global n_nodes, n_edges, n_colors, nodes, edges, parts, hard, num_vars

    #initializing variables
    v = 1
    for _ in range(1,n_colors+1):
        for v in range(1,n_nodes+1):
            nodes[v].append(num_vars)
            num_vars += 1
    # print(nodes)
    
    # Each vertex should assigned a color
    for n in range(1,n_nodes+1):
        c = ""
        for v in range(1,n_colors+1):
    	    c = c+str(nodes[n][v])+" "
        formula = formula +str(parts)+" "+str(hard)+" "+c+" 0\n"
        clauses = clauses + 1

    # Each vertex should assigned at most one color
    for v in range(1,n_nodes+1):
        for c1 in range(1,n_colors+1):
            for c2 in range(v+1,n_colors+1):
                formula = formula +str(parts)+" "+str(hard)+" "+ str(-nodes[v][c1])+" "+ str(-nodes[v][c2])+" 0\n"
                clauses = clauses + 1        


    # Each two adjacent vertices a and b cannot be assigned the same color
    for c in range(1,n_colors+1):
        for (v1, v2) in edges:
            formula = formula +str(parts)+" "+str(hard)+" "+ str(-nodes[v1][c])+" "+ str(-nodes[v2][c])+" 0\n"            
            clauses = clauses + 1 

    # we use c in color-based partitions and n in vertex-based        
    parts = n_colors+1 if args.pwcnf_colors else n_nodes + 1
                    
    # Soft clauses
    for n in range(1,n_nodes+1):
        for c in range(1,n_colors+1):
            # we use c in color-based partitions and n in vertex-based
            cost = c if args.pwcnf_colors else n
            p = c + 1 if args.pwcnf_colors else n + 1
            formula = formula + str(p)+" "+str(cost) + " " + str(-nodes[n][c]) + " 0\n"
            clauses = clauses + 1

                                
def parser():
    parser = argparse.ArgumentParser(prog='msc.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-f', '--input_file', nargs='?',help='input file')
    parser.add_argument('-pc', '--pwcnf_colors', action='store_true', default=False, help='Output format: PWCNF (colors)')
    parser.add_argument('-pn', '--pwcnf_nodes', action='store_true', default=False, help='Output format: PWCNF (nodes) (DEFAULT)')
    parser.add_argument('-w', '--wcnf', action='store_true', default=False, help='Output format: WCNF')
    args = parser.parse_args(argv[1:])
    return args

                                
def main():
    parse_input(args.input_file)
    encoding()
    print("p pwcnf {v} {c} {h} {p}".format(v=num_vars-1,c=clauses,h=hard,p=parts))
    print(formula)

if __name__ == "__main__":
    args = parser()
    main()


