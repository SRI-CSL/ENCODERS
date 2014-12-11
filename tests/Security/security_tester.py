#!/usr/bin/python

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Security Tester

Usage:
  security_tester.py [options] [<tests>...]

Options:
  -c <config>, --config <config>    Read config from the given string, or from a file if <config> is a file.
  -l <file>, --log <file>       Log to <log_file> [default: security_tester.log]
  --slevel <level>              Log level to use for stdout (e.g. LOG, INFO, SUCCESS, WARNING, FAILURE) [default: INFO]
  --flevel <level>              Log level to use for log file (e.g. LOG, INFO, SUCCESS, WARNING, FAILURE) [default: LOG]
  --rerun                       Rerun the same tests from the previous run
  --rerunfailed                 Rerun only the failed tests from the previous run
  --categories <expr>           Run all tests that belong in any categories satisfying the expression. [default: all-microbenchmark]
  -e <env>, --environment <env>       Run tests in the given environment (e.g core or android) [default: core]
  -d <dir>, --directory <dir>   Load tests from the given directory [default: tests]
  -s <n>, --skip <n>            Skip the first n tests [default: 0]
  -h --help                     Show this screen.
"""

import os
import sys
import json
import pwd
from docopt import docopt
from framework.clitools import Console
from framework.config import TestFrameworkConfig
from framework.core_test_environment import CoreTestEnvironment
from framework.android_test_environment import AndroidTestEnvironment
from framework.mock_test_environment import MockAndroidTestEnvironment, MockCoreTestEnvironment
from framework.test_runner import TestRunner

def main(arguments):
    if arguments['--help']:
        return

    for (option, var) in [('--flevel', 'FILELEVEL'), ('--slevel', 'STDOUTLEVEL')]:
        if arguments[option]:
            if arguments[option] not in ['LOG', 'INFO', 'WARNING', 'SUCCESS', 'FAILURE']:
                Console.warning('Invalid value for %s! should be one of LOG, INFO, SUCCESS, WARNING, or FAILURE' % option)
            else:
                setattr(Console, var, getattr(Console, arguments[option]))

    oldLogFile = ""
    try:
        with open(arguments['--log']) as input:
            oldLogFile = input.read()
    except Exception, ex:
        Console.info("Couldn't open log file %s to get previously queued tests!" % arguments['--log'])

    try:
        out = open(arguments['--log'], 'w')
        os.chown(arguments['--log'], pwd.getpwnam(os.getlogin()).pw_uid, -1)
        Console.FILE = out
    except Exception, ex:
        Console.warning('Could not open log file for writing!')

    if arguments['--environment'] == 'android':
        if os.geteuid() == 0:
            Console.fail('Script should not be run as root for Android tests!')
            return
        mockClass = MockAndroidTestEnvironment
        environmentClass = AndroidTestEnvironment
        baseConfig = TestFrameworkConfig()
        baseConfig.autoDiscoverAndroid()
    elif arguments['--environment'] == 'core':
        if os.geteuid() != 0:
            Console.fail('Script must be run as root for CORE tests!')
            return
        mockClass = MockCoreTestEnvironment
        environmentClass = CoreTestEnvironment
        baseConfig = TestFrameworkConfig()
        baseConfig.autoDiscoverCore()
    else:
        Console.fail('Invalid environment "%s"! Environment must be either "core" or "android"' % arguments['--environment'])
        sys.exit(1)

    if arguments['--config'] is not None:
        config = arguments['--config']
        if config.find('{') != -1:
            Console.info('Loading config from command line argument dict')
            try:
                dic = json.loads(config)
            except Exception:
                Console.fail('Error parsing JSON in config %s!' % config)
                sys.exit(1)
            baseConfig.update(dic, True)
        else:
            if not os.path.exists(config):
                Console.fail('Config file %s does not exist!' % config)
                sys.exit(1)

            dic = {}
            with open(config) as input:
                try:
                    dic = json.load(input)
                except Exception:
                    Console.fail('Error parsing JSON in file %s!' % config)
                    sys.exit(1)

            Console.info('Loading config from %s' % config)
            baseConfig.update(dic, True)

    runner = TestRunner(mockClass, environmentClass, baseConfig)

    testNames = arguments['<tests>']
    queueAllIfEmpty = len(testNames) == 0
    if arguments['--rerun']:
        testNames.extend(runner.getTestsQueuedInLastRun(oldLogFile))
        queueAllIfEmpty = False

    if arguments['--rerunfailed']:
        testNames.extend(runner.getTestsFailedInLastRun(oldLogFile))
        queueAllIfEmpty = False

    skip = 0
    try:
        skip = int(arguments['--skip'])
    except Exception, ex:
        Console.fail('<n> must be an integer with --skip')
        sys.exit(1)
    
    runner.discover(arguments['--directory'])
    if queueAllIfEmpty:
        runner.addTestsByCategories(arguments['--categories'])
    runner.addTestsByName(testNames)
    runner.queueTests(skip, queueAllIfEmpty)
    runner.run()

if __name__ == '__main__':
    arguments = docopt(__doc__)
    main(arguments)
