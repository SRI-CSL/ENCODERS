#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

for dir in $(ls Testsuite); do
	pushd Testsuite/$dir &> /dev/null
	rm mobile*
	cp ../../mobile.imn.template .
	cp ../../emane/antennapattern.xml .
	cp ../../emane/antennaprofiles.xml .
	cp ../../emane/deployment.xml .
	cp ../../emane/eelgenerator.xml .
	cp ../../emane/events.xml .
	cp ../../emane/scenario.eel .
	popd &> /dev/null
done
