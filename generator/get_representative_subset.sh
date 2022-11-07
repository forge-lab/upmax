#!/usr/bin/env bash
#Title			: get_representative_subset.sh
#Usage			: bash get_representative_subset.sh
#Author			: pmorvalho
#Date			: November 07, 2022
#Description		: Gathers a representative subset of the initial dataset (10% of the instances).
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

msc_main_dir="/data/tmp/tacas23/minimum-sum-coloring"
sa_main_dir="/data/tmp/tacas23/seating-assignment"

# Minimum Sum Coloring (MSC) Problem
msc_dir="/data/tmp/tacas23/representative_subset/minimum-sum-coloring"
mkdir -p $msc_dir/"pwcnfs-color-based" $msc_dir/"pwcnfs-vertex-based" $msc_dir/"wcnfs"

# Seating Assignment Problem
sa_dir="/data/tmp/tacas23/representative_subset/seating-assignment"
mkdir -p $sa_dir"/pwcnfs-tables-based" $sa_dir/"pwcnfs-tags-based" $sa_dir/"wcnfs"

n_instances=1000
for((i=1; i<=$n_instances; i=i+50))
do
    # currently we are collecting 10% of the initial dataset.
    nums=$(shuf -i $i-$((i+49)) -n5)
    instances=($nums)
    for((j=0;j<${#instances[@]};j++));
    do
	f=${instances[$j]}
	# MSC problem
	cp $msc_main_dir/wcnfs/g$f-* $msc_dir/wcnfs/.
	cp $msc_main_dir/*color*/g$f-* $msc_dir/*color*/.
	cp $msc_main_dir/*vertex*/g$f-* $msc_dir/*vertex*/.
	
	# SA problem
	cp $sa_main_dir/wcnfs/s$f-* $sa_dir/wcnfs/.
	cp $sa_main_dir/*table*/s$f-* $sa_dir/*table*/.
	cp $sa_main_dir/*tags*/s$f-* $sa_dir/*tags*/.
    done
done
