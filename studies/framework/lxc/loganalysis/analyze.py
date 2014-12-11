# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

import sys
import re
import copy
import sqlite3
import os
import glob
import tarfile
import shutil
import tempfile
import binascii
import json
import docopt
import time
import collections
import multiprocessing      

try:
  import lxml.etree as ET
except ImportError:
  print "Warning! Unable to load lxml, falling back to slower xml parser"
  import xml.etree.ElementTree as ET

DO_FIELD_KEYS = 'Action,ObjectId,CreateTime,ReqestTime,ArrivalTime,DelayRelToRequest,DelayRelToCreation,DataLen,tags'.split(',')

class DataObject:

  def __init__(self, publisher, delay, attributes, datafile):

    self.id = None
    self.publisher = publisher

    self.delay = float(delay)
    self.datafile = datafile
    self.attributes = {}
    self.cmd = attributes

    for attribute in attributes.split(';'):
      k,v = attribute.split('=')
      self.attributes[k] = v

    self.action = None
    self.createTime = None
    self.requestTime = None
    self.arrivalTime = None
    self.delayRelToRequest = None
    self.delayRelToCreation = None
    self.dataLen = None
    self.tags = None

    self.doId = None

  def update_fields(self, dic):

    self.action = dic['Action']
    self.doId = dic['ObjectId']
    self.createTime = dic['CreateTime']
    self.requestTime = dic['ReqestTime']
    self.arrivalTime = dic['ArrivalTime']
    self.delayRelToRequest = float(dic['DelayRelToRequest'])
    self.delayRelToCreation = float(dic['DelayRelToCreation'])
    self.dataLen = int(dic['DataLen'])
    self.tags = dic['tags']

  def __str__(self):
    return "DO : file %s published by %s - %s" % (self.datafile, self.publisher.name, self.cmd)

  def __repr__(self):
    return "DO : file %s published by %s - %s" % (self.datafile, self.publisher.name, self.cmd)

class Node:

  def __init__(self, name):

    self.name = name
    self.id = None
    self.nodeId = None
    self.published = []
    self.received = {}
    self.inserted = False

