#!/bin/bash
rm -rf ./TestEvaluationFramework/Testsuite/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py evaluation.json
cd ../
ls -d Testsuite/* > evaluation
./testrunner.sh evaluation | tee /tmp/output 
OUTPUT_DIR="1hopbf_grid_results_$(date '+%s')"
mkdir ${OUTPUT_DIR}

(echo '"1hop-TX" ' $(cat $(cat /tmp/output | grep "Log files"  | grep "500-1hop" | awk '{print $5;}')/apps_output | grep "TX bytes" | awk '{print $2;}' | awk -F: '{SUM=SUM+$2;} END {print SUM;}')) >> ${OUTPUT_DIR}/overhead.csv
(echo '"default-TX" ' $(cat $(cat /tmp/output | grep "Log files"  | grep "500-default" | awk '{print $5;}')/apps_output | grep "TX bytes" | awk '{print $2;}' | awk -F: '{SUM=SUM+$2;} END {print SUM;}')) >> ${OUTPUT_DIR}/overhead.csv

(echo '"1hop-RX" ' $(cat $(cat /tmp/output | grep "Log files"  | grep "500-1hop" | awk '{print $5;}')/apps_output | grep "RX bytes" | awk '{print $6;}' | awk -F: '{SUM=SUM+$2;} END {print SUM;}')) >> ${OUTPUT_DIR}/overhead.csv
(echo '"default-RX" ' $(cat $(cat /tmp/output | grep "Log files"  | grep "500-default" | awk '{print $5;}')/apps_output | grep "RX bytes" | awk '{print $6;}' | awk -F: '{SUM=SUM+$2;} END {print SUM;}')) >> ${OUTPUT_DIR}/overhead.csv

rm /tmp/output
cp ../grid.plot ${OUTPUT_DIR}
cd ${OUTPUT_DIR}
gnuplot grid.plot
cd ../
mv ${OUTPUT_DIR} ../
cd ../
