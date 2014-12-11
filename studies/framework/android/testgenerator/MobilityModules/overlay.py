#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


from heapq import heappush, heappop
import re
import commands
import io
from utils import *
from mobility_utils import convert_mobilities_for_core
import mobility_factory 

#
# Meta-mobility module: allows a composition of multiple
# mobility models, as tile overlays.
#

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            seed = int(self.cfg['seed'])
            mobilities = self.cfg['mobilities']
            try:
                offset_x = self.cfg['offset_x']
            except: 
                offset_x = 0
            try:
                offset_y = self.cfg['offset_y']
            except: 
                offset_y = 0
            try:
                delay = self.cfg['mobility_delay_s']
            except:
                delay = 0
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            mobility_results = []
            mob_count = 0
            for mobility_cfg in mobilities:
                mob_count = mob_count + 1
                current_nodes = mobility_cfg['nodes']
                testset(mobility_cfg, 'offset_x', offset_x)
                testset(mobility_cfg, 'offset_y', offset_y)
                testset(mobility_cfg['parameters'], "num_nodes",   len(current_nodes))
                testset(mobility_cfg['parameters'], "duration",    int(duration))
                testset(mobility_cfg['parameters'], "area_width",  int(area_width))
                testset(mobility_cfg['parameters'], "area_height", int(area_height))
                testset(mobility_cfg['parameters'], "seed",        int(seed))
                if (mobility_cfg['parameters']['area_width'] + mobility_cfg['offset_x']) > area_width:
                    raise Exception("x offset+width is greater than maximum area")
                if (mobility_cfg['parameters']['area_height'] + mobility_cfg['offset_y']) > area_height:
                    raise Exception("y offset+width is greater than maximum area")
                mobility_module = mobility_factory.get_mobility_module(mobility_cfg)
                current_mobile_path = output_path + 'mobile.%d' % mob_count
                if not mobility_module:
                    raise Exception("Could not load mobility module")
                if not mobility_module.generate_scen_file(current_mobile_path):
                    raise Exception("Could not generate scen file")
                mobility_results = mobility_results + [ (current_mobile_path + '.scen', current_nodes,  int(mobility_cfg['offset_x']), int(mobility_cfg['offset_y'])) ]
                convert_mobilities_for_core(output_path + '.scen', mobility_results, num_nodes, duration, area_width, area_height, delay)
            return True
        except Exception, e:
            print e
            print "Invalid mobility configuration"
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "overlay"
        return pretty
