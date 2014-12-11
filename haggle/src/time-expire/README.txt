This program sends a timed packet into haggle.

You specify the type of timed packet:
Relative - After receiving this packet, you have 'x' seconds of life.
Absolute - After receiving this packet, you will be deleted at time 'x'.

This set up 2 things:
1.  Insert the object into the database
2.  At the appropriate time, delete the dataobject and clear the bloom filter.

Options:
-n <name>  Optional:  You can give a different client name.
-r         Choose relative timing.
-a         Choose absolute timing.
-t <num>   Specify the relative time value or absolute time value
-t+<num>   Choose an absolute time value that is NOW+num seconds.


Examples:
Both examples do the same thing:

 ./timeexpire -r -t 60

 ./timeexpire -a -t +60

./timeexpire -n test -a -t 198328232


If the absolute timestamp is within a threshold, the data is not stored.
Example config.xml data is within the appropriate file:
ReplacementTotalOrder.cpp
ReplacementAbsTTL.cpp
ReplacementRelTTL.cpp

KNOWN BUGS:
1.  Currently crashes if no replacement config.xml file defined
2.  tested rel/abs individually and together.
3.  Android make not implemented
4.  Needs to be compiled by hand

