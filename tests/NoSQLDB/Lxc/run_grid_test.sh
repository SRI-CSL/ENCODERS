#!/bin/bash
rm -rf ./TestEvaluationFramework/Testsuite/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py nosql_grid_test.json
cd ../
ls -d Testsuite/* > nosql_grid_test_list 
./testrunner.sh nosql_grid_test_list | tee /tmp/output 
OUTPUT_DIR="nosql_grid_results_$(date '+%s')"
mkdir ${OUTPUT_DIR}
cat $(cat /tmp/output | grep "Log files"  | grep "\-nosql" | awk '{print $5;}')/apps_output | grep "Received," | awk -F, '{print $7;}' | sort -n -k1 | awk '{print $0, NR;}' > ${OUTPUT_DIR}/nosql.dat
cat $(cat /tmp/output | grep "Log files"  | grep "\-sql" | awk '{print $5;}')/apps_output | grep "Received," | awk -F, '{print $7;}' | sort -n -k1 | awk '{print $0, NR;}' > ${OUTPUT_DIR}/sql.dat
rm /tmp/output
cp ../grid.plot ${OUTPUT_DIR}
cd ${OUTPUT_DIR}
gnuplot grid.plot
cd ../
mv ${OUTPUT_DIR} ../
cd ../
