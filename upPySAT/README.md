upRC2.py
============

UpRC2 is an adaptation of the RC2 Solver for PWCNFs i.e. takes advantage of user partitioning of the soft clauses of a WCNF formula

'''Usage: upRC2.py [-h] [-f PWCNF] [-nup] [-v]'''

optional arguments:
  -h, --help            show this help message and exit
  -f PWCNF, --pwcnf PWCNF
                        PWCNF formula.
  -nup, --no_up         Calls the MaxSAT solver (RC2) without considering the partitions present in the formula.
  -v, --verbose         Prints debugging information.


upHitman.py
============

UpHitman is a MaxHS-like Solver adaptation for PWCNFs based on user partitionig of the soft clauses.

'''Usage: upHitman.py [-h] [-f PWCNF] [-nup] [-v]'''

optional arguments:
  -h, --help            show this help message and exit
  -f PWCNF, --pwcnf PWCNF
                        PWCNF formula.
  -nup, --no_up         Calls the MaxSAT solver (Hitman) without considering the partitions present in the formula.
  -v, --verbose         Prints debugging information.


pwcnf.py
============

Class for manipulating partition-based partial weighted CNF formulas (PWCNFs). 