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
    
    servers_mem = [0 for _ in range(ns+1)]
    servers_cpu = [0 for _ in range(ns+1)]
    total_mem = 0
    servers_info = "servers\n"
    for i in range(1,ns+1):
        # random choice of each server's memory
        m=random.sample([4, 8, 16, 32, 64, 128], k=1)[0]
        # c=100           
        servers_mem[i] = m
        total_mem += m
        # initially each server is empty so it has 100 of free CPU
        servers_cpu[i] = 100
        servers_info = servers_info + "{s} {m}\n".format(s=i, m=m)# , cpu=c)

    # The total memory we want to used 
    mem_2_use = random.sample([0.25,0.3,0.35,0.4], k=1)[0]
    # k is the ideal number of servers we want to de active after minimization
    k = int(mem_2_use*2*ns)
    mem_2_use = int(total_mem*mem_2_use)
    vms_info = "VMs\n"
    for i in range(1,nvms+1):
        not_assigned = True
        while not_assigned:
            mem_slots = [1, 2, 4, 8, 16]
            mems = [m for m in mem_slots if mem_2_use >= m]
            if mems == []:
                # No more memory available.
                break
            
            m=random.sample(mems, k=1)[0]
            s=random.randint(1,ns)
            c=random.randint(1,32)
            available_mem = servers_mem[s] - m
            available_cpu = servers_cpu[s] - c            
            if available_mem >= 0 and available_cpu >= 0:
                servers_mem[s] = available_mem
                servers_cpu[s] = available_cpu
                mem_2_use -= m
            else:
                continue
            vms_info = vms_info + "{i} {m} {cpu} {s}\n".format(i=i, m=m, cpu=c, s=s)
            not_assigned=False      
            break

        if not not_assigned:
            continue # is VM i has already been assigned successfully to some server
        
        # in case no more memory (to use) is available which is likely since we chosse a percentage of at most 40% of the total memory
        nvms = i-1
        break

    # header: "vmc" num_servers num_VMs K (K is the highest number (at most) of servers we want to remain active)
    print("vmc {n} {vms} {k}".format(n=ns, vms=nvms, k=k))
    print(servers_info[:-1])
    print(vms_info[:-1])
    
def parser():
    parser = argparse.ArgumentParser(prog='generator.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-s', '--num_servers', default=10, type=int, help='Number of severs of the problem')
    parser.add_argument('-v', '--num_VMs', default=20, type=int, help='Number of VMs the problem should have.')
    args = parser.parse_args(argv[1:])
    return args

if __name__ == '__main__':
    args = parser()
    generate_VMC_problem(args.num_servers, args.num_VMs)
