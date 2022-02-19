generator.py
============

```Usage: python3 generator.py -n <num_node> -p <edge_prob>``` 

Generates a file with the following lines:
- g <#nodes> <#edges> <#colors>
- <#edges> lines where each line contains the information of an edge i.e., the ids of its vertices

(The number of colors is the maximum number of edges each node has + 1 i.e. if n3 is the node with more connection (20) then the maximum number of colors will be 20+1=21).


Instance generated filename explanation:
e.g g87-n30-p10.g
This instance (87) was generated with the following command: ```python3 generator.py -n 30 -p 10```
Which means that the instance has 30 nodes. The graph was a probability of edges of 10%.




msc.py
=======

```Usage: msc.py [-h] [-f [INPUT_FILE]] [-d] [-pc] [-pn] [-w]```

  -f [INPUT_FILE], --input_file [INPUT_FILE] input file
  
optional arguments:
  -h, --help            show this help message and exit
  -d, --debbug          Debug info
  -pc, --pwcnf_colors   Output format: PWCNF (colors)
  -pn, --pwcnf_nodes    Output format: PWCNF (nodes) (DEFAULT)
  -w, --wcnf            Output format: WCNF  -f [INPUT_FILE], --input_file [INPUT_FILE] : input file generated using vmc-generator




gen_pwcnfs.sh
=======

'''bash gen_pwcnfs.sh'''

Generates pwcnfs n_instances for the minimum sum coloring problem using the scripts above.

Script's variables:
- n_instances - number of total instances the user wants
- p_min - the minimum edge probability each instance can have
- p_max - the maximum edge probability each instance can have
- data_dir - the path to the output directory where all the pwcnfs and instances will be put.
- n_min - minimum number of nodes per instance
- n_max - maximum number of nodes per instance

This script creates 20 parallel processes running with different minimum and maximum number of nodes per instances. Hence, in the end, these two variables will take values from 5 (resp. 10)  to 80 (resp. 85).
