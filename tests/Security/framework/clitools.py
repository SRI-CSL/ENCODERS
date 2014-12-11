# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import sys
import time
import threading
import os
import termios 
import fcntl 
import struct

class _ProgressBar(threading.Thread):

    def __init__(self, ticksPerSec = 8):
        super(_ProgressBar, self).__init__()
        self.daemon = True
        self.running = False
        self.stop = False
        self.ticks = 0
        self.maxticks = 0
        self.ticksPerSec = ticksPerSec
        self.lock = threading.RLock()
        self.linefmt = '{message} ... [{dots}{bar}{spaces}] [ {left:<.2f} s] [ ETA {eta} ]'
        self.pbarlength = 0
        self.bars = '|/-\\'
        self.reportOnComplete = False
        self.eta = 0

    def run(self):
        while not self.stop:
            with self.lock:
                if self.running:
                    self.ticks = self.ticks + 1
                    self.eta = max(0.0, self.eta - 1.0/self.ticksPerSec)
                    self._printProgressBar()
                    # if self.ticks == self.maxticks:
                    #     self.running = False
                    #     self._onTaskComplete()
            time.sleep(1.0/self.ticksPerSec)

    def _setPBarLength(self):
        rows, columns = Console.getConsoleSize()
        self.pbarlength = columns - len(self.linefmt.format(message=self.msg, dots='', bar='', spaces='', left=-123.45, eta='00:00:00'))
        if self.pbarlength <= 1:
            if self.message.find(':') != -1:
                self.msg = self.message[:self.message.find(':')]
                self.pbarlength = columns - len(self.linefmt.format(message=self.msg, dots='', bar='', spaces='', left=-123.45, eta='00:00:00'))


    def _printProgressBar(self):
        self._setPBarLength()
        if self.ticks > self.maxticks:
            dots = self.pbarlength-1
            bar = self.bars[self.ticks%4]
            spaces = 0
        elif self.ticks == self.maxticks:
            dots = self.pbarlength
            spaces = 0
            bar = ''
        else:
            bar = self.bars[self.ticks % 4]
            spaces = int(((self.maxticks - self.ticks) / float(self.maxticks)) * self.pbarlength)
            dots = self.pbarlength - spaces - 1
        sys.stderr.write('\033[2K\r%s' % self.linefmt.format(message=self.msg, dots='.' * dots, bar=bar, spaces=' '*spaces,
                         left=float(self.maxticks - self.ticks)/self.ticksPerSec, eta=time.strftime('%H:%M:%S', time.gmtime(int(self.eta)))))

    def startTask(self, message, eta, reportOnComplete=False):
        with self.lock:
            self.maxticks = int(eta * self.ticksPerSec)
            self.ticks = 0
            self.message = message
            self.msg = message
            self.running = True
            self.reportOnComplete = reportOnComplete

    def endTask(self):
        with self.lock:
            self.ticks = self.maxticks 
            self._printProgressBar()
            self.running = False
            self._onTaskComplete()

    def _onTaskComplete(self):
        sys.stderr.write('\r\033[K')
        sys.stderr.flush()
        if self.reportOnComplete:
            Console.log('{message} ... Done!'.format(message=self.message))

    def stopProgress(self):
        with self.lock:
            self.stop = True
            self.ticks = self.maxticks
            self._printProgressBar()
            self._onTaskComplete()
            self.running = False

    def setETA(self, eta):
        self.eta = float(eta)

class Console:

    LOG = 4
    INFO = 3
    SUCCESS = 2
    WARNING = 1
    FAILURE = 0

    STDOUTLEVEL = INFO
    FILELEVEL = LOG
    FILE = None

    def __init__(self):
        pass

    @staticmethod
    def getConsoleSize():
        if not sys.stdout.isatty():
            return None

        s = struct.pack("HHHH", 0, 0, 0, 0)
        fd_stdout = sys.stdout.fileno()
        size = fcntl.ioctl(fd_stdout, termios.TIOCGWINSZ, s)
        height, width = struct.unpack("HHHH", size)[:2]
        return (height, width)

    @staticmethod
    def success(message):
        if Console.STDOUTLEVEL >= Console.SUCCESS:
            print '\033[32m[SUCCESS] %s\033[0m' % message
        if Console.FILE and Console.FILELEVEL >= Console.SUCCESS:
            Console.FILE.write('[SUCCESS] %s\n' % message)

    @staticmethod
    def log(message):
        if Console.STDOUTLEVEL >= Console.LOG:
            print '\033[30m[LOG] %s\033[0m' % message
        if Console.FILE and Console.FILELEVEL >= Console.LOG:
            Console.FILE.write('[LOG] %s\n' % message)

    @staticmethod
    def info(message):
        if Console.STDOUTLEVEL >= Console.INFO:
            print '\033[34m[INFO] %s\033[0m' % message
        if Console.FILE and Console.FILELEVEL >= Console.INFO:
            Console.FILE.write('[INFO] %s\n' % message)
    
    @staticmethod
    def warning(message):
        if Console.STDOUTLEVEL >= Console.WARNING:
            print '\033[33m[WARNING] %s\033[0m' % message
        if Console.FILE and Console.FILELEVEL >= Console.WARNING:
            Console.FILE.write('[WARNING] %s\n' % message)
    
    @staticmethod
    def fail(message):
        if Console.STDOUTLEVEL >= Console.FAILURE:
            print '\033[31m[FAILURE] %s\033[0m' % message
        if Console.FILE and Console.FILELEVEL >= Console.FAILURE:
            Console.FILE.write('[FAILURE] %s\n' % message)

ProgressBar = _ProgressBar()
    
def main():
    print 'About to start progress bar'
    ProgressBar.start()
    print 'Started progress bar'
    ProgressBar.startTask('This is a test', 10, True)
    ProgressBar.setETA(12)
    time.sleep(12)
    ProgressBar.stopProgress()
    ProgressBar.join()
    print 'Waiting for join'

    Console.success('This is a success!')
    Console.log('This is a log')
    Console.info('This is info')
    Console.warning('This is a warning')
    Console.fail('This is a failure')


if __name__ == '__main__':
    main()
