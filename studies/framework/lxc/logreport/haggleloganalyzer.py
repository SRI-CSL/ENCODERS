# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import sys
import re
import glob
from subprocess import check_output

class HaggleLogAnalyzer:
  testRoot = None
  grepList=[]
  count=[]
  def __init__(self, testRoot, logFilterPath, grepListPath):
    self.testRoot = testRoot
    self.parseGrepList(grepListPath)
    filteredLog = self.filterHaggleLog(logFilterPath)
    self.countTokenNum(filteredLog)

  def toString(self):
    text=""
    for i in range(0, len(self.grepList)):
      line=""
      for t in self.grepList[i]:
        if line != "":
          line += " & "
        line += t
      line += " : " + str(self.count[i])
      text += line+"\n"
    return text

  def countTokenNum(self, text):
    for i in range(0, len(self.grepList)):
      self.count.append(0)

    for line in text.split('\n'):
      index=0
      for tokens in self.grepList:
        c=0
        for t in tokens:
          if t in line:
            c += 1
        if c == len(tokens):
          self.count[index] += 1
        index += 1

  def filterHaggleLog(self, logFilterPath):
    logs = glob.glob('%s/*.haggle.log' % self.testRoot)
    cmd = 'egrep -f %s %s' % (logFilterPath, " ".join(logs))
    return check_output(cmd, shell=True)

  def parseGrepList(self, grepListPath):
    f = open(grepListPath, "rb")
    tokenp = re.compile(r'"([^"]+)"|([\d\w\-\_]+)')
    for line in f:
      tokenpairs = tokenp.findall(line)
      tokens = []
      for tokenpair in tokenpairs:
        if isinstance(tokenpair,str):
          tokens.append(tokenpair)
        else:
          for t in tokenpair:
            if isinstance(t,str) and t != '':
              tokens.append(t)
      self.grepList.append(tokens)
