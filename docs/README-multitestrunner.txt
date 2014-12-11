Relevant files & directories: 

    - ../studies/framework/lxc-runner/multitestrunner.sh : The automated testrunner that executes cases across branches 


===
MULTI TEST RUNNER
===

The script expects expects an environment variable called $HAGGLE_SRC_DIR to be pointing to cbmen-encoders/haggle.

Invoke with ./multitest.sh INPUTFILE LOG_DIR

The input file should be of the format

branch1 case1
branch2 case2
branch3 case3
