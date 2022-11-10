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
cd dependencies
pip3 install python-sat-0.1.7.dev19.tar.gz
pip3 install numpy-1.23.4-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
pip3 install contourpy-1.0.6-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
pip3 install python_dateutil-2.8.2-py2.py3-none-any.whl
pip3 install pyparsing-3.0.9-py3-none-any.whl
pip3 install Pillow-9.3.0-cp310-cp310-manylinux_2_28_x86_64.whl
pip3 install packaging-21.3-py3-none-any.whl
pip3 install kiwisolver-1.4.4-cp310-cp310-manylinux_2_12_x86_64.manylinux2010_x86_64.whl
pip3 install cycler-0.11.0-py3-none-any.whl
pip3 install fonttools-4.38.0-py3-none-any.whl
pip3 install decorator-5.1.1-py3-none-any.whl
pip3 install funcsigs-1.0.2-py2.py3-none-any.whl
pip3 install tempdir-0.7.1.tar.gz
pip3 install shutilwhich-1.1.0.tar.gz
pip3 install future-0.18.2.tar.gz
pip3 install data-0.4.tar.gz
pip3 install latex-0.7.0.tar.gz
pip3 install matplotlib-3.6.2-cp310-cp310-manylinux_2_17_x86_64.manylinux2014_x86_64.whl
cd ..
chmod +x scripts/mkplot/*.py
chmod +x scripts/*.sh *.sh upPySAT/*.py

# to unzip the other solvers' binaries
cd solvers
unzip MaxHS.zip
unzip UWrMaxSAT.zip
unzip UpMax.zip
cd ..

# to unzip the datasets
unzip evaluation_dataset.zip
