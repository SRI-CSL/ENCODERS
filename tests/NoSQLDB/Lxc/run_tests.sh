#!/bin/bash
rm -rf ./TestEvaluationFramework/Testsuite/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py nosql_test_spec.json
cd ../
ls -d Testsuite/* > nosql_test_list 
./testrunner.sh nosql_test_list
cd ../
