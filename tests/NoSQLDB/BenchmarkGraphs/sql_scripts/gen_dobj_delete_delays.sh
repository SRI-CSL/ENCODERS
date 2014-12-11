#!/bin/bash
grep "Data object .* deleted successfully" haggle.log  | awk '{print $8;}'  > dobj_delete_delays.dat
#cat dobj_insert_delays.dat  | sort -n | tail -5 > bad_insert_delays
#grep -v -f bad_insert_delays dobj_insert_delays.dat > dobj_insert_delays.dat.temp
#mv dobj_insert_delays.dat.temp  dobj_insert_delays.dat
