To run gps_person, you must use the configuration file at: cbmem/tests/config-gps-tests.xml and copy it to your ~/.Haggle/config.xml

This utility exists if you wish to customize the GPS testing.   This will allow you to either recieve your own, none, or all but your, GPS beacons.   In addition, you can publish GPS beacon objects by specifying the correct parameters.

valid options are:
        -r 0/1/2         0=no receive GPS beacons, 1=receive all, 2=recieve all but your own
        -t <seconds>     Number seconds (base) between GPS broadcasts
        -j <seconds>     +-value for jitter off base GPS broadcast time
        -m <number       maximum gps beacon broadcasts
        -l <string>      location in ascii
        -n <name>        unique name of individual or identifier