def create_db(dbPath):

  conn = sqlite3.connect(dbPath)
  c = conn.cursor()

  c.execute("""PRAGMA journal_mode = off""")
  c.execute("""PRAGMA synchronous = off""")

  c.execute("""CREATE TABLE nodes (
              id integer primary key autoincrement,
              name text,
              nodeId text,
              startupTime real
            )""")
  c.execute("""CREATE TABLE bandwidth (
              nodeId integer primary key,
              rx_bytes integer not null,
              tx_bytes integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id)
            )""")
  c.execute("""CREATE TABLE dos (
              id integer primary key autoincrement,
              doId text not null
            )""")
  c.execute("""CREATE TABLE pubs (
              id integer primary key,
              delay real not null,
              datafile text not null,
              cmd text not null,
              tags text not null,
              createTime real not null,
              dataLen integer not null,
              publisher integer not null,
              FOREIGN KEY (id) REFERENCES dos (id),
              FOREIGN KEY (publisher) REFERENCES nodes(id)
            )""")
  c.execute("""CREATE TABLE subs (
              id integer not null,
              createTime real not null,
              requestTime real not null,
              arrivalTime real not null,
              delayRelToRequest real not null,
              delayRelToCreation real not null,
              dataLen integer not null,
              tags text not null,
              publisher integer not null,
              subscriber integer not null,
              FOREIGN KEY (id) REFERENCES dos(id),
              FOREIGN KEY (publisher) REFERENCES nodes(id),
              FOREIGN KEY (subscriber) REFERENCES nodes(id),
              PRIMARY KEY(id, publisher, subscriber)
            )""")
  c.execute("""CREATE TABLE attrs (
              id integer primary key autoincrement,
              name text not null,
              value text not null,
              UNIQUE (name, value)
            )""")
  c.execute("""CREATE UNIQUE INDEX attrsnamevalue ON attrs(name, value)""")
  c.execute("""CREATE TABLE doAttrs (
              doId integer not null,
              attrId integer not null,
              FOREIGN KEY (doId) REFERENCES dos(id),
              FOREIGN KEY (attrId) REFERENCES attrs(id),
              PRIMARY KEY (doId, attrId)
            )""")
  c.execute("""CREATE VIEW attributes AS
              SELECT doId AS id, a.name, a.value FROM 
              doAttrs JOIN attrs AS a ON attrId = a.id""")

  c.execute("""CREATE TABLE squads (
              squadName text not null,
              nodeId integer not null,
              nodeName text not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (nodeName) REFERENCES nodes(name)
            )""")
  c.execute("""CREATE TABLE dosInHaggleDB (
              doId integer not null,
              nodeId integer not null,
              FOREIGN KEY (doId) REFERENCES dos(id),
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY (doId, nodeId)
            )""")

  c.execute("""CREATE TABLE events (
              id integer primary key autoincrement,
              name text not null
            )""")

  c.execute("""CREATE TABLE interfaces (
              id integer primary key autoincrement,
              nodeId integer not null,
              identifier text not null,
              type text not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              UNIQUE (nodeId, identifier)
            )""")
  c.execute("""CREATE TABLE addresses (
              id integer primary key autoincrement,
              uri text not null
            )""")
  c.execute("""CREATE TABLE interfaceAddresses (
              ifId integer not null,
              addrId integer not null,
              FOREIGN KEY (ifId) REFERENCES interfaces(id),
              FOREIGN KEY (addrId) REFERENCES addresses(id),
              PRIMARY KEY (ifId, addrId)
            )""")
  c.execute("""CREATE TABLE virtualNodes (
              id integer primary key autoincrement,
              name text not null,
              nodeId text not null,
              proxyId text,
              local_application boolean not null,
              type text not null,
              UNIQUE (name, nodeId, proxyId, type)
            )""")

  c.execute("""CREATE TABLE observeCPUUsage (
              nodeId integer not null,
              entry integer not null,
              block_input_ops integer not null,
              block_output_ops integer not null,
              soft_page_faults integer not null,
              hard_page_faults integer not null,
              voluntary_context_switches integer not null,
              involuntary_context_switches integer not null,
              max_rss integer not null,
              system_cpu_time real not null,
              user_cpu_time real not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY(nodeId, entry)
            )""")
  c.execute("""CREATE TABLE observeMemoryUsage (
              nodeId integer not null,
              entry integer not null,
              allocated integer not null,
              available_in_fast_bins integer not null,
              fast_bin_blocks integer not null,
              free integer not null,
              free_chunks integer not null,
              malloc_total integer not null,
              mmaped integer not null,
              not_mmaped integer not null,
              releasable integer not null,
              sqlite integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY (nodeId, entry)
            )""")
  c.execute("""CREATE TABLE observeBandwidthUsage (
              nodeId integer not null,
              entry integer not null,
              interface text not null,
              rx_bytes integer not null,
              rx_packets integer not null,
              tx_bytes integer not null,
              tx_packets integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY (nodeId, entry)
            )""")
  c.execute("""CREATE TABLE observeKernelUsage (
              nodeId integer not null,
              entry integer not null,
              event_queue_size integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY (nodeId, entry)
            )""")
  c.execute("""CREATE TABLE observeEventQueueEvents (
              nodeId integer not null,
              entry integer not null,
              eId integer not null,
              etime real not null,
              dobj text,
              iface text,
              node text,
              nodeListSize integer,
              dataObjectListSize integer,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (eId) REFERENCES events(id),
              PRIMARY KEY (nodeId, entry, eId, etime, dobj, iface, node)
            )""")
  c.execute("""CREATE TABLE observeEventQueueCounts (
              nodeId integer not null,
              entry integer not null,
              eId integer not null,
              count integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (eId) REFERENCES events(id),
              PRIMARY KEY (nodeId, entry, eId)
            )""")
  c.execute("""CREATE TABLE observeCertificates (
              nodeId integer not null,
              entry integer not null,
              type text not null,
              issuer integer not null,
              subject integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (issuer) REFERENCES nodes(id),
              FOREIGN KEY (subject) REFERENCES nodes(id),
              PRIMARY KEY (nodeId, entry, type, issuer, subject)
            )""")
  c.execute("""CREATE TABLE observeInterfaces (
              nodeId integer not null,
              entry integer not null,
              ifId integer not null,
              age real not null,
              flags text not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (ifId) REFERENCES interfaces(id),
              PRIMARY KEY (nodeId, entry, ifId)
            )""")
  c.execute("""CREATE TABLE observeNodeDescriptionAttributes (
              nodeId integer not null,
              entry integer not null,
              aId integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (aId) REFERENCES attrs(id),
              PRIMARY KEY (nodeId, entry, aId)
            )""")
  c.execute("""CREATE TABLE observeNodeStoreNodes (
              nodeId integer not null,
              entry integer not null,
              vNodeId integer not null,
              confirmed boolean not null,
              num_objects_in_bloom_filter integer not null,
              stored boolean not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (vNodeId) REFERENCES virtualNodes(id),
              PRIMARY KEY (nodeId, entry, vNodeId)
            )""")
  c.execute("""CREATE TABLE observeNodeStoreAttributes (
              nodeId integer not null,
              entry integer not null,
              vNodeId integer not null,
              aId integer not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (vNodeId) REFERENCES virtualNodes(id),
              FOREIGN KEY (aId) REFERENCES attrs(id),
              PRIMARY KEY (nodeId, entry, vNodeId, aId)
            )""")
  c.execute("""CREATE TABLE observeNodeStoreInterfaces (
              nodeId integer not null,
              entry integer not null,
              vNodeId integer not null,
              iId integer not null,
              is_up boolean not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              FOREIGN KEY (vNodeId) REFERENCES virtualNodes(id),
              FOREIGN KEY (iId) REFERENCES interfaces(id),
              PRIMARY KEY (nodeId, entry, vNodeId, iId)
            )""")
  c.execute("""CREATE TABLE observeCacheStrategy (
              nodeId integer not null,
              entry integer not null,
              allow_db_purging boolean not null,
              bloomfilter_remove_delay_ms integer not null,
              current_capacity_ratio real not null,
              current_drop_on_insert integer not null,
              current_dupe_do_recv integer not null,
              current_num_do integer not null,
              current_size_kb integer not null,
              db_size_threshold integer not null,
              event_based_purging boolean not null,
              global_optimizer text not null,
              handle_zero_size boolean not null,
              keep_in_bloomfilter boolean not null,
              knapsack_optimizer text not null,
              manage_locally_sent_files boolean not null,
              manage_only_remote_files boolean not null,
              max_capacity_kb integer not null,
              poll_period_ms integer not null,
              total_db_evicted integer not null,
              total_do_evicted integer not null,
              total_do_evicted_bytes integer not null,
              total_do_hard_evicted integer not null,
              total_do_hard_evicted_bytes integer not null,
              total_do_inserted integer not null,
              total_do_inserted_bytes integer not null,
              utility_compute_period_ms integer not null,
              utility_function text not null,
              watermark_capacity_kb integer not null,
              watermark_capacity_ratio real not null,
              FOREIGN KEY (nodeId) REFERENCES nodes(id),
              PRIMARY KEY (nodeId, entry)
            )""")

  conn.commit()
  return conn

