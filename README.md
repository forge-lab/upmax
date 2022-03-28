# UpMax : User partitioning for MaxSAT

We propose a new format called ``pwcnf`` for defining MaxSAT formulas that extends the current ``wcnf`` format, allowing the user to propose how to partition MaxSAT formulas based on his domain-knowledge of the problem to be solved. Moreover, until now, the partitioning of MaxSAT formulas is interconnected to the subsequent algorithm to be used. Therefore, it is not easy to define and test new partitioning methods with several MaxSAT algorithms developed by different people. This new format allows to decouple these components, hopefully facilitating the appearance of new partition methods for MaxSAT formulas. 

![Overview of UpMax](https://raw.githubusercontent.com/forge-lab/upmax/master/assets/images/UpMax-overview.pdf)

The above figure illustrates the schematic view of the UpMax architecture based on decoupling of the MaxSAT solving algorithm from the split of the clauses on the MaxSAT formula.
