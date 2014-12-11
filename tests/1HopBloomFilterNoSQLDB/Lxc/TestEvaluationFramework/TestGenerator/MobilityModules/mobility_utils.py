#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Utility functions for converting ns2 scen files to a format
# compatible with CORE. 
# NOTE: CORE is picky about how scen files are specified (more
# picky than ns2).
#

from heapq import heappush, heappop
import re
import io

def convert_mobilities_for_core(output_path, mobility_results, num_nodes, duration, area_width, area_height, delay=0):
    of = open(output_path, 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (num_nodes, duration, area_width, area_height))
    pos_array = []
    move_array = []
    comment_array = []
    pX = re.compile("\$node_\((\d+)\) set X_ (\d+)\.(\d+)")
    pY = re.compile("\$node_\((\d+)\) set Y_ (\d+)\.(\d+)")
    pZ = re.compile("\$ns_ at (\d+)\.(\d+) \"\$node_\((\d+)\) setdest (\d+)\.(\d+) (\d+)\.(\d+) (\d+)\.(\d+)\"")
    # organize the movements based on when they occur
    for mobility_result in mobility_results:
        (mobility_file, nodes, offset_x, offset_y) = mobility_result
        for line in io.open(mobility_file):
            mX = pX.search(line)
            mY = pY.search(line)
            mZ = pZ.search(line)
            if mX != None:
                node_num = nodes[int(mX.group(1))-1]
                float_val = float("%s.%s" % (mX.group(2), mX.group(3)))
                line = "$node_(%d) set X_ %f\n" % (node_num, float_val + offset_x)
            if mY != None:
                node_num = nodes[int(mY.group(1))-1]
                float_val = float("%s.%s" % (mY.group(2), mY.group(3)))
                line = "$node_(%d) set Y_ %f\n" % (node_num, float_val + offset_y)
            if line.startswith("$node_("):
                heappush(pos_array, (node_num ,line))
            elif line.startswith("#"):
                comment_array = comment_array + [ line ]
            elif mZ != None:
                timestamp = float("%s.%s" % (mZ.group(1), mZ.group(2))) + delay
                node_num = nodes[int(mZ.group(3))-1]
                destX = float("%s.%s" % (mZ.group(4), mZ.group(5))) + offset_x
                destY = float("%s.%s" % (mZ.group(6), mZ.group(7))) + offset_y
                rate = float("%s.%s" % (mZ.group(8), mZ.group(9)))
                line = "$ns_ at %f \"$node_(%d) setdest %f %f %f\"\n" % (timestamp, node_num, destX, destY, rate)
                heappush(move_array, (timestamp, line))
    while len(pos_array) > 0:
        line = heappop(pos_array)
        of.write(line[1])
    while len(move_array) > 0:
        line = heappop(move_array)
        of.write(line[1])
    of.close()

def convert_scen_for_core(path_in, path_out, N, duration, area_width, area_height, delay=0):
    mobilities = [ (path_in, range(1, N+1), 0, 0) ]
    convert_mobilities_for_core(path_out, mobilities, N, duration, area_width, area_height, delay)
