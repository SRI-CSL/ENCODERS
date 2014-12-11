#!/bin/python

#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# Generate a Gephi file from the mobility for graph visualization. 
#

from heapq import heappush, heappop
import sys
import json
import glob
import copy

PERIOD_FUDGE_S=2 # we give an extra second since clock
                 # may not be exact

def main():
    output_dir = sys.argv[1]

    json_string = "[ "
    nbr_lists = glob.glob(output_dir + '/*neighbors.log')
    for nbr_list in nbr_lists:
        for line in open(nbr_list):
            json_string += line + ','
    json_string = json_string[:len(json_string)-1]
    json_string += " ]"

    data=json.loads(json_string)

    max_date = 0
    min_date = 1900000000

    # build node dictionary
    node_dict = {}
    for (time, ipa, ipb, period_s, max_latency_ms) in data:
        max_date = max(max_date, time)
        min_date = min(min_date, time)
        node_dict[str(ipa)] = True
        node_dict[str(ipb)] = True
    
    ip_node_map = {}
    node_count = 0
    for (ip, v) in node_dict.items():
        ip_node_map[ip] = node_count
        node_count += 1
 
    all_links = {}

    for (time, ipa, ipb, period_s, max_latency_ms) in data:
        n1 = ip_node_map[ipa]
        n2 = ip_node_map[ipb]
        if n2 < n1:
            tmp = n1
            n1 = n2
            n2 = tmp
        hash_name = str(n1) + "," + str(n2)
        try:
            links = all_links[hash_name]
        except Exception:
            links = []
        heappush(links, (time, (time, n1, n2, period_s, max(1, int(max_latency_ms/1000)))))
        all_links[hash_name] = links
    # build interval links
    slices = {}
    for k, v in all_links.items():
        while len(v) > 0:
            cur_ele = heappop(v)[1]
            start_interval = cur_ele[0]
            end_interval = cur_ele[0] + cur_ele[3]
            n1 = cur_ele[1]
            n2 = cur_ele[2]
            cur_expire_time = str(int(cur_ele[0]) + int(cur_ele[3]) + PERIOD_FUDGE_S)
            while len(v) > 0:
                cur_ele = heappop(v)[1] 
                cur_time = cur_ele[0]
                if int(cur_time) <= int(cur_expire_time):
                    cur_expire_time = str(int(cur_time) + int(cur_ele[3]) + PERIOD_FUDGE_S)
                    end_interval = cur_expire_time
                else:
                    heappush(v, (cur_time, cur_ele))
                    break
            try:
                slices[str(n1) + ',' + str(n2)] += [ (n1, n2, start_interval, end_interval) ]
            except:
                slices[str(n1) + ',' + str(n2)] = [ (n1, n2, start_interval, end_interval) ]
    slice_nodes = {}
    slice_dates = {}
    for k,v in slices.items():
        start_date = v[0][2]
        end_date = v[len(v)-1][3]
        slice_dates[k] = (start_date, end_date)
        slice_nodes[k] = (v[0][0], v[0][1])

    output = open(output_dir + "/neighbors.gexf", "w")
    output.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    output.write('<gexf xmlns="http://www.gexf.net/1.1draft"\n')
    output.write('  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n')
    output.write('  xsi:schemaLocation="http://www.gexf.net/1.1draft\n')
    output.write('                         http://www.gexf.net/1.1draft/gexf.xsd"\n')
    output.write('  version="1.1">\n')
    # parallel edges are not supported in gephi, 
    output.write('<graph mode="dynamic" defaultedgetype="undirected" start="'+ str(min_date) +'" end="' + str(max_date) + '">\n')
    output.write('  <nodes>\n')
    for (ip, v) in ip_node_map.items():
        output.write('      <node id="' + str(v) + '" label="' + ip + '" start="' + str(min_date) + '" end="' + str(max_date) + '" />\n')
    output.write('  </nodes>\n')
    output.write('  <edges>\n')
    edge_count = 0
    for (k, v) in slices.items():
        edge_start_date = slice_dates[k][0]
        edge_end_date = slice_dates[k][1]
        n1 = slice_nodes[k][0]
        n2 = slice_nodes[k][1]
        output.write('      <edge id="' + str(edge_count) + '" source="'+ str(n1) + '" target="' + str(n2) + '" start="' + str(edge_start_date) + '" end="' + str(edge_end_date) + '">\n')
        for (n1, n2, start_interval, end_interval) in slices[k]:
            output.write('          <slice start="' + str(start_interval) + '" end="' + str(end_interval) + '" />\n')
        output.write('      </edge>\n')
        edge_count += 1
    output.write('  </edges>\n')
    output.write('</graph>\n')
    output.write('</gexf>\n')
    output.close()

if __name__ == "__main__":
    main()
