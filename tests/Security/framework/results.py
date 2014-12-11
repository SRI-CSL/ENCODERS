# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Results - Store test results

"""

import types
import inspect
from clitools import Console

class Assertion(object):

    def __init__(self, message, predicate, actual, critical, warningMessage):
        self.message = message
        self.predicate = predicate
        self.success = False
        self.actual = actual
        self.result = None
        self.critical = critical
        self.warningMessage = warningMessage
        self.warning = False

    def check(self):
        if type(self.predicate) == types.LambdaType:
            if self.predicate(self.actual):
                self.success = True
                self.result = 'Case "%s" succeeded!' % self.message
            else:
                self.success = not self.critical
                self.result = 'Case "%s" failed! %s does not satisfy %s' % (self.message, self.actual, inspect.getsource(self.predicate).strip()) 
                if not self.critical:
                    self.warning = True
                    self.result = '%s (%s)' % (self.result, self.warningMessage)
        elif self.predicate != self.actual:
            self.success = not self.critical
            self.result = 'Case "%s" failed! Expected %s, got %s' % (self.message, self.predicate, self.actual)
            if not self.critical:
                self.warning = True
                self.result = '%s (%s)' % (self.result, self.warningMessage)
        elif self.predicate == self.actual:
            self.success = True
            self.result = 'Case "%s" succeeded!' % self.message

    def explain(self):
        if not self.success:
            Console.fail(self.result)
        elif self.warning:
            Console.warning(self.result)

    def log(self):
        Console.log('    %s' % self.result)

class Result(object):

    def __init__(self, name):
        self.name = name
        self.assertions = []
        self.success = None

    def expect(self, message, expected, actual, critical=True, warningMessage=''):
        self.assertions.append(Assertion(message, expected, actual, critical, warningMessage))

    def complete(self):
        self.success = True
        
        if len(self.assertions) == 0:
            self.success = False
            self.assertions.append(Assertion("Number of assertions", 1, 0, True, ''))
            self.assertions[0].check()
        else:
            for assertion in self.assertions:
                assertion.check()
                self.success = self.success and assertion.success

    def summarize(self):
        if self.success:
            Console.success('%s test succeeded!' % self.name)
        else:
            Console.fail('%s test failed!' % self.name)

class Results(object):

    def __init__(self):
        self.results = []

    def add(self, result):
        self.results.append(result)

    def summarize(self):
        success = True
        failures = 0
        Console.info('-' * 50)
        Console.info('Result Summary:')
        for result in self.results:
            if result.success:
                Console.success('%s: Succeeded!' % result.name)
            else:
                Console.fail('%s: Failed!' % result.name)
                success = False
                failures = failures + 1

        Console.info('-' * 50)

        if success:
            if len(self.results) == 0:
                Console.fail('NO TESTS RUN')
            else:
                Console.success('ALL TESTS SUCCEEDED')
        else:
            Console.fail('%s/%s TESTS FAILED' % (failures, len(self.results)))

    def report(self):
        Console.log('-' * 50)
        Console.log('Detailed Result Report:')
        Console.log('-' * 50)
        for result in self.results:
            Console.log('-' * 50)
            if result.success:
                Console.log('Test %s: Succeeded!' % result.name)
            else:
                Console.log('Test %s: Failed!' % result.name)
            Console.log('-' * 50)
            for assertion in result.assertions:
                assertion.log()
            Console.log('-' * 50)