def create_db_indices(db):

  c = db.cursor()
  unique_indices = [
    ('nodes', 'name'),
    ('nodes', 'nodeId'),
    ('dos', 'doId'),
    ('attrs', 'id'),
    ('addresses', 'uri'),
    ('events', 'name'),
  ]

  for (table, column) in unique_indices:
    c.execute("""CREATE UNIQUE INDEX %s%s ON %s (%s);""" % (table, column, table, column))

  indices = [
    ('squads', 'squadName'),
    ('squads', 'nodeName'),
    ('squads', 'nodeId'),
    ('pubs', 'publisher'),
    ('subs', 'id'),
    ('subs', 'subscriber'),
    ('doAttrs', 'attrId'),
    ('doAttrs', 'doId'),
    ('interfaces', 'identifier'),
    ('virtualNodes', 'name'),
    ('virtualNodes', 'nodeId'),
    ('observeCPUUsage', 'nodeId'),
    ('observeMemoryUsage', 'nodeId'),
    ('observeBandwidthUsage', 'nodeId'),
    ('observeKernelUsage', 'nodeId'),
    ('observeCertificates', 'nodeId'),
    ('observeNodeDescriptionAttributes', 'nodeId'),
    ('observeNodeDescriptionAttributes', 'aId'),
    ('observeNodeStoreNodes', 'nodeId'),
    ('observeNodeStoreAttributes', 'nodeId'),
    ('observeNodeStoreInterfaces', 'nodeId')
  ]

  for (table, column) in indices:
    c.execute("""CREATE INDEX %s%s ON %s (%s);""" % (table, column, table, column))

def load_db(dbPath):

  conn = sqlite3.connect(dbPath)
  return conn

def parse_app_sh_file(input):

  newNodeRegex = re.compile(r"""elif\ \[ \"\${NODE}\" == \"(n[0-9]*)\"\ ];\ then""")
  pubRegex = re.compile(r"""    echo \"([0-9]*\.[0-9]*),pub,\\\"([a-zA-Z0-9=;_\$\{\}\.-]*)\\\",\\\"([0-9a-zA-z/_\.-]*)\\\"\" >> \${TMPPUB\}""")
  nodes = {}
  curNodeName = ""
  curNode = None
  for line in input.readlines():
    match = newNodeRegex.match(line)
    if match is not None:
      newName = match.groups(0)[0]
      if curNode is not None:
        nodes[curNodeName] = curNode
      curNodeName = newName
      curNode = Node(curNodeName)
      continue
    match = pubRegex.match(line)
    if match is not None:
      delay, attributes, datafile = match.groups()
      do = DataObject(curNode, delay, attributes.replace("${NODE}", curNodeName), datafile)
      curNode.published.append(do)
  if curNode is not None:
    nodes[curNodeName] = curNode
  return nodes

