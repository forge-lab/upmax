#!/usr/bin/python
#Title			: generator.py
#Usage			: python generator.py -n num_nodes -p prob_edges
#Author			: pmorvalho
#Date			: February 11, 2022
#Description	        : Creates a Erdos-Renyi graph with n nodes and probability prob of edges and a number of colors
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

import argparse
from sys import argv
import random


def generate_erdos_renyi_graph(n, p):
    edges = []
    connections = [0 for _ in range(n+1)]
    for i in range(1,n+1):
        for j in range(i+1, n+1):
            if random.random() < p:
                edges.append((i,j))
                connections[i] += 1
                connections[j] += 1
    # the number of colors is the maximum number of connections for all nodes in the graph
    print("g {n} {ne} {nc} ".format(n=n, ne=len(edges), nc=max(connections)+1))
    for e in edges:
        print(e[0], e[1])

def parser():
    parser = argparse.ArgumentParser(prog='generator.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-n', '--num_nodes', default=10, type=int, help='number of nodes the graph should have.')
    parser.add_argument('-p', '--probability_edges', default=0.7, type=float, help='probability of edges expected in the graph.')
    args = parser.parse_args(argv[1:])
    return args

if __name__ == '__main__':
    args = parser()
    generate_erdos_renyi_graph(args.num_nodes, args.probability_edges)
