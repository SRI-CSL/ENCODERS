#!/bin/bash
grep " Z" benchmark-*.log | awk '{print $1;}' > node_insert_delays.dat
