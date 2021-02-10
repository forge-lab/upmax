WCNF formula without user provided partitions (default):
```-formula = 0```

WCNF formula with user provided partitions (only used by the user-based algorithm; the other algorithms will ignore the user partition):
```-formula = 2```

Running the user-based partition algorithm:
```./open-wbo -formula=2 -algorithm=4 <formula>```

Running the auto-generated partition algorithm:
```./open-wbo -formula=0 -algorithm=3 <formula>```

Running the baseline algorithm without partitions:
```./open-wbo -formula=0 -algorithm=2 <formula>```

Find all optimal solutions that falsify a different set of soft clauses:
```-all-opt```

Find all optimal solutions taking into consideration all variables of the formula:
```-all-opt -all-vars```
