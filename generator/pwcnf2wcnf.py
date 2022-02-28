#!/usr/bin/python
#Title			: pwcnf2wcnf.py
#Usage			: python pwcnf2wcnf.py -h
#Author			: pmorvalho
#Date			: February 28, 2022
#Description     	: Translates from pwcnf format to wcnf 
#Notes			: 
#Python Version: 3.8.5
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

import argparse
from sys import argv
import gzip

def pwcnf2wcnf(pwcnf_file, wcnf_file):
    with gzip.open(pwcnf_file, 'rb') as f:
        pwcnf = str(f.read()).split('\\n')
        with open(wcnf_file, 'w') as wcnf:
            header = " ".join(pwcnf[0].split(" ")[:-1])[2:]+"\n"
            wcnf.write(header)
            for l in pwcnf[1:-1]:
                wcnf.write(" ".join(l.split(" ")[1:])+"\n")

def parser():
    parser = argparse.ArgumentParser(prog='pwcnf2wcnf.py', formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-i', '--pwcnf', nargs='?',help='input pwcnf file. (gzip file)')
    parser.add_argument('-o', '--wcnf', nargs='?',help='output wcnf file. (not a gzip file)')
    args = parser.parse_args(argv[1:])
    return args

if __name__ == '__main__':
    args = parser()
    pwcnf2wcnf(args.pwcnf, args.wcnf)
