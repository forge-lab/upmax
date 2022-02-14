#!/usr/bin/python
#Title			: generator.py
#Usage			: python generator.py -h
#Author			: pmorvalho
#Date			: February 11, 2022
#Description            : Creates instances for the Virtual Machine Consolidation problem 
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

import argparse
from sys import argv
import random


def generate_VMC_problem(ns, nvms):
    print("vmc {n} {vms}".format(n=ns, vms=nvms))
    print("severs")
    servers_mem = [0 for _ in range(ns+1)]
    servers_cpu = [0 for _ in range(ns+1)] 
    for i in range(1,ns+1):
        m=random.sample([512, 1024, 2048, 4096], k=1)[0]
        c=100
        servers_mem[i] = m
        servers_cpu[i] = c
        print("{s} {m} {cpu}".format(s=i, m=m, cpu=c))

    print("VMs")    
    for i in range(1,nvms+1):
        not_assigned = True
        while not_assigned:
            m=random.sample([8, 16, 32, 64, 128, 256, 512, 1024], k=1)[0]
            s=random.randint(1,ns)
            c=random.sample([0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8], k=1)[0]
            available_mem = servers_mem[s] - m
            available_cpu = servers_cpu[s] - c            
            if available_mem >= 0 and available_cpu >= 0:
                servers_mem[s] = available_mem
                servers_cpu[s] = available_cpu
            else:
                continue
            print("{i} {m} {cpu} {s}".format(i=i, m=m, cpu=c, s=s))
            not_assigned=False      
            break
                
def parser():
    parser = argparse.ArgumentParser(prog='generator.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-s', '--num_servers', default=10, type=int, help='Number of severs of the problem')
    parser.add_argument('-v', '--num_VMs', default=20, type=int, help='Number of VMs the problem should have.')
    args = parser.parse_args(argv[1:])
    return args

if __name__ == '__main__':
    args = parser()
    generate_VMC_problem(args.num_servers, args.num_VMs)
