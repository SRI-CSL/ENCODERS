To run gps_test, you must use the configuration file at: cbmem/tests/config-gps-tests.xml and copy it to your ~/.Haggle/config.xml

Simply run the test 'gps_test'.   This test runs by visual inspection.   After the initial setup, it will publish beacon data.    It is also setup to receive the same beacon data.    For every beacon published, you should see a message it is recieved, such as: 


pvt Kong publishing beacon 10/20...


        Received GPS Beacon: pvt Kong is at elsewhere at time 1350248998

pvt Kong publishing obsolete data (should not be received)...

pvt Kong publishing beacon 11/20...


        Received GPS Beacon: pvt Kong is at elsewhere at time 1350249004

pvt Kong publishing beacon 12/20...


        Received GPS Beacon: pvt Kong is at elsewhere at time 1350249009

Note that, obsolete data (e.g. timestamp is older than last timestamp) is NOT published.

At the end, you may view the haggle database, and it will show only 3 Beacon objects (one from Mothra, One from Gojira, One from Kong).

Based upon these visual inspections, you may verify the test passed or failed.

