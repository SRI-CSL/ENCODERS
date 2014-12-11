Last modified: Sam Wood, 4/10/13

Relevant files & directories:

    - ../studies/framework/lxc/testrunner/testrunner.sh : The automated testrunner that executes tests generated
                      by the TEST GENERATOR.
    - ../studies/framework/lxc/testrunner/env_check.sh  : A script that returns status code 0 if the environment
                      is OK for the testrunner to executed, and 1 otherwise.
                      Used by the TESTRUNNER.
    - sample_test_list : An example list of testcases to run; passed as input
                         to TESTRUNNER.
    - sample_output : An example output from a test run.
    - Testsuite/* : Example test cases referenced by 'sample_test_list'.


=====
TEST RUNNER:
=====

NOTE: TESTRUNNER expects the modified version of core that supports mobility
without a person manually pressing the green "start" arrow (core -s...).
Also, remember to edit /etc/core/core.conf and make sure the line that looks
like: "custom_services_dir = /home/username/.core/myservices" is uncommented
and points to your own home directory. If multiple users are sharing a machine,
then each user will have to edit this file when they take over control of the
machine. Also, it is recommended to clear out the /tmp directory when changing
users, as files generated during testing may not be completely cleaned up, and
may cause permissions issues.

We also modified core-4.3 to not generate large log files in the tmp
directory. Both of these changes can be found in the version of core
included in: core-4.3-modified.zip. Testrunner uses the "cpulimit" tool to
control the amount of cpu resources that is given to each lxc.

A testcase is a directory consisting of a core imn file multiple bash scripts
that the testrunner uses to manage the test.

By default, running "./testrunner.sh" will search for a "Testsuite/" directory
and execute all the tests in this directory.

Alternatively, one can specify a file containing a list of tests, and an output
file for statistics, as follows:

./testrunner.sh sample_test_list sample_output.csv

`sample_test_list' contains a list of test cases to be executed, as follows:

sample_test_list:

Testsuite/001-200x200-N2-D500K-K1-S45-NONE-r1/ 1
Testsuite/001-200x200-N2-D500K-K1-S45-NONE-r2/ 2
#Testsuite/001-200x200-N2-D500K-K1-S45-NONE-r2/ 1

The first column specifies a test that was generated using the TEST GENERATOR.
The (optional) second column specifies the number of iterations to run the
same test, this may be neccessary to compute the variance across runs (i.e.
investigate operating system effects).
Lines preceeded with a "#" are ignored, allowing users to easily enable/disable
tests.

Upon completion, the testrunner will report the number of tests executed, and
indicate how many failed and passed. Upon failure, the output directory
should be indicated to inform the user of where logfiles are located, for
debugging purposes.

Upon completion, the sample_output.csv file will contain information in the format:

sample_output:

001-200x200-N2-D500K-K1-S45-NONE-r1,1357759449,Y,1127834,1140501,4,2,0.7105,0.642351
001-200x200-N2-D500K-K1-S45-NONE-r2,1357759544,Y,574449,588726,1,1,0.678,0
001-200x200-N2-D500K-K1-S45-NONE-r2,1357759640,Y,578093,589521,1,1,0.655,0

Where the columns are as follows:

column 1: name           - the test name
column 2: date           - the timestamp
column 3: passed         - the status of the test (Y = passed, N = failed)
column 4: hash           - git hash of the version
column 5: num_nodes      - number of nodes
column 6: duration_s     - duration (seconds)
column 7: tx_bytes       - total transmitted bytes (ifconfig)
column 8: rx_bytes       - total receive bytes (ifconfig)
column 9: do_delivered   - total data objects delivered to interested apps
column 10: do_published  - total data objects published
column 11: avg_delay     - average data object delivery delay
column 12: std_dev_delay - standard deviation
column 13: has_stats     - detailed stats collected?
column 14: do_persis_inserted - persistent data objects inserted
column 15: do_persis_deleted  - persistent data objects deleted
column 16: do_inserted        - data objects inserted
column 17: do_deleted         - data objects deleted
column 18: do_bytes_outgoing  - data object bytes outgoing
column 19: do_bytes_sent      - data object bytes incoming
column 20: do_bytes_incoming  - data objects bytes incoming
column 21: do_bytes_received  - data objects bytes fully received
column 22: do_not_sent   - data objects not sent
column 23: do_sent       - data objects fully sent
column 24: do_sent_ack   - data objects fully sent and ACKed
column 25: do_out        - data objcts outgoing
column 26: non_control_do_out - non-control data objects outgoing
column 27: do_in         - data objects incoming
column 28: non_control_do_in  - non-control data objects incoming
column 29: do_not_recv   - data objects not received
column 30: do_fully_recv - data objects fully received
column 31: do_reject     - data objects rejected
column 32: do_accept     - data objects accepted
column 33: do_ack        - data objects acknowledged
column 34: frag_rej      - data object fragments rejected
column 35: block_rej     - data object blocks rejected
column 36: nd_bytes_sent - number of node description bytes sent
column 37: nd_bytes_recv - number of node description bytes received
column 38: nd_sent       - number of node descriptions sent
column 39: nd_recv       - number of node descriptions received

NOTE on the delay average and variance: we compute these values in a
non-standard form to take into account the two modes of data transfer in
haggle: for each received data object, if the interest was registered after
the data object was published then we use the delay from the interest
registration until the data object was received. Otherwise, we compute the
delay as the delay from when the data object was published until it was
received.

If the variance could not be computed (i.e. there was only 1 value), then we
report 0.

We STRONGLY recommend that the /tmp directory is mounted either on an SSD
or in memory. CORE writes many files to /tmp, and a slow disk can drastically
alter emulation results due to disk IO.

The testrunner uses HAGGLE_STAT messages to collect metrics and detect problems
during shutdown. If an error does occur (i.e. haggle does not shutdown due
to deadlock), then the testrunner will generate a core file and force quit
haggle. In this case, the database files and the coredumps will be saved
in the output directory. A test can also fail if haggletest generates an
error when connecting with the haggle daemon. These error messages are stored
in the "fail_log" file in the test output directory.

The output directory contains all the information that is needed to re-run
the test. We also store the files in ~/.Haggle and their sizes (at each node)
in separate log files (i.e. nX.sizes.log). We also store a list of the data
object ids and their sizes (as reported by the database) in
nX.haggle.db.hash_sizes.log. nX.resources.log contains a triple:
(timestamp, cpu usage, memory usage) of the haggle daemon periodically
during the test. This eases debugging during a crash by giving us a rough
estimate of when haggle crashed (the cpu and memory fields will be empty).

Optional Arugments:
----

"-c <haggled path>":
You can specify the path of the haggle executable to be tested using the
"-c" option. By default "/usr/local/bin/haggle" will be used.

"-d":
Debug mode. This will pause haggle at the end of the test, before shutting down
the instances to give the user time to manually inspect the lxc hosts.


Automated Emails:
----

Please read the `email_guide.txt' for instructions on configuring your
environment to send emails from the command line. Once this is done,
automated emails will be sent if the following environment
variables are set:

export TESTRUNNER_EMAIL_ENABLE=1
export TESTRUNNER_EMAIL_FROM="sam@suns-tech.com"
export TESTRUNNER_EMAIL_TO="sam@suns-tech.com"

where EMAIL_FROM is who message sender is, and EMAIL to is the message
receiver.
