# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

import sys
import re

def main():
  filename = sys.argv[1]
  nodes = {}
  published = {}
  received = {}
  currentNode = None
  fieldKeys = 'Action,ObjectId,CreateTime,ReqestTime,ArrivalTime,DelayRelToRequest,DelayRelToCreation,DataLen,tags'.split(',')

  with open(filename) as f:
    for line in f:
      result = re.search(r'(n\d+)\.\d+(\.pubs)?\.houtput', line)
      if result:
        currentNode = result.group(1)
        if not currentNode in nodes:
          nodes[currentNode] = {'Published': {}, 'Received': {}, 'NeverReceived': [], 'LocalOnly': []}
        continue

      result = re.search(r'(Received|Published),', line)
      if result:
        fields = dict(zip(fieldKeys, line.split(',')))
        fields['DataLen'] = int(fields['DataLen'])
        fields['Node'] = currentNode
        if fields['ObjectId'] in nodes[currentNode][fields['Action']]:
          print "DO %s %s multiple times" % (fields['ObjectId'], fields['Action'])
        nodes[currentNode][fields['Action']][fields['ObjectId']] = fields

        if fields['Action'] == 'Received':
          if fields['ObjectId'] not in received:
            received[fields['ObjectId']] = []
          received[fields['ObjectId']].append(fields)
        else:
          published[fields['ObjectId']] = fields


  receivedCount = 0
  localOnlyCount = 0
  nonReceivedCount = 0

  for pub, do in published.iteritems():
    if pub not in received and do['Node'] != 'n8':
      print "DO %s (size %s from node %s) published but never received" % (pub, do['DataLen'], do['Node'])
      nonReceivedCount += 1
      nodes[do['Node']]['NeverReceived'].append(do)
    else:
      wasReceived = False
      for node in nodes:
        if node != do['Node'] and do['ObjectId'] in nodes[node]['Received']:
          wasReceived = True
          break
      if wasReceived:
        receivedCount += 1
      else:
        localOnlyCount += 1
        nodes[do['Node']]['LocalOnly'].append(do)

  #latency:
  #tracks from fighter->fighter
  calculateLatency(nodes, published, received, 'tracks fighter->fighter', ['n1', 'n2', 'n3', 'n4'], ['n1', 'n2', 'n3', 'n4'], 1024)
  #tracks from fighter->awacs
  calculateLatency(nodes, published, received, 'tracks fighter->awacs', ['n1', 'n2', 'n3', 'n4'], ['n5', 'n6'], 1024)
  #tracks from ground->awacs
  calculateLatency(nodes, published, received, 'tracks ground->awacs', ['n8'], ['n5', 'n6'], 1024)
  #tracks from awacs->fighter
  calculateLatency(nodes, published, received, 'tracks awacs->fighter', ['n5', 'n6'], ['n1', 'n2', 'n3', 'n4'], 10240)

  #radar from fighter->awacs
  calculateLatency(nodes, published, received, 'radar fighter->awacs', ['n1', 'n2', 'n3', 'n4'], ['n5', 'n6'], 128000)
  #radar from fighter->ship
  calculateLatency(nodes, published, received, 'radar fighter->ship', ['n1', 'n2', 'n3', 'n4'], ['n7'], 128000)
  #radar from fighter->ground
  calculateLatency(nodes, published, received, 'radar fighter->ground', ['n1', 'n2', 'n3', 'n4'], ['n8'], 128000)

  #video from fighter->awacs
  calculateLatency(nodes, published, received, 'video fighter->awacs', ['n1', 'n2', 'n3', 'n4'], ['n5', 'n6'], 983040)
  #video from fighter->ship
  calculateLatency(nodes, published, received, 'video fighter->ship', ['n1', 'n2', 'n3', 'n4'], ['n7'], 983040)
  #video from fighter->ground
  calculateLatency(nodes, published, received, 'video fighter->ground', ['n1', 'n2', 'n3', 'n4'], ['n8'], 983040)

  # delivery rate:
  # fighter tracks
  # ground tracks
  # awacs tracks
  # radar
  # video


  print "Out of %d DOs published, %d were received by another node, %d were only received locally, and %d were never received" % (len(published), receivedCount, localOnlyCount, nonReceivedCount)
  for node in nodes:
    print "%s: %d objects published, %d never received, %d only locally received" % (node, len(nodes[node]['Published']), len(nodes[node]['NeverReceived']), len(nodes[node]['LocalOnly']))

def calculateLatency(nodes, published, received, title, senders, receivers, filesize):
  dataObjectsSent = []
  receiveTimes = []
  selfReceives = 0

  for n in senders:
    dataObjectsSent.extend([item for item in nodes[n]['Published'].values() if item['DataLen'] == filesize])

  for obj in dataObjectsSent:
    if not (obj['ObjectId'] in received):
      continue

    for receipt in received[obj['ObjectId']]:
      if receipt['Node'] != obj['Node'] and receipt['Node'] in receivers:
        receiveTimes.append(float(receipt['DelayRelToCreation']))
      else:
        selfReceives += 1

  if len(receiveTimes) == 0:
    print "Average latency for %s is undetermined (never received)" % (title)
  else:
    print "Average Latency for %s is %f (%d values)" % (title, sum(receiveTimes)/len(receiveTimes), len(receiveTimes))

def calculateDissCurve(published, received, filesize, maxRecipients):
  buckets = {}
  for t in range(0, 1200, 5):
    buckets[t] = []


  for do in [item for item in published.values() if item['DataLen'] == filesize]:
    receipts = received[do['ObjectId']]
    return 1




if __name__ == '__main__':
  main()
