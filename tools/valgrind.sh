#!/bin/bash

valgrind --verbose --tool=memcheck --show-reachable=yes --track-fds=yes --trace-children=yes \
	--track-origins=yes --leak-check=full --log-file=valgrind$1.log /usr/local/bin/haggle -f

