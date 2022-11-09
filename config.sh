#!/usr/bin/env bash
#Title			: config.sh
#Usage			: bash config.sh
#Author			: pmorvalho
#Date			: November 09, 2022
#Description		: Configuration file
#Notes			: 
# (C) Copyright 2022 Pedro Orvalho.
#==============================================================================

# to install PySAT
pip3 install installation/python-sat-0.1.7.dev19.tar.gz

chmod +x *.sh upPySAT/*.py

# to unzip the other solvers' binaries
cd solvers
unzip MaxHS.zip
unzip UWrMaxSAT.zip
unzip UpMax.zip
cd ..

# to unzip the datasets
unzip -f evaluation_dataset.zip
unzip -f representative_subset.zip
