#!/bin/bash
rm -rf ./TestEvaluationFramework/Testsuite/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py send_priority.json
cd ../
ls -d Testsuite/* > send_priority
./testrunner.sh send_priority
cd ../
