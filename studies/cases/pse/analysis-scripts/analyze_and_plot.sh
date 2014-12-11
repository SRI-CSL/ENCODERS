#!/bin/bash



# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function exit_on_fail () {
    if [ $? -eq 0 ]
    then
        true
    else
        echo "failed! exiting"
        exit 1
    fi
}

NUM_PROCESSES=4
TEST_OUTPUT_DIRECTORY=../$1
BASE_NAME=$2

python ../../../framework/lxc/loganalysis/analyze.py -p ${NUM_PROCESSES} -l -d pse.db analysis.json $TEST_OUTPUT_DIRECTORY
exit_on_fail
python ../../../framework/lxc/loganalysis/plotter.py -p ${NUM_PROCESSES} plot_per_case.json $TEST_OUTPUT_DIRECTORY/*
exit_on_fail

rm -f tmp_plot_pse.json
python pse_plot_spec_generator.py ${BASE_NAME} ${TEST_OUTPUT_DIRECTORY} > tmp_plot_pse.json
exit_on_fail
python ../../../framework/lxc/loganalysis/plotter.py tmp_plot_pse.json
exit_on_fail

rm -f *.csv
rm -f tmp_plot_pse.json