def parse_apps_output_file(nodes, db, input):

  c = db.cursor()
  newNodeRegex = re.compile(r"""(n\d+)\.\d+(\.pubs)?\.houtput""")
  resultRegex = re.compile(r"""(Received|Published),""")
  bandwidthRegex = re.compile(r"""\s+RX bytes:(\d*) \(\d*\.\d* \w*\)  TX bytes:(\d*) \(\d*\.\d* \w*\)""")
  curNodeName = ""
  curNode = None
  pubIdx = 0
  doIdToPublisher = {}
  doIdToDo = {}
  for line in input.readlines():
    result = newNodeRegex.search(line)
    if result is not None:
      curNodeName = result.group(1)
      curNode = nodes[curNodeName]
      pubIdx = 0
      if not curNode.inserted:
        c.execute("""INSERT INTO nodes (name) VALUES (?)""", (curNodeName,))
        curNode.inserted = True
        nid = c.execute("""SELECT id FROM nodes WHERE name = ?""", (curNodeName,)).fetchone()[0]
        curNode.id = nid
      continue

    result = resultRegex.search(line)
    if result is not None:
      fields = dict(zip(DO_FIELD_KEYS, line.split(',')))

      do = None
      if fields['Action'] == 'Published':
        do = curNode.published[pubIdx]
        pubIdx = pubIdx + 1
        do.update_fields(fields)
        doIdToPublisher[do.doId] = curNode
        doIdToDo[do.doId] = do
        c.execute("""REPLACE INTO dos (doId) VALUES (?)""", (do.doId,))
        do.id = c.execute("""SELECT id FROM dos WHERE doId = ?""", (do.doId,)).fetchone()[0]
        c.execute("""INSERT INTO pubs(id, delay, datafile, cmd, tags, createTime, dataLen, publisher) SELECT ?,?,?,?,?,?,?,n.id FROM nodes AS n WHERE n.name = ?""", (do.id, do.delay, do.datafile, do.cmd, do.tags, do.createTime, do.dataLen, do.publisher.name))
      elif fields['Action'] == 'Received':
          curNode.received[fields['ObjectId']] = fields

    result = bandwidthRegex.search(line)
    if result is not None:
      rx = result.group(1)
      tx = result.group(2)
      c.execute("""INSERT INTO bandwidth(nodeId, rx_bytes, tx_bytes) VALUES (?,?,?)""", (nid, rx, tx))

  for node in nodes.values():
    for (id, fields) in node.received.copy().items():
      if id in doIdToDo:
        do = copy.copy(doIdToDo[id])
        do.update_fields(fields)
        node.received[id] = do
        c.execute("""INSERT INTO subs(id, createTime, requestTime, arrivalTime, delayRelToRequest, delayRelToCreation, dataLen, tags, publisher, subscriber) SELECT ?,?,?,?,?,?,?,?,p.id,s.id FROM nodes AS p JOIN nodes AS s WHERE p.name = ? AND s.name = ?""", (do.id, do.createTime, do.requestTime, do.arrivalTime, do.delayRelToRequest, do.delayRelToCreation, do.dataLen, do.tags, do.publisher.name, node.name))
      else:
        pass
        # print 'DO %s received but not published?' % fields['ObjectId']
        # c.execute("""INSERT INTO subs VALUES (?,?,?,?,?,?,?,?,?)""", (fields['ObjectId'], fields['CreateTime'], fields['ReqestTime'], fields['ArrivalTime'], fields['DelayRelToRequest'], fields['DelayRelToCreation'], fields['DataLen'], fields['tags'], node.name))

  uniques = set()
  attrss = []
  for do in doIdToDo.values():
    attrs = [(do.id, k, v) for (k,v) in do.attributes.items()]
    uniques |= set(do.attributes.items())
    attrss.extend(attrs)
  c.executemany("""INSERT INTO attrs (name, value) VALUES (?,?)""", list(uniques))
  c.executemany("""INSERT INTO doAttrs (doId, attrId) SELECT ?, a.id FROM attrs AS a WHERE a.name = ? AND a.value = ?""", attrss)

  db.commit()

def parse_haggle_log_file(node, db, input):

  startupTimeRegex = re.compile(r"""[0-9]*\.[0-9]*:\[[0-9]*-[0-9]*\]{HaggleKernel::init}: Startup Time is ([0-9]*\.[0-9]*)""")
  nodeIdRegex = re.compile(r"""[0-9]*\.[0-9]*:\[[0-9]*-[0-9]*\]{Node::calcId}: Node type=[0-9]*, id=\[([a-z0-9]*)\]""")
  eventRegex = re.compile(r"""([0-9]*\.[0-9]*):\[[0-9]*-[0-9]*\]{HaggleKernel::run}: Event ([A-Za-z0-9 _]*) at ([0-9]*.[0-9]*)""")

  c = db.cursor()
  entry = -1

  for line in input.readlines():
    result = startupTimeRegex.search(line)
    if result is not None:
      startupTime = result.group(1)
      c.execute("""UPDATE nodes SET startupTime = ? WHERE name = ?""", (startupTime, node))
      continue
    result = nodeIdRegex.search(line)
    if result is not None:
      nodeId = result.group(1)
      c.execute("""UPDATE nodes SET nodeId = ? WHERE name = ?""", (nodeId, node))
      continue
    result = eventRegex.search(line)
    if result is not None:
      logTime = result.group(1)
      eventName = result.group(2)
      eventTime = result.group(3)
      nodeId = c.execute("""SELECT id FROM nodes WHERE name = ?""", (node,)).fetchone()[0]
      c.execute("""INSERT INTO observeEventQueueEvents SELECT ?,?,e.id,?,?,?,?,?,? FROM events AS e WHERE e.name = ?""", (nodeId, entry, eventTime, None, None, None, None, None, eventName))

  db.commit()

def unhandled_parser(node, c, timestamp, root, entries):
  pass

def parse_observe_cpu_usage(node, c, timestamp, root, entries):
  attributes = ['block_input_ops', 'block_output_ops', 'soft_page_faults', 'hard_page_faults', 'voluntary_context_switches', 'involuntary_context_switches', 'max_rss', 'system_cpu_time', 'user_cpu_time']
  tup = tuple([node, entries] + [root.get(a, None) for a in attributes])
  c.execute("""INSERT INTO observeCPUUsage VALUES (?,?,?,?,?,?,?,?,?,?,?)""", tup)

def parse_observe_memory_usage(node, c, timestamp, root, entries):
  attributes = ['allocated', 'available_in_fast_bins', 'fast_bin_blocks', 'free', 'free_chunks', 'malloc_total', 'mmaped', 'not_mmaped', 'releasable', 'sqlite']
  tup = tuple([node, entries] + [root.get(a, None) for a in attributes])
  c.execute("""INSERT INTO observeMemoryUsage VALUES (?,?,?,?,?,?,?,?,?,?,?,?)""", tup)

