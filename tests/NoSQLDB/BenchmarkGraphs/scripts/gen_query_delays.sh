#!/bin/bash
grep " D" benchmark-*.log | awk '{print $1;}' > query_delays.dat
