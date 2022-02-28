#!/usr/bin/env bash
#Title			: translatespwcnfs2wcnfs.sh
#Usage			: bash translatespwcnfs2wcnfs.sh
#Author			: pmorvalho
#Date			: February 28, 2022
#Description     	: Translated pwcnfs to wcnf (Seating and MSC Problems)
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

pwcnf2wcnf(){
  for f in $(find $pwcnfs_dir/*.pwcnf.gz -type f);
  do
      out_name=$data_dir/"$(basename $f .pwcnf.gz)"
      python3 pwcnf2wcnf.py -i $f $out_name
      gzip out_name
  done
}

data_dir="/data/tmp/wcnfs/seating-assignment-smaller"
pwcnfs_dir="/data/tmp/pwcnfs/seating-assignment-smaller/pwcnfs-tables-based"
mkdir -p $data_dir
pwcnf2wcnf

data_dir="/data/tmp/wcnfs/minimum-sum-coloring-p2-10-n5-80/"
pwcnfs_dir="/data/tmp/pwcnfs/minimum-sum-coloring-p2-10-n5-80/pwcnfs-color-based"
mkdir -p $data_dir
pwcnf2wcnf
