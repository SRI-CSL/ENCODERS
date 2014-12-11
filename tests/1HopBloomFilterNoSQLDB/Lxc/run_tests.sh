#!/bin/bash
rm -rf ./TestEvaluationFramework/Testsuite/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py 1hop_test_spec.json
cd ../
ls -d Testsuite/* > 1hop_test_list 
./testrunner.sh 1hop_test_list
cd ../