def parse_observe_bandwidth_usage(node, c, timestamp, root, entries):
  attributes = ['name', 'rx_bytes', 'rx_packets', 'tx_bytes', 'tx_packets']
  for interface in root.findall('Interface'):
    tup = tuple([node, entries] + [interface.get(a, None) for a in attributes])
    c.execute("""INSERT INTO observeBandwidthUsage VALUES (?,?,?,?,?,?,?)""", tup)

def parse_observe_kernel_usage(node, c, timestamp, root, entries):
  attributes = ['event_queue_size']
  tup = tuple([node, entries] + [root.get(a, None) for a in attributes])
  c.execute("""INSERT INTO observeKernelUsage VALUES (?,?,?)""", tup)
  for eq in root.findall('./*'):
    for counts in eq.findall('Counts'):
      for count in counts.findall('./*'):
        c.execute("""INSERT OR IGNORE INTO events (name) VALUES (?)""", (count.get('name', None),))
        attributes = ['count', 'name']
        tup = tuple([node, entries] + [count.get(a, None) for a in attributes])
        c.execute("""INSERT INTO observeEventQueueCounts SELECT ?,?,e.id,? FROM events AS e WHERE e.name = ?""", tup)
    for events in eq.findall('Events'):
      for event in events.findall('./*'):
        c.execute("""INSERT OR IGNORE INTO events (name) VALUES (?)""", (event.get('name', None),))
        attributes = ['time', 'dObj', 'iface', 'node', 'nodeListSize', 'dataObjectListSize', 'name']
        tup = tuple([node, entries] + [event.get(a, None) for a in attributes])
        c.execute("""INSERT INTO observeEventQueueEvents SELECT ?,?,e.id,?,?,?,?,?,? FROM events AS e WHERE e.name = ?""", tup)

def parse_observe_certificates(node, c, timestamp, root, entries):
  attributes = ['issuer', 'subject']
  for sub in root.findall('./*'):
    type = sub.tag
    for certificate in sub.findall('Certificate'):
      tup = tuple([node, entries, type] + [certificate.get(a, None) for a in attributes])
      c.execute("""INSERT INTO observeCertificates (nodeId, entry, type, issuer, subject) SELECT ?,?,?, n1.id, n2.id FROM nodes AS n1 CROSS JOIN nodes AS n2 WHERE n1.nodeId = ? AND n2.nodeId = ?""", tup)

def parse_observe_node_metrics(node, c, timestamp, root, entries):
  handlers = {
    'CPU' : parse_observe_cpu_usage,
    'Memory' : parse_observe_memory_usage,
    'Bandwidth' : parse_observe_bandwidth_usage,
    'Kernel': parse_observe_kernel_usage
  }
  observed = [o for o in root.findall('./*') if o.tag in handlers]
  for o in observed:
    handlers[o.tag](node, c, timestamp, o, entries)

def parse_interface(node, c, iface):
  attributes = ['identifier', 'type']
  tup = tuple([node] + [iface.get(a, None) for a in attributes])
  c.execute("""INSERT OR IGNORE INTO interfaces (nodeId, identifier, type) VALUES (?,?,?)""", tup)
  retval = tup
  for address in iface.findall('./*'):
    tup = tuple([address.get('uri', None)])
    c.execute("""INSERT OR IGNORE INTO addresses (uri) VALUES (?)""", tup)
    c.execute("""INSERT OR IGNORE INTO interfaceAddresses SELECT i.id, a.id FROM interfaces AS i CROSS JOIN addresses as a WHERE i.identifier = ? AND a.uri = ?""", (iface.get('identifier', None), address.get('uri', None)))
  return retval

def parse_observe_interfaces(node, c, timestamp, root, entries):
  for iface in root.findall('./*'):
    parse_interface(node, c, iface)
    attributes = ['age', 'flags', 'identifier']
    tup = tuple([node, entries] + [iface.get(a, None) for a in attributes])
    c.execute("""INSERT INTO observeInterfaces SELECT ?,?,i.id,?,? FROM interfaces AS i WHERE i.identifier = ?""", tup)

def parse_observe_node_store(node, c, timestamp, root, entries):
  for node_ in root.findall('./*'):
    attributes = ['name', 'id', 'proxy_id', 'local_application', 'type']
    lis = [node_.get(a, None) for a in attributes]
    lis[3] = (lis[3] == 'true')
    tup = tuple(lis)
    c.execute("""INSERT OR IGNORE INTO virtualNodes (name, nodeId, proxyId, local_application, type) VALUES (?,?,?,?,?)""", tup)
    vid = c.execute("""SELECT id FROM virtualNodes WHERE name = ? AND nodeId = ? AND proxyId = ? AND local_application = ? AND type = ?""", tup).fetchone()[0]
    attributes = ['confirmed', 'num_objects_in_bloom_filter', 'stored']
    lis = [node_.get(a, None) for a in attributes]
    lis[0] = (lis[0] == 'Neighbor')
    lis[2] = (lis[2] == 'true')
    tup = tuple([node, entries, vid] + lis)
    c.execute("""INSERT INTO observeNodeStoreNodes VALUES (?,?,?,?,?,?)""", tup)
    for attr in node_.findall('Attribute'):
      attributes = ['name', 'value']
      tup = tuple([attr.get(a, None) for a in attributes])
      c.execute("""INSERT OR IGNORE INTO attrs (name, value) VALUES (?,?)""", tup)
      tup = tuple([node, entries, vid] + [attr.get(a, None) for a in attributes])
      c.execute("""INSERT INTO observeNodeStoreAttributes SELECT ?,?,?,a.id FROM attrs AS a WHERE a.name = ? AND a.value = ?""", tup)
    for interfaces in node_.findall('Interfaces'):
      for iface in interfaces.findall('Interface'):
        tup = parse_interface(node, c, iface)
        lis = list(tup)
        lis.insert(0, (iface.get('is_up', None) == 'true'))
        tup = tuple([node, entries, vid] + lis)
        c.execute("""INSERT INTO observeNodeStoreInterfaces SELECT ?,?,?,i.id,? FROM interfaces AS i WHERE i.nodeId = ? AND i.identifier = ? AND i.type = ?""", tup)

