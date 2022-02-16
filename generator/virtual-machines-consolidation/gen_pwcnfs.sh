#!/usr/bin/env bash
#Title			: gen_pwcnfs.sh
#Usage			: bash gen_pwcnfs.sh
#Author			: pmorvalho
#Date			: February 16, 2022
#Description	        : Generates pwcnfs instances for the VMC problem, giving the  min and max values of servers and the percentage of usage
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_instances=1000
per_timeout=150  # we only want at most 15% of timeouts
servers_min=10
servers_max=40
vms_min=20
vms_max=45
timeouts=0    

#data_dir=$(pwd)
data_dir="/data/tmp/pwcnfs/virtual-machine-consolidation"
mkdir -p $data_dir $data_dir/outputs $data_dir/instances $data_dir/unweighted/pwcnfs-server-based $data_dir/unweighted/pwcnfs-vm-based $data_dir/weighted/pwcnfs-server-based $data_dir/weighted/pwcnfs-vm-based

gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit. 
for((g=$1; g<=$2; g++))
do  
   echo $g
   s=$(python -c "import random; print(random.randint("$servers_min","$servers_max"))")
   u=$(python -c "import random; print(random.randint("$vms_min","$vms_max"))")
   i_name=vmc$g-s$s-u$u
   ./vmc-generator $s $u $g > $data_dir/instances/$i_name.vmc
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -uw > $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf
   gzip $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf
   timeout 600s ../../open-wbo -formula=2 $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf.gz > /tmp/$i_name.out
   # test timrout
   exit_status=$?
   if [[ $exit_status -eq 124 ]]; then
       if [[ $timeout -ge 150 ]]; then # we only want at most 15% of timeouts
	   rm  $data_dir/instances/$i_name.vmc
	   rm  $data_dir/unweighted/pwcnfs-server-based/$i_name.pwcnf.gz
	   i=$((i-1))
	   continue
       else
	   timeout=$((timeout+1))
       fi
    fi
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$i_name.out) ]]; then
       rm  $data_dir/instances/$i_name.vmc
       rm  $data_dir/pwcnfs-server-based/$i_name.pwcnf.gz
       i=$((i-1))
       continue
   fi
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -pv -uw > $data_dir/unweighted/pwcnfs-vm-based/$i_name.pwcnf
   gzip $data_dir/unweighted/pwcnfs-vm-based/$i_name.pwcnf
   python3 vmc.py -f $data_dir/instances/$i_name.vmc > $data_dir/weighted/pwcnfs-server-based/$i_name.pwcnf
   gzip $data_dir/weighted/pwcnfs-server-based/$i_name.pwcnf
   python3 vmc.py -f $data_dir/instances/$i_name.vmc -pv > $data_dir/weighted/pwcnfs-vm-based/$i_name.pwcnf
   gzip $data_dir/weighted/pwcnfs-vm-based/$i_name.pwcnf
   cp /tmp/$i_name.out $data_dir/outputs/.
done
}

for((i=1; i<=1000; i=i+100))
do
    gen_instances $i $((i+99)) &
done     

