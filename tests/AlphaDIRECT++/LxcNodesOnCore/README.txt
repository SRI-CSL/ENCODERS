
INTERNAL CBMEN ONLY.

TestRunner README 

Status: Work in progress.

Last modified: 7/11/12
------------

WARNING:

testrunner.sh deletes some files in /tmp, replaces files in ~/.core/myservices,
and kills processes with the name "core". We do this to create a fresh
enviornment, but please be careful when running this program to ensure that
your data is not lost. 

DESCRIPTION: 

This document describes the automated test program, testrunner.sh, for 
CORE+Haggle simulations. The purpose of testrunner.sh is to automate test
cases in an easily extendible way. 

USAGE:

First, make sure that the proper version of haggle under test is installed 
in /usr/local/bin/haggle. 

For example:

git checkout direct
./autogen.sh
./configure.sh
make 
sudo make install

Make sure that "haggletest" is compiled and installed in your path.

Make sure that your "/etc/core/core.conf" has the following line:

custom_services_dir = /home/swood/.core/myservices

Where "swood" is replaced with your username.

Then type "./testrunner.sh" to run all of the tests in the "Testsuite/"
directory. 

If everything passes, then you should see output such as:  

- sam-VirtualBox64 - done with test on: Wed Jul 11 13:22:41 PDT 2012
- duration: 93 seconds
- 3 tests done.
- 3 passed.
- 0 failed.

If a failure occured, then look at the proper subdirectory in /tmp for 
all of the log files (this directory will be printed by testrunner.sh upon
failure).

DETAILS:

`testrunner.sh' scans the `Testsuite' directory for directories containing
CORE+Haggle tests. The 'TestTemplate' directory contains an example test
which has the proper shell scripts and format to work with 'testrunner.sh'.

testrunner.sh first checks the system environment. Upon passing this, the
testrunner proceeds to remove leftover 'pycore' files in the /tmp/ directory
and shutdown core in order to put the system environment in a clean state.  
Once the environment is clean, testrunner.sh iterates over each of the tests
(in numerical order, hence 001-* will run before 002-*), copies their core
service files (these configure each emulated node with the proper haggle and
network settings) to ~/.core/myservices, and startes core in batch mode 
(no GUI) on the included ".imn" file. 

The following lists the necessary files used by testrunner.sh and a brief 
description of their purpose.

TestTemplate/001-TestTemplate/:

The directory containing an example test case compatible with testrunner.sh

TestTemplate/001-TestTemplate/3node-TEST.imn:

This is a CORE topology created using the CORE GUI. The nodes should be labeled
n1, n2 ... in order to work properly with testrunner.sh.

TestTemplate/001-TestTemplate/config.cfg: 

The haggle xml configuration file that specifies what modules haggle should
load.

TestTemplate/001-TestTemplate/echo_duration.sh:

A shell script which echos the duration of the experiment in seconds. 
'testrunner.sh' uses this file to determine the duration of an experiment, and
kill experiments that exceed this duration. 
The shell script exits with return code 0 on success, and 1 on failure.

TestTemplate/001-TestTemplate/echo_output_path.sh:

A shell script which echos the test output file. Haggle applications may
output data there throughout the duration of the experiment. This file
is used by testrunner.sh to validate the success/failure of a test.
The shell script exits with return code 0 on success, and 1 on failure.

TestTemplate/001-TestTemplate/echo_test_appname.sh:

A shell script which echos the Haggle application path to run on the core nodes.
This application is typically a C app which connects to the haggle daemon.
The shell script exits with return code 0 on success, and 1 on failure.

TestTemplate/001-TestTemplate/echo_testscenario.sh:

A shell script which echos the path of the CORE .imn file. testrunner.sh uses
this path when starting core in batch mode.

TestTemplate/001-TestTemplate/validate.sh:

A shell script which reads the output file (usually specified by 
echo_output_path.sh) to validate whether the test passed or failed.
The shell script exits with return code 0 on (test) success, and 1 on (test)
failure.

TTestTemplate/001-TestTemplate/App/:

The directory containing the Haggle application that will run on each node. 
Typically the core service will start this application and pass in the node
number along with the output path. The application is responsible for
connecting to the haggle daemon, publishing and subscribing for content,
and outputing information used for test validation.

TestTemplate/001-TestTemplate/App/basic3node_test.sh:

The implementation of the application described above.

TestTemplate/001-TestTemplate/CoreService/:

This directory contains the python files that define the haggle service that
is launched on each node. The .imn CORE file should specify that each node
use the "Haggle" service. These files will be copied to ~/.core/myservices 
at the beginning of each test. Additionally, testrunner.sh will create some
shell scripts in ~/.core/myservices that the python code can call (via utils.py)
to get the test USER and path.

TestTemplate/001-TestTemplate/CoreService/haggle.py:

The CORE service file that sets the network settings, starts the haggle daemon,
and starts the haggle application. 

TestTemplate/001-TestTemplate/CoreService/__init__.py:

The initialization file that CORE uses to load the Haggle service defined in
haggle.py.

TestTemplate/001-TestTemplate/CoreService/utils.py:

Some utility function that haggle.py can use to get the test's USER and 
path from shell scripts that will be added to ~/.core/myservices at runtime 
(by testrunner.sh).
