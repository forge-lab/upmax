#!/usr/bin/python    

import sys
from pysat.card import *

if len(sys.argv) < 6:
	print("Usage: python seat.py <#tables> <min #persons per table> <max #persons per table> <generator file> <0=tables,1=tags>")
	exit()

# input: tables, min, max, csv, pwcnf (0, 1)
# pwcnf = 0 #tables
# pwcnf = 1 #tags
tbls = int(sys.argv[1])
min_tbls = int(sys.argv[2])
max_tbls =  int(sys.argv[3])
csv = sys.argv[4]
pwcnf = int(sys.argv[5])
parts = 1
# we will build an incomplete pwcnf (without num variables and clauses) to be faster
complete_pwcnf=False
if len(sys.argv) > 6:
	complete_pwcnf=True

persons = dict()
tags = set()

TablePerson = []
TableTag = []
v = 1
soft = 1
hard = 1
clauses = 1
variables = 1

formula = ""

def parse_csv():
	global TablePerson
	global TableTag
	global soft
	global hard
	global parts
	f = open(csv, "r")
	for x in f:
		y = x.split(',')
		for i in range(0,len(y)):
			if i==0:
				persons[int(y[i].strip())] = []
			else:
				persons[int(y[0].strip())].append(int(y[i].strip()))
				tags.add(int(y[i].strip()))

	TablePerson = [[0 for x in range(tbls)] for y in range(len(persons))]
	# print(len(persons))
	# print(TablePerson)
	TableTag = [[0 for x in range(tbls)] for y in range(max(tags)+1)]
	# print(TableTag)
	soft = len(tags)*tbls
	hard = soft+1

def add_atLeastK_constraint(lits, k):
        global formula, clauses, v, complete_pwcnf
        cnf = CardEnc.atleast(lits, bound=k, top_id=v)
        v = cnf.nv
        for cl in cnf.clauses:
                clauses += 1
                if not complete_pwcnf:
                        print(str(parts)+" "+str(hard)+" "+" ".join([str(c) for c in cl])+" 0")
                else:
                        formula = formula+str(parts)+" "+str(hard)+" "+" ".join([str(c) for c in cl])+" 0\n"

def add_atMostK_constraint(lits, k):
        global formula, clauses, v, complete_pwcnf
        cnf = CardEnc.atmost(lits, bound=k, top_id=v)
        v = cnf.nv
        for cl in cnf.clauses:
                clauses += 1
                if not complete_pwcnf:
                        print(str(parts)+" "+str(hard)+" "+" ".join([str(c) for c in cl])+" 0")
                else:
                        formula = formula+str(parts)+" "+str(hard)+" "+" ".join([str(c) for c in cl])+" 0\n"

def encoding():
	# create variables
	global v
	global formula
	global clauses, complete_pwcnf
	
	for x in range(0,tbls):
		for y in range(0,len(persons)):
			TablePerson[y][x] = v
			v = v + 1
	
	for x in range(0,tbls):
		for y in range(0,max(tags)+1):
			TableTag[y][x] = v
			v = v + 1

	# Header
	if not complete_pwcnf:
		print("p pwcnf XX "+ str(hard) + " {p}".format(p=tbls+1 if pwcnf == 0 else max(tags)+2)) 
	# If a person is seated in a table then that tag is true
	for p in persons.keys():
		for tag in persons[p]:
			for table in range(0,tbls):
				if complete_pwcnf:
					formula = formula + str(parts)+" "+str(hard) + " " + str(-TablePerson[p][table]) + " " + str(TableTag[tag][table]) + " 0\n"
				else:
					print(str(parts)+" "+str(hard) + " " + str(-TablePerson[p][table]) + " " + str(TableTag[tag][table]) + " 0")
				clauses += 1

	# Each table has at most k persons
	for table in range(0,tbls):
		lits_pos = []
		lits_neg = []
		for p in range(len(persons)):
			lits_pos.append(TablePerson[p][table])
			lits_neg.append(-TablePerson[p][table])

		add_atMostK_constraint(lits_pos, max_tbls)
		# sinz(lits_pos, max_tbls)
		add_atLeastK_constraint(lits_pos, min_tbls)
		# sinz(lits_neg, len(lits_neg)-min_tbls)
		
	# Each person is seated in at least one table
	for p in persons.keys():
		#for tag in persons[p]:
		lits = []
		for table in range(0,tbls):
			lits.append(TablePerson[p][table])
		# Each person is seated in at most one table
		# sinz(lits, 1)
		add_atMostK_constraint(lits, 1)

		clause = str(parts)+" "+str(hard) 
		for x in lits:
			clause = clause + " " + str(x)
		clause = clause + " 0\n"
		if complete_pwcnf:
			formula = formula + clause
		else:
			print(clause[:-1])
		clauses += 1


	# Soft clauses
	for x in range(0,tbls):
		for y in range(0,max(tags)+1):
			clauses += 1
			if pwcnf == 0:
				if complete_pwcnf:
					formula = formula + str(x+2)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0\n"
				else:
					print(str(x+2)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0")
			else:
				if complete_pwcnf:
					formula = formula + str(y+2)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0\n"
				else:
					print(str(y+2)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0")

def main():
	global complete_pwcnf
	parse_csv()
	# print(persons)
	# print(tags)
	# print(max_tbls)
	
	encoding()
	# print("TablePerson", TablePerson)
	# print("TableTag", TableTag)
	if complete_pwcnf:
		print("p pwcnf " + str(v) + " " + str(clauses) + " " + str(hard) + " {p}".format(p=tbls+1 if pwcnf == 0 else max(tags)+2))
		print(formula, end="")
	else:
		print(str(v))
		print(str(clauses))

if __name__ == "__main__":
	main()