def parse_observe_node_description(node, c, timestamp, root, entries):
  for node_ in root.findall('./*'):
    for attrs in node_.findall('Attributes'):
      for attr in attrs.findall('Attribute'):
        attributes = ['name', 'value']
        tup = tuple([attr.get(a, None) for a in attributes])
        c.execute("""INSERT OR IGNORE INTO attrs (name, value) VALUES (?,?)""", tup)
        tup = tuple([node, entries] + [attr.get(a, None) for a in attributes])
        c.execute("""INSERT INTO observeNodeDescriptionAttributes SELECT ?,?,a.id FROM attrs AS a WHERE a.name = ? AND a.value = ?""", tup)

def parse_observe_cache_strategy(node, c, timestamp, root, entries):
  for cacheStrategyUtility in root.findall('CacheStrategyUtility'):
    attributes = ['allow_db_purging', 'bloomfilter_remove_delay_ms', 'current_capacity_ratio', 'current_drop_on_insert', 'current_dupe_do_recv', 'current_num_do', 'current_size_kb', 'db_size_threshold', 'event_based_purging', 'global_optimizer', 'handle_zero_size', 'keep_in_bloomfilter', 'knapsack_optimizer', 'manage_locally_sent_files', 'manage_only_remote_files', 'max_capacity_kb', 'poll_period_ms', 'total_db_evicted', 'total_do_evicted', 'total_do_evicted_bytes', 'total_do_hard_evicted', 'total_do_hard_evicted_bytes', 'total_do_inserted', 'total_do_inserted_bytes', 'utility_compute_period_ms', 'utility_function', 'watermark_capacity_kb', 'watermark_capacity_ratio']
    lis = [cacheStrategyUtility.get(a, None) for a in attributes]
    for idx in [0, 8, 10, 11, 13, 14]:
      lis[idx] = (lis[idx] == 'enabled')
    tup = tuple([node, entries] + lis)
    c.execute("""INSERT INTO observeCacheStrategy VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""", tup)

def parse_observer_file(node, db, input):

  handlers = {
    'ObserveInterfaces' : parse_observe_interfaces,
    'ObserveNodeStore' : parse_observe_node_store,
    'ObserveNodeDescription' : parse_observe_node_description,
    'ObserveNodeMetrics' : parse_observe_node_metrics,
    'ObserveCertificates' : parse_observe_certificates,
    'ObserveRoutingTable' : unhandled_parser,
    'ObserveProtocols' : unhandled_parser,
    'ObserveCacheStrategy' : parse_observe_cache_strategy,
    'ObserveDataStoreDump': unhandled_parser,
  }

  entries = dict((k, 1) for k in handlers.keys())

  c = db.cursor()
  node_ = c.execute("""SELECT id FROM nodes WHERE name = ?""", (node,)).fetchone()
  if node_ is None:
    print 'WARNING! Could not find node %s' % node
    return
  node = node_[0]

  blocks = [b for b in input.read().split('\n<?xml version="1.0"?>\n') if len(b.strip()) > 0]
  blocks = [b for b in blocks if b.find('<Observe') != -1]
  for block in blocks:
    root = ET.fromstring(block)
    timestamp = None
    if root.tag not in handlers:
      raise Exception('Unhandled observable %s!' % root.tag)
    handlers[root.tag](node, c, timestamp, root, entries[root.tag])
    entries[root.tag] = entries[root.tag] + 1

  db.commit()

def parse_haggle_db(db, stream, nodeName):

  with tempfile.NamedTemporaryFile() as tmp:
    shutil.copyfileobj(stream, tmp)
    tmp.flush()
    haggleDB = sqlite3.connect(tmp.name)
    c = db.cursor()
    h = haggleDB.cursor()
    entries = [(binascii.hexlify(id[0]), nodeName) for id in h.execute("""SELECT id FROM table_dataobjects""").fetchall()]
    c.executemany("""INSERT OR REPLACE INTO dosInHaggleDB(doId, nodeId) SELECT d.id, n.id FROM dos AS d CROSS JOIN nodes AS n WHERE d.doId = ? AND n.name = ?""", entries)
  db.commit()

def create_squads(squads, db):

  c = db.cursor()
  for (squad, nodes) in squads.items():
    entries = [(squad, node, node) for node in nodes]
    c.executemany("""INSERT INTO squads(squadName, nodeName, nodeId) SELECT ?,?,id FROM nodes WHERE name = ?""", entries)
  db.commit()

