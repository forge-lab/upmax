#!/usr/bin/python
#Title			: vmc.py
#Usage			: python vmc.py -h
#Author			: pmorvalho
#Date			: February 11, 2022
#Description	        : Encodes the problem of Virtual Machines Consolidation (VMC) into MaxSAT 
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================


import sys
from sys import argv
import argparse
from pysat.pb import *

# input f
# g num_nodes num_edges
# v1 v2
# ...

parts = 1
clauses = 0
formula = ""
num_vars = 0
n_servers = None
n_vms = None
vms = None
servers = None
soft = None
hard = None
vms_host = []
vms_memory = []
vms_cpu = []
servers_memory = []
servers_cpu = []

def parse_input(f):
    global n_servers, n_vms, n_colors, vms, servers, parts, hard, args, k, vms_memory, vms_cpu, vms_host, servers_memory, servers_cpu
    f = open(f, "r")
    lines = f.readlines()
    while "Total" in lines[0]:
        lines.pop(0)
    # getting servers' info    
    n_servers = int(lines[0])
    lines.pop(0)
    servers_memory = [None for _ in range(n_servers+1)]
    servers_cpu = [None for _ in range(n_servers+1)]
    for s in range(1,n_servers+1):
        m, cpu = lines[0].split()
        servers_memory[s]=int(m)
        servers_cpu[s]=int(cpu)
        lines.pop(0)
    
    # getting VMs' info
    n_vms = int(lines[0])
    lines.pop(0)
    vms_host = [None for _ in range(n_vms+1)]
    vms_memory = [None for _ in range(n_vms+1)]
    vms_cpu = [None for _ in range(n_vms+1)]
    for v in range(1,n_vms+1): 
        m, cpu, s = lines[0].split()
        vms_host[v] = int(s)
        vms_memory[v] = int(m)
        vms_cpu[v] = int(cpu)
        lines.pop(0)
            
    hard = (max(servers_memory[1:])*2)+1
    k = int(lines[0])
    
def add_lessEqualK_constraint(lits, k, weights=None):
    global formula, clauses, num_vars
    cnf = PBEnc.leq(lits=lits, top_id=num_vars, weights=weights, bound=k)
    num_vars = cnf.nv
    for cl in cnf.clauses:
        formula = formula+str(parts)+" "+str(hard)+" "+" ".join([str(c) for c in cl])+" 0\n"
        clauses += 1
        
def encoding():
    global formula, clauses
    global n_servers, n_vms, n_colors, vms, servers, parts, hard, args, k, num_vars, vms_host, vms_memory, vms_cpu

    #initializing variables for VMs (X_i j)
    v = 1
    vms = [[[]] for _ in range(n_vms+1)]
    for _ in range(1,n_servers+1):
        for v in range(1,n_vms+1):
            num_vars += 1
            vms[v].append(num_vars)
            
    if args.debbug:
        for v in range(1, n_vms+1):
            print("VM {v}:".format(v=v), vms[v][1:], file=sys.stderr)

    servers = [None for _ in range(n_servers+1)]
    for s in range(1,n_servers+1):
        num_vars += 1
        servers[s] = num_vars
        
    if args.debbug:
        for s in range(1, n_servers+1):
            print("Server {s}: ".format(s=s), servers[s], file=sys.stderr)
        
    # Each VM should assigned to a physical server
    for v in range(1,n_vms+1):
        c = ""
        for s in range(1,n_servers+1):
    	    c = c+" "+str(vms[v][s])
        formula = formula +str(parts)+" "+str(hard)+c+" 0\n"
        clauses += 1

    # Each VM should assigned at most one server
    for v in range(1,n_vms+1):
        c = ""
        for s in range(1,n_servers+1):
            for s2 in range(s+1,n_servers+1):
                formula = formula +str(parts)+" "+str(hard)+" "+ str(-vms[v][s])+" "+ str(-vms[v][s2])+" 0\n"
                clauses += 1        

    # If at least one VM is assigned to a server, then that server is active
    for s in range(1,n_servers+1):
        for v in range(1,n_vms+1):
            formula = formula +str(parts)+" "+str(hard)+" "+ str(-vms[v][s])+" "+ str(servers[s])+" 0\n"
            clauses += 1        

    # If no VM is assigned to a server, then that server is inactive
    for s in range(1,n_servers+1):
        c = ""
        for v in range(1,n_vms+1):
    	    c = c+" "+str(vms[v][s])
        formula = formula +str(parts)+" "+str(hard)+c+" "+str(-servers[s])+" 0\n"
        clauses += 1
            
    # The memory and CPU resources used by the VMs at a given server do not exceed the memory and CPU resources of the server;
    for s in range(1,n_servers+1):
        lits = []
        for v in range(1,n_vms+1):
            lits.append(vms[v][s])  

        # Memory constraint
        if args.debbug:
            print("VMs literals:", lits, file=sys.stderr)
            print("Server {s}\nMemory {m}  VMs:{w}".format(s=s, m=servers_memory[s], w=vms_memory[1:]), file=sys.stderr)
        add_lessEqualK_constraint(lits, servers_memory[s], weights=vms_memory[1:])

        # CPU constraint
        if args.debbug:
            print("CPU {c}  VMs:{w}".format(c=servers_cpu[s], w=vms_cpu[1:]), file=sys.stderr)
        add_lessEqualK_constraint(lits, servers_cpu[s], weights=vms_cpu[1:])
        
    # A maximum of K servers (K < n) are to be active
    add_lessEqualK_constraint(servers[1:], k)
    
    # Soft clauses
    parts = n_vms+1 if args.pwcnf_vms else n_servers + 1                
    for v in range(1,n_vms+1):
        for s in range(1,n_servers+1):
            if vms_host[v] != s:
                cost = vms_memory[v] if not args.unweighted else 1  # unweighted version (cost=1)
                p = v+1 if args.pwcnf_vms else s+1
                formula = formula + str(p)+" "+str(cost)+" "+ str(-vms[v][s]) + " 0\n"
                clauses += 1         
                    
                                
def parser():
    parser = argparse.ArgumentParser(prog='msc.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-f', '--input_file', nargs='?',help='input file')
    parser.add_argument('-d', '--debbug', action='store_true', default=False, help='Debug info')
    parser.add_argument('-ps', '--pwcnf_servers', action='store_true', default=False, help='Output format: PWCNF (colors) (DEFAULT)')
    parser.add_argument('-pv', '--pwcnf_vms', action='store_true', default=False, help='Output format: PWCNF (nodes)')
    parser.add_argument('-uw', '--unweighted', action='store_true', default=False, help='Unweighted Version of the VMC problem. If this flag is not used then the weighted version is used.')
    args = parser.parse_args(argv[1:])
    return args

                                
def main():
    parse_input(args.input_file)
    encoding()
    print("p pwcnf {v} {c} {h} {p}".format(v=num_vars,c=clauses,h=hard,p=parts))
    print(formula, end="")

if __name__ == "__main__":
    args = parser()
    main()
