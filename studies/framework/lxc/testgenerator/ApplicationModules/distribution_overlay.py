#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

#
# This application module supports several statistical
# distributions for the pubs and subs. These distributions
# can further be composed to create more complex applications.
#

from heapq import heappush, heappop
from time import time
import random
import bisect
import math
import copy
from ApplicationModules.custom import output_bash_to_file_obj

# thanks to: http://stackoverflow.com/questions/1366984/generate-random-numbers-distributed-by-zipf
class ZipfDiscreteGenerator:

    def recompute(self):
        n = len(self.elements)
        if n == 0:
            return
        tmp = [1. / (math.pow(float(i), self.alpha)) for i in range(1, n+1)]
        zeta = reduce(lambda sums, x: sums + [sums[-1] + x], tmp, [0])
        # Store the translation map:
        self.distMap = [x / zeta[-1] for x in zeta]


    def __init__(self, elements_by_rank, alpha, seed=1):
        # Calculate Zeta values from 1 to n:
        self.alpha = float(alpha)
        self.elements = copy.deepcopy(elements_by_rank)
        self.recompute()

        self.randomInstance = random.Random()
        self.randomInstance.seed(int(seed))
        self.orig_elements = copy.deepcopy(elements_by_rank)

    def next(self, remove=False):
        if len(self.elements) == 0:
            return None
        # Take a uniform 0-1 pseudo-random value:
        u = self.randomInstance.random()

        # Translate the Zipf variable:
        pos = bisect.bisect(self.distMap, u) - 1
        ret = self.elements[pos]
        if remove:
            del self.elements[pos]
            self.recompute()
        return ret

    def reset(self):
        self.elements = copy.deepcopy(self.orig_elements)
        self.recompute()

class UniformDiscreteGenerator:
    def __init__(self, elements, seed=1):
        self.elements = copy.deepcopy(elements)
        self.orig_elements = copy.deepcopy(elements)
        self.randomInstance = random.Random()
        self.randomInstance.seed(int(seed))

    def next(self, remove=False):
        if len(self.elements) == 0:
            return None
        pos = self.randomInstance.choice(range(0, len(self.elements)))
        ret = self.elements[pos]
        if remove:
            del self.elements[pos]
        return ret

    def reset(self):
        self.elements = copy.deepcopy(self.orig_elements)

class ExponentialGenerator:
    def __init__(self, lam, seed=1):
        self.lam = float(lam)
        self.randomInstance = random.Random()
        self.randomInstance.seed(int(seed))

    def next(self):
        return self.randomInstance.expovariate(self.lam)

class UniformGenerator:
    def __init__(self, low, high, seed=1):
        self.low = float(low)
        self.high = float(high)
        self.randomInstance = random.Random()
        self.randomInstance.seed(int(seed))

    def next(self):
        return self.randomInstance.uniform(self.low, self.high)

class NormalGenerator:
    def __init__(self, mu, std, seed=1):
        self.mu = float(mu)
        self.std = float(std)
        self.randomInstance = random.Random()
        self.randomInstance.seed(int(seed))

    def next(self):
        return self.randomInstance.gauss(self.mu, self.std)

def cont_dist_factory(cfg):
    dist_type = cfg['type']
    params = cfg['parameters']
    if dist_type == 'exponential':
        return ExponentialGenerator(**params)
    if dist_type == 'uniform':
        return UniformGenerator(**params)
    if dist_type == 'normal':
        return NormalGenerator(**params)
    raise Exception("unknown continuous distribution")

def discrete_dist_factory(cfg):
    dist_type = cfg['type']
    params = cfg['parameters']
    if dist_type == 'uniform_discrete':
        return UniformDiscreteGenerator(**params)
    if dist_type == 'zipf_discrete':
        return ZipfDiscreteGenerator(**params)
    raise Exception("unknown discrete distribution")

def get_attrs_from_dist(dist, num_attr):
    attrs = []
    for i in range(0, num_attr):
        attr = dist.next(True)
        if attr:
            # allow user to specify lists of lists of attributes
            new_attr_tuples = []
            if isinstance(attr[0], list):
                for (attr_k, attr_v) in [tuple(new_attr) for new_attr in attr]:
                    new_attr_tuples += [ (attr_k, attr_v) ]
            else:
                new_attr_tuples = [ (attr[0], attr[1]) ]
            attrs += new_attr_tuples
    dist.reset()
    return attrs