def create_custom_table(db, table):

  name = table['name']
  columns = table['columns']
  indices = table.get('indices', [])
  unique_indices = table.get('unique_indices', [])
  values = table.get('values', [])

  columnStrings = ["%s %s" % (c['name'], c['type']) for c in columns]
  columnString = ',\n'.join(columnStrings)
  qs = ','.join('?' for c in columns)

  c = db.cursor()
  c.execute("""CREATE TABLE %s (%s)""" % (name, columnString))

  for column in indices:
    c.execute("""CREATE INDEX %s %s ON %s (%s)""" % (name, column, name, column))

  for column in unique_indices:
    c.execute("""CREATE UNIQUE INDEX %s%s ON %s (%s)""" % (name, column, name, column))

  c.executemany("""INSERT INTO %s VALUES (%s)""" % (name, qs), map(tuple, values))

  db.commit()

def create_custom_view(db, query):

  c = db.cursor()
  c.execute(query)
  db.commit()

def print_report(db, dirname, squads, nodes, report, verbose):

  if report.get('skip', False):
    return

  types = report.get('type', [])
  if not isinstance(types, list):
    types = [types]

  if 'nodes' in types:
    iteratee = nodes
  elif 'squads' in types:
    iteratee = squads
  else:
    iteratee = [{}]

  fetchall = False
  if 'fetchall' in types:
    if len(report['fields']) != 1:
      raise Exception("Can't have a fetchall with more than one field!")
    fetchall = True

  silent = False
  if 'silent' in types:
    silent = True

  transpose = False
  if 'transpose' in types:
    transpose = True

  logFile = report.get('logfile', None)

  fieldNames = []
  for field in report['fields']:
    name = field.get('name')
    if isinstance(name, list):
      fieldNames.extend(name)
    else:
      fieldNames.append(name)

  format = report.get('format', 'pretty')
  if format == 'pretty':
    separator = ' - '
  elif format == 'csv':
    separator = ','

  c = db.cursor()

  repeats = report.get('repeat', [None])
  for repeat in repeats:

    suffix = ''
    if repeat is not None:
      suffix = ' - %s' % repeat

    rows = []
    for (i, e) in enumerate(iteratee):
      item = e.copy()
      item['^repeat'] = repeat
      values = []
      for field in report['fields']:
        query = field['query']
        formats = field.get('format', ['%s'])
        defaults = field.get('default', [0])
        args = tuple([item[a] for a in field.get('args', [])])
        type = field.get('type', 'single')

        if not isinstance(formats, list):
          formats = [formats]
        if not isinstance(defaults, list):
          defaults = [defaults]

        start = time.time()
        if fetchall or transpose:
          results = c.execute(query, args).fetchall()
        else:
          results = [c.execute(query, args).fetchone()]
        end = time.time()
        if verbose:
          print ('%s took %.2f ms' % (query.replace('?', '\'%s\'') % args, (end-start)*1000))

        for row in results:
          for (j, res) in enumerate(row):
            if res is None:
              res = defaults[j]
            values.append(formats[j] % (res, ))

          if fetchall:
            rows.append(values)
            values = []

        if transpose:
          rows.append(values)
          values = []

      if not (fetchall or transpose):
        rows.append(values)

    if transpose:
      rows = zip(*rows)

    if format == 'pretty':
      transposed = rows + [fieldNames]
      transposed = zip(*transposed)
      lengths = [max(map(len, r)) for r in transposed]
      formatString = separator.join('%%%ss' % l for l in lengths)
    elif format == 'csv':
      formatString = separator.join('%s' for name in fieldNames)

    if not silent:
      print '%s%s: ' % (report['title'], suffix)
      print formatString % tuple(fieldNames)
      for values in rows:
        print formatString % tuple(values)

    if logFile is not None:
      with open(os.path.join(dirname, logFile.replace('^repeat', repeat or '')), 'w') as stream:
        print>>stream, '%s%s: ' % (report['title'], suffix)
        print>>stream, formatString % tuple(fieldNames)
        for values in rows:
          print>>stream, formatString % tuple(values)

