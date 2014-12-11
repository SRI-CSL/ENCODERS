#!/bin/bash
grep " X" benchmark-*.log | awk '{print $1;}' > dobj_insert_delays.dat