def get_pub_scripts(pub_distributions):
    pub_scripts = []
    for pub_distribution in pub_distributions:
        nodes = pub_distribution['nodes']
        delay_s = float(pub_distribution['delay_s'])
        duration_s = float(pub_distribution['duration_s'])
        max_pubs = int(pub_distribution['max_pubs'])
        extra_attrs = pub_distribution.get('extra_attributes', {})
        num_pub_dist = None
        if 'pub_num_attributes_distribution' in pub_distribution:
            num_pub_dist = discrete_dist_factory(pub_distribution['pub_num_attributes_distribution'])
        pub_attr_dist = discrete_dist_factory(pub_distribution['pub_attributes_distribution'])
        pub_file_dist = discrete_dist_factory(pub_distribution['pub_file_distribution'])
        inter_pub_dist = cont_dist_factory(pub_distribution['inter_pub_delay_s_distribution'])
        for node in nodes:
            cur_time = 0
            cur_pubs = 0
            while (cur_time < duration_s) and (cur_pubs < max_pubs):
                num_pubs = len(pub_distribution['pub_attributes_distribution']['parameters']['elements'])
                if num_pub_dist:
                    num_pubs = num_pub_dist.next()

                attrs = get_attrs_from_dist(pub_attr_dist, num_pubs)

                script = {
                    "node_num" : node,
                    "time"     : delay_s + cur_time,
                    "type"     : "pub",
                    "attributes" : attrs + extra_attrs.items()
                }

                pub_file = pub_file_dist.next()

                # optional pub file (user could specify "")
                if pub_file:
                    script["file"] = pub_file

                pub_scripts += [script]
                cur_pubs += 1
                cur_time += max(0, inter_pub_dist.next())
    return pub_scripts

def get_sub_scripts(sub_distributions):
    sub_scripts = []
    for sub_distribution in sub_distributions:
        nodes = sub_distribution['nodes']
        delay_s = float(sub_distribution['delay_s'])
        duration_s = float(sub_distribution['duration_s'])
        max_subs = int(sub_distribution['max_subs'])
        num_sub_dist = None
        if 'sub_num_attributes_distribution' in sub_distribution:
            num_sub_dist = discrete_dist_factory(sub_distribution['sub_num_attributes_distribution'])
        sub_attr_dist = discrete_dist_factory(sub_distribution['sub_attributes_distribution'])
        inter_sub_dist = cont_dist_factory(sub_distribution['inter_sub_delay_s_distribution'])
        sub_duration_dist = cont_dist_factory(sub_distribution['sub_duration_s_distribution'])
        for node in nodes:
            cur_time = 0
            cur_subs = 0
            while (int(duration_s - cur_time) > 0) and (cur_subs < max_subs):
                num_subs = len(sub_distribution['sub_attributes_distribution']['parameters']['elements'])
                if num_sub_dist:
                    num_subs = num_sub_dist.next()

                attrs = get_attrs_from_dist(sub_attr_dist, num_subs)

                duration = sub_duration_dist.next()
                duration = max(0, min(duration_s - cur_time, duration))
                script = {
                    "node_num" : node,
                    "time"     : delay_s + cur_time,
                    "type"     : "sub",
                    "duration" : duration,
                    "attributes" : attrs
                }
                sub_scripts += [script]
                cur_subs += 1
                sub_delay = inter_sub_dist.next()
                cur_time += max(0, sub_delay) + duration
    return sub_scripts

class Application:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_app_file(self, app_file):
        try:
            app = open(app_file, 'w')
            N = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            pub_distributions = self.cfg['pub_distributions']
            sub_distributions = self.cfg['sub_distributions']
            script = []
            pub_scripts = get_pub_scripts(pub_distributions)
            script += pub_scripts
            sub_scripts = get_sub_scripts(sub_distributions)
            script += sub_scripts
        except Exception, e:
            print e
            print "Invalid parameters to application configuration."
            return False
        try:
            output_bash_to_file_obj(app, N, duration, script, 1)
            app.close()
            return True
        except Exception, e:
            print "Could not create application."
            print e
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "dist-overlay"
        return pretty