def parse_test_case(config, dirname, dbPath):

  squads = config.get('squads', {})
  parseHaggleDBs = config.get('parse_haggle_dbs', False)
  parseObserverFiles = config.get('parse_observer_files', False)
  customTables = config.get('custom_tables', [])
  customViews = config.get('custom_views', [])

  dbNodeNameRegex = re.compile("""db\/([a-zA-Z0-9]*).haggle.db""")
  appsOutputFile = glob.glob('%s%sapps_output' % (dirname, os.path.sep)) + glob.glob('%s%s*.apps_output' % (dirname, os.path.sep))
  testCaseFile = '%s%stestcase_testrunner.tar.gz' % (dirname, os.path.sep)
  haggleDBsFile = '%s%sdb.tar.gz' % (dirname, os.path.sep)

  if len(appsOutputFile) != 1:
    raise Exception("Couldn't find apps_output file!")

  db = create_db(dbPath)
  create_db_indices(db)
  appsOutputFile = appsOutputFile[0]
  with open(appsOutputFile) as appsOutputFileStream:
    with tarfile.open(testCaseFile, 'r:gz') as testcase:

        appShFile = [n for n in testcase.getnames() if n.find('app.sh') != -1]
        if len(appShFile) != 1:
          raise Exception("Couldn't find app.sh file!")
        appShFile = appShFile[0]
        appShFileStream = testcase.extractfile(testcase.getmember(appShFile))

        for table in customTables:
          create_custom_table(db, table)

        for view in customViews:
          create_custom_view(db, view)

        nodes = parse_app_sh_file(appShFileStream)
        parse_apps_output_file(nodes, db, appsOutputFileStream)
        create_squads(squads, db)

        if parseHaggleDBs:
          if os.path.exists(haggleDBsFile):
            with tarfile.open(haggleDBsFile, 'r:gz') as haggleDBs:
              for member in haggleDBs.getmembers():
                match = dbNodeNameRegex.match(member.name)
                if match is not None:
                  nodeName = match.groups(0)[0]

                  haggleDBStream = haggleDBs.extractfile(member)
                  parse_haggle_db(db, haggleDBStream, nodeName)
                  haggleDBStream.close()

        for nodeName in nodes.keys():

          haggleLogFileName = os.path.join(dirname, '%s.haggle.log' % nodeName)
          with open(haggleLogFileName) as haggleLogFileStream:
            parse_haggle_log_file(nodeName, db, haggleLogFileStream)

          if parseObserverFiles:
            observerLogFileName = os.path.join(dirname, '%s.observer.log' % nodeName)
            with open(observerLogFileName) as observerFileStream:
              parse_observer_file(nodeName, db, observerFileStream)

        appShFileStream.close()

  return (db, squads, nodes)

def load_test_case(config, dirname, dbPath):

  squads = config.get('squads', {})
  db = load_db(dbPath)

  c = db.cursor()

  nodes = []
  result = c.execute("""SELECT name, id, nodeId FROM nodes""").fetchall()
  for row in result:
    node = Node(row[0])
    node.id = row[1]
    node.nodeId = row[2]
    nodes.append(node)
  nodes = dict((node.name, node) for node in nodes)

  return (db, squads, nodes)

def handle_test_case(config, dirname, dbPath, load, clear, verbose):

  if dbPath != ':memory:':
    dbPath = os.path.join(dirname, dbPath)

  times = collections.OrderedDict()
  if load and clear:
    raise Exception("Can't load and clear database at the same time!")

  start = time.time()
  if load:
    if dbPath == ':memory:':
      raise Exception("Can't load in memory database!")

    if os.path.exists(dbPath):
      (db, squads, nodes) = load_test_case(config, dirname, dbPath)
    else:
      (db, squads, nodes) = parse_test_case(config, dirname, dbPath)

  else:

    if clear and os.path.exists(dbPath):
      os.remove(dbPath)

    (db, squads, nodes) = parse_test_case(config, dirname, dbPath)
  end = time.time()
  times['Parsing/Loading data'] = end-start

  for report in config.get('reports', []):
    start = time.time()
    sqs = [dict(name=name) for name in sorted(squads.keys())]
    nds = [nodes[name] for name in sorted(nodes.keys(), key=lambda n: int(n[1:]))]
    nds = [dict(name=n.name, id=n.id, nodeId=n.nodeId) for n in nds]
    print_report(db, dirname, sqs, nds, report, False)
    end = time.time()
    times['Report "%s"' % report['title']] = end-start
  db.close()

  if verbose:
    print '\nPerformance stats:'
    for (k, v) in times.items():
      print '%s took %.2f ms' % (k, v*1000)

  print 'Handled case %s' % dirname

def handle_test_case_wrapper(args):
  (params, dirname) = args
  (config, dbPath, load, clear, verbose) = params
  return handle_test_case(config, dirname, dbPath, load, clear, verbose)

DOCOPT_STRING = """Haggle Log Analyzer

Usage:
  analyze.py [options] <config_path> <log_dir>
  analyze.py -h | --help
  analyze.py --version

Options:
  -h --help           Show this screen.
  --version           Show version.
  -d --db=<path>      Path to Database [default: :memory:].
  -l --load           Load Database instead of creating, if it exists [default: false].
  -c --clear          Clear Database if it exists [default: false]
  -p --parallel=<n>   Parse data in parallel using n processes [default: 1].
  -v --verbose        Show verbose output (e.g. performance stats) 

"""


def main():

  arguments = docopt.docopt(DOCOPT_STRING, version='Haggle Log Analyzer 0.1')

  with open(arguments['<config_path>']) as input:
    config = json.load(input)

  basedir = os.path.abspath(arguments['<log_dir>'])
  dirs = [d for d in os.listdir(basedir) if os.path.isdir(os.path.join(basedir, d))]
  params = (config, arguments['--db'], arguments['--load'], arguments['--clear'], arguments['--verbose'])
  dirs = [os.path.abspath(os.path.join(basedir, d)) for d in sorted(dirs)]

  processes = int(arguments['--parallel'])
  if processes > 1:
    pool = multiprocessing.Pool(processes=processes)
    results = pool.map(handle_test_case_wrapper, [(params, d) for d in dirs])
  else:
    for d in dirs:
      handle_test_case_wrapper((params, d))

if __name__ == '__main__':
  main()
