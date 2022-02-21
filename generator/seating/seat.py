#!/usr/bin/python    

import sys

if len(sys.argv) < 6:
	print "Usage: python seat.py <#tables> <min #persons per table> <max #persons per table> <generator file> <0=tables,1=tags>"
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
	TableTag = [[0 for x in range(tbls)] for y in range(len(tags))]
	soft = len(tags)*tbls
	hard = soft+1
	if pwcnf == 0:
		parts = tbls+1
	else:
		parts = len(tags)+1

def sinz(lits, k):
	global v
	global formula
	global clauses

	TableSeq = [[0 for x in range(k)] for y in range(len(lits))]
	for x in range(0,k):
		for y in range(0,len(lits)):
			TableSeq[y][x] = v
			v = v + 1

	# print TableSeq

	formula = formula + str(parts)+" "+ str(hard) + " " + str(-lits[0]) + " " + str(TableSeq[0][0]) + " 0\n"
	formula = formula + str(parts)+" "+str(hard) + " " + str(-lits[len(lits)-1]) + " " + str(-TableSeq[len(lits)-2][k-1]) + " 0\n"
	clauses = clauses + 2
	for x in range(1,k):
		formula = formula + str(parts)+" "+str(hard) + " " + str(-TableSeq[0][x]) + " 0\n"
		clauses = clauses + 1

	for i in range(1,len(lits)-1):
		formula = formula + str(parts)+" "+str(hard) + " " + str(-lits[i]) + " " + str(TableSeq[i][0]) + " 0\n"
		formula = formula + str(parts)+" "+str(hard) + " " + str(-TableSeq[i-1][0]) + " " + str(TableSeq[i][0]) + " 0\n"
		formula = formula + str(parts)+" "+str(hard) + " " + str(-lits[i]) + " " + str(-TableSeq[i-1][k-1]) + " 0\n"
		clauses = clauses + 3
		for j in range(1, k):
			formula = formula + str(parts)+" "+str(hard) + " " + str(-TableSeq[i-1][j]) + " " + str(TableSeq[i][j]) + " 0\n"
			formula = formula + str(parts)+" "+str(hard) + " " + str(-lits[i]) +  " " + str(-TableSeq[i-1][j-1]) + " " + str(TableSeq[i][j]) + " 0\n"
			clauses = clauses + 2


def encoding():
	# create variables
	global v
	global formula
	global clauses
	for x in range(0,tbls):
		for y in range(0,len(persons)):
			TablePerson[y][x] = v
			v = v + 1
	
	for x in range(0,tbls):
		for y in range(0,len(tags)):
			TableTag[y][x] = v
			v = v + 1

	# If a person is seated in a table then that tag is true
	for p in persons.keys():
		for tag in persons[p]:
			for table in range(0,tbls):
				formula = formula + str(parts)+" "+str(hard) + " " + str(-TablePerson[p][table]) + " " + str(TableTag[tag][table]) + " 0\n"
				clauses = clauses + 1

	# Each table has at most k persons
	for table in range(0,tbls):
		lits_pos = []
		lits_neg = []
		for p in range(len(persons)):
			lits_pos.append(TablePerson[p][table])
			lits_neg.append(-TablePerson[p][table])
		# print lits_pos
		# print max_tbls
		sinz(lits_pos, max_tbls)
		sinz(lits_neg, len(lits_neg)-min_tbls)

	# Each person is seated in at least one table
	for p in persons.keys():
		#for tag in persons[p]:
		lits = []
		for table in range(0,tbls):
			lits.append(TablePerson[p][table])
		# Each person is seated in at most one table
		sinz(lits, 1)

		clause = str(parts)+" "+str(hard) 
		for x in lits:
			clause = clause + " " + str(x)
		clause = clause + " 0\n"
		formula = formula + clause
		clauses + clauses + 1


	# Soft clauses
	for x in range(0,tbls):
		for y in range(0,len(tags)):
			if pwcnf == 0:
				formula = formula + str(x+1)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0\n"
				clauses = clauses + 1
			else:
				formula = formula + str(y+1)+" "+str(1) + " " + str(-TableTag[y][x]) + " 0\n"
				clauses = clauses + 1

def main():
	parse_csv()
	# print persons
	# print tags
	#print max_tbls
	
	encoding()
	#print TablePerson
	# print TableTag
	print "p pwcnf " + str(v) + " " + str(clauses) + " " + str(hard) + " " + str(parts)
	print formula,

if __name__ == "__main__":
	main()
