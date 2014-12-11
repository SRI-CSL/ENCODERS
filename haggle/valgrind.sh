#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Joshua Joy (JJ, jjoy)

valgrind --verbose --tool=memcheck --show-reachable=yes --track-fds=yes --trace-children=yes \
	--track-origins=yes --leak-check=full --log-file=valgrind$1.log bin/haggle -f

