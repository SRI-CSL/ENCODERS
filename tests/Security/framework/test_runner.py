# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Test Runner - Discover, Validate, and run Tests

"""

import os
import sys
import types
import inspect
import re
import glob
import time
import imp
import traceback
from clitools import ProgressBar, Console
from config import TestFrameworkConfig
from results import Result, Results

class Test(object):

    def __init__(self, directory, filename):
        self.directory = directory
        self.filename = filename
        self.name = None
        self.categories = ['all']
        self.valid = False
        self.validationError = None
        self.module = None
        self.function = None
        self.result = None
        self.eta = 0

    def validate(self, mockClass, baseConfig):
        path = os.path.join(self.directory, self.filename)
        if not os.path.exists(path):
            self.validationError = 'Nonexistent file %s' % path
            return

        self.name = self.filename.replace('.py', '')
        if self.name[:5] == 'test_':
            self.name = self.name[5:]

        try:
            self.module = imp.load_source(self.name, path)
        except Exception:
            self.validationError = 'Error importing source of test function %s: %s' % (self.filename, traceback.format_exc())
            return

        if not hasattr(self.module, 'runTest'):
            self.validationError = 'Test file %s has no function named runTest!' % self.filename
            return

        if hasattr(self.module, 'CATEGORIES'):
            categories = getattr(self.module, 'CATEGORIES')
            if type(categories) == types.ListType:
                if all(type(c) == types.StringType for c in categories):
                    self.categories.extend(categories)

        self.function = getattr(self.module, 'runTest')
        if not type(self.function) == types.FunctionType:
            self.validationError = '%s in file %s does not have a type of FunctionType!' % ('runTest', self.filename)
            return

        args = inspect.getargspec(self.function)
        if (args.varargs is not None or args.keywords is not None or args.defaults is not None or
            args.args != ['env', 'nodes', 'results', 'Console']):
            self.validationError = '%s arg spec in file %s should be exactly %s(env, nodes, results, Console):' % ('runTest', self.filename, 'runTest')
            return

        config = TestFrameworkConfig()
        config.update(baseConfig.toDict())
        config.update({'logDirectory': os.path.join(config.logDirectory, self.name)})
        config.validate(mockClass)

        env = mockClass(config)
        nodes = env.nodes
        env.setupEnvironment()
        result = env.mockResultObject
        try:
            self.function(env, nodes, result, Console)
        except Exception, ex:
            self.validationError = 'Error in test function: %s' % traceback.format_exc()
            return
        env.shutdownEnvironment()
        self.eta = env.eta

        self.valid = True

    def run(self, environmentClass, baseConfig):

        if not self.valid:
            Console.fail('Attempt to run invalid test at file %s!' % self.filename)
            sys.exit(1)

        Console.info('Running test: %s' % self.name)

        config = TestFrameworkConfig()
        config.update(baseConfig.toDict())
        config.update({'logDirectory': os.path.join(config.logDirectory, self.name)})
        config.validate(environmentClass)

        env = environmentClass(config)
        nodes = env.nodes
        env.setupEnvironment()

        self.result = Result(self.name)
        self.function(env, nodes, self.result, Console)

        env.shutdownEnvironment()

        self.result.complete()
        self.result.summarize()
        for assertion in self.result.assertions:
            assertion.explain()

        return self.result

class TestRunner(object):

    def __init__(self, mockClass, environmentClass, baseConfig):
        self.mockClass = mockClass
        self.environmentClass = environmentClass
        self.baseConfig = baseConfig
        self.results = Results()
        self.tests = []
        self.allTests = []

    def discover(self, directory):
        if not os.path.exists(directory) or not os.path.isdir(directory):
            Console.fail('Test directory %s does not exist!' % directory)
            sys.exit(1)

        Console.log('Discovering tests in %s' % os.path.abspath(directory))

        for filename in glob.glob(os.path.join(directory, 'test_*.py')):
            test = Test(directory, os.path.basename(filename))
            test.validate(self.mockClass, self.baseConfig)
            self.allTests.append(test)

    def getTestsQueuedInLastRun(self, log):
        return re.findall('^\[LOG\] Queueing test (\w*)', log, flags=re.MULTILINE)

    def getTestsFailedInLastRun(self, log):
        return re.findall('^\[FAILURE\] (\w*) test failed!', log, flags=re.MULTILINE)

    def addTestsByCategories(self, categories=None):
        if categories is not None:
            allCategories = {}
            for test in self.allTests:
                for category in test.categories:
                    if category in allCategories:
                        allCategories[category].add(test)
                    else:
                        allCategories[category] = set([test])
            split = re.split(' |\||&|-|\^|\(|\)', categories)
            for token in split:
                if len(token) > 0 and token not in allCategories:
                    Console.fail('Invalid token in categories expression: "%s"! Should only contain category names, brackets, or set operators.' % token)
                    sys.exit(1)
            for test in eval(categories, {}, allCategories):
                if test not in self.tests:
                    self.tests.append(test)

    def addTestsByName(self, testNames):
        names = []
        if len(testNames) > 0:
            for name in testNames:
                if name[:5] == 'test_':
                    names.append(name[5:])
                else:
                    names.append(name)
        for test in self.allTests:
            if test.name in names and test not in self.tests:
                self.tests.append(test)

    def queueTests(self, skip=0, queueAllIfEmpty=True):
        if len(self.tests) == 0 and queueAllIfEmpty:
            self.tests = self.allTests

        for test in self.tests:
            if not test.valid:
                Console.warning('Error loading test file %s: %s' % (test.filename, test.validationError))
        self.tests = sorted([test for test in self.tests if test.valid], key = lambda test: test.name)[skip:]

        for test in self.tests:
            Console.log('Queueing test %s - ETA %s' % (test.name, time.strftime('%H:%M:%S', time.gmtime(int(test.eta)))))

        eta = sum([test.eta for test in self.tests])
        Console.info('Queued %d test(s)! ETA %s.' % (len(self.tests), time.strftime('%H:%M:%S', time.gmtime(int(eta)))))
        ProgressBar.start()
        ProgressBar.setETA(eta)

    def run(self):
        start = time.time()
        for (i, test) in enumerate(self.tests):
            eta = sum([t.eta for t in self.tests[i:]])
            ProgressBar.setETA(eta)
            result = test.run(self.environmentClass, self.baseConfig)
            self.results.add(result)

        self.results.report()
        self.results.summarize()
        end = time.time()
        Console.info('Ran %d tests in %s' % (len(self.tests), time.strftime('%H:%M:%S', time.gmtime(int(end-start)))))
