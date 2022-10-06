#!/usr/bin/env bash
#Title			: gen_pwcnfs.sh
#Usage			: bash gen_pwcnfs.sh
#Author			: pmorvalho
#Date			: February 17, 2022
#Description	        : Generates pwcnfs instances for the seating assignment problem, giving the min and max values of people and the minimum and max possible tags per person
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

n_instances=1000
tags_min=6
tags_max=10    
#data_dir=$(pwd)
data_dir="/data/tmp/pwcnfs/seating-assignment-smaller"
mkdir -p $data_dir $data_dir/outputs $data_dir/instances $data_dir/pwcnfs-tables-based $data_dir/pwcnfs-tags-based

build_pwcnf(){
   file_name=$1
   clauses=$(tail -n 1 $file_name.out)
   vars=$(tail -n 2 $file_name.out | head -n 1)
   head -n 1 $file_name.out | sed -e 's/XX/'$vars' '$clauses'/g' >  $file_name.pwcnf
   sed -n 2,$((clauses+1))p $file_name.out >> $file_name.pwcnf
   rm $file_name.out
   gzip $file_name.pwcnf
}

gen_instances(){
# $1 is the beginning value for the for-cycle and $2 the limit.
# $3 is the minimum number of servers and $4 the maximum 
for((g=$1; g<=$2; g++))
do  
   p=$(python -c "import random; print(random.randint("$3","$4"))")
   t=$(python -c "import random; print(random.randint("$tags_min","$tags_max"))")
   #max_tags_per_person=$(python -c "import random; print(random.randint(int(0.4*"$tags_min"),int(0.8*"$tags_min")))")
   max_tags_per_person=$(python -c "print(int(0.8*"$t"))")
   i_name=s$g-p$p-tgs$t-mt$max_tags_per_person
   python3 generator.py $t $p $max_tags_per_person > $data_dir/instances/$i_name.sa
   num_tables=$(python -c "print(int(0.2*"$p"))")
   min_per_table=$(python -c "print(int(0.8*int("$p"/"$num_tables")))")
   max_per_table=$(python -c "print(int(1.3*int("$p"/"$num_tables")))")
   pwcnf_name=$i_name-tabs$num_tables-$min_per_table-$max_per_table 
   python3 seat.py $num_tables $min_per_table $max_per_table $data_dir/instances/$i_name.sa 0 > $data_dir/pwcnfs-tables-based/$pwcnf_name.out
   build_pwcnf $data_dir/pwcnfs-tables-based/$pwcnf_name
   timeout 600s ../../open-wbo -formula=2 $data_dir/pwcnfs-tables-based/$pwcnf_name.pwcnf.gz > /tmp/$pwcnf_name.out
   # Test if instance is unsat
   if [[ $(grep "UNSAT" /tmp/$pwcnf_name.out) ]]; then
       echo $g "UNSAT"	   
       rm  $data_dir/instances/$i_name.sa
       rm  $data_dir/pwcnfs-tables-based/$pwcnf_name.pwcnf.gz
       g=$((g-1))
       continue
   fi  
   python3 seat.py $num_tables $min_per_table $max_per_table $data_dir/instances/$i_name.sa 1 > $data_dir/pwcnfs-tags-based/$pwcnf_name.out
   build_pwcnf $data_dir/pwcnfs-tags-based/$pwcnf_name
   cp /tmp/$pwcnf_name.out $data_dir/outputs/.
done
}

# calculations from 18-23 to 38-43 since we increment +1 20 times
persons_min=18
persons_max=23
for((i=1; i<=$n_instances; i=i+51))
do
    echo "Generating instances from "$i" to "$((i+51))
    gen_instances $i $((i+50)) $persons_min $persons_max &
    persons_min=$((persons_min+1))
    persons_max=$((persons_max+1))
done     
