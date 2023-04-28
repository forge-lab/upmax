# UpMax : User partitioning for MaxSAT

We propose a new format called ``pwcnf`` for defining MaxSAT formulas that extends the current ``wcnf`` format, allowing the user to propose how to partition MaxSAT formulas based on his domain-knowledge of the problem to be solved. Moreover, until now, the partitioning of MaxSAT formulas is interconnected to the subsequent algorithm to be used. Therefore, it is not easy to define and test new partitioning methods with several MaxSAT algorithms developed by different people. This new format allows to decouple these components, hopefully facilitating the appearance of new partition methods for MaxSAT formulas. 

![Overview of UpMax](https://raw.githubusercontent.com/forge-lab/upmax/master/assets/images/UpMax-overview.jpg)

The above figure illustrates the schematic view of the UpMax architecture based on decoupling of the MaxSAT solving algorithm from the split of the clauses on the MaxSAT formula.


## ``pwcnf`` file format

We propose a new generic format for MaxSAT, ``pwcnf``, where the partition scheme can be defined and solvers can take advantage of it. The ``pwcnf`` format starts with a header:

``p pwcnf n_vars n_clauses topw n_part``

and each line in the body is of the form:

``[part] [weight] [literals] 0``

In the header, ``n_vars`` and ``n_clauses`` are respectively the number of variables and clauses of the formula. ``topw`` is the weight assigned to the hard clauses and ``n_part`` corresponds to the number of partitions. Each partition label ``[part]`` must be a positive integer from 1 to ``n_part``.

## Automatic generation of ``pwcnf`` files from ``wcnf``

The user can either encode the problem directly into ``pwcnf`` format or it can transform an existing ``wcnf`` to ``pwcnf`` using an automatic extraction of partition implemented in **UpMax**. 

The following partitioning strategies can be used:

```
-graph-type   = <int32>  [   0 ..    3] (default: 2)
Graph type (0=vig, 1=cvig, 2=res, 3=random).
```

A ``pwcnf`` file can be created with the option ``-upfile``:

```
-upfile     = <string>
PWCNF file with automatic partition.
```
For example, suppose you want to automatically generate a ``pwcnf`` file from an existing ``wcnf`` file called ``filename.wcnf`` using the VIG partitioning strategy. Then you can accomplish this task with the command:

``./upmax -graph-type=0 -upfile=filename.pwcnf filename.wcnf``

## Solving ``pwcnf`` formulas

The option ``-formula`` controls the kind of formula being solved by the solver. The following options are available in **UpMax**:

```  
-formula      = <int32>  [   0 ..    2] (default: 0)
Type of formula (0=WCNF, 1=OPB, 2=PWCNF).
```

**UpMax** currently supports three MaxSAT solver that can be used with user partitioning information.  The option ``-algorithm`` controls the algorithm being used. 

```
-algorithm    = <int32>  [   0 ..    2] (default: 1)
Search algorithm (0=wbo,1=msu3,2=oll)
```

To use the user partitioning from the ``pwcnf`` file the option ``-upmax`` should be as well. 

```
-upmax, -no-upmax                       (default: on)
User partitioning.
```

For instance, consider that you want to solver the formula ``filename.pwcnf`` with algorithm ``oll`` using the ``upmax`` option then you should run **UpMax** as follows:

``./upmax -algorithm=2 -formula=2 -upmax filename.pwcnf``

## UpPySAT

Our README explaining how to run PySAT with user-based partitions can be found [here](https://github.com/forge-lab/upmax/blob/master/upPySAT/README.md).
