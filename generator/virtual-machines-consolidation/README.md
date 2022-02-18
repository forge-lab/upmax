vmc-generator.cc (vmm's)
============

```Usage:
make
./vmc-generator <#servers> <%usage> <seed>``` 

Generates a file with the following lines:
- <#servers>
- <#servers> lines where each line contains its memory and CPU info.
- <#VMs>
- <#VMs> lines where each line contains the respective VM's memory, CPU info and the server's id where the VM is initially launched.
- <K>

(K = The number of servers the user wants active at the end, K < <#servers>)


Instance generated filename explanation:
e.g vmc87-s30-u20.vmc
This instance was generated with the following command: ```./vmc-generator 30 20 87```
Which means that the instance has 30 servers with 20% of VMs and the seed is 87. 


vmc.py
=======

```Usage: python vmc.py [-h] [-f [INPUT_FILE]] [-d] [-cpwcnf] [-ps] [-pv] [-uw]```

  -f [INPUT_FILE], --input_file [INPUT_FILE] : input file generated using vmc-generator

optional arguments:
  -h, --help            : show this help message and exit
  -d, --debbug          : Debug info
  -cpwcnf, --complete_pwcnf : Writes the complete PWCNF with the value of variables and clauses in the header and not in the end of the file. This approach is quite slower (SLOW).
  -ps, --pwcnf_servers  : Output format: PWCNF (colors) (DEFAULT)
  -pv, --pwcnf_vms      : Output format: PWCNF (nodes)
  -uw, --unweighted     : Unweighted Version of the VMC problem. If this flag is not used then the weighted version is used.



gen_pwcnfs.sh
=======

'''bash gen_pwcnfs.sh'''

Generates pwcnfs n_instances for the virtual machine consolidation problem using the scripts above.
Script's variables:
- n_instances - number of total instances the user wants
- vms_min - the minimum number of servers' usage each instances can have
- vms_max - the maximum number of servers' usage each instances can have
- data_dir - the path to the output directory where all the pwcnfs and instances will be put.
- servers_min - minimum number of servers per instance
- servers_max - maximum number of servers per instance

This script creates 20 parallel processes running with different minimum and maximum number of servers per instances. Hence, in the end, these two variables will take values from 3 (resp. 5) to 43 (resp. 45).