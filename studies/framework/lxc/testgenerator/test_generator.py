#!/bin/python

#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# TestGenerator takes specifications in json format and outputs
# files that are compatible with TestRunner (mainly a bunch of
# bash scripts and an imn file).
# The output files are compatible for CORE 4.3.
# 
# This file uses application_factory, mobile_factory and
# macphy_factory to load application, mobility and mac layer
# modules from the MobilityModules/ and ApplicationModules/ directory.
# This separation allows one ot easily add new mobility and application
# modules with little (if any) needed changes to this file.
#
# This file uses many of the bash scripts in TestTemplate/ as templates
# when constructing the final files.
#
# In order to support relative paths in the test specification, we have to
# compute the paths at run-time (this is why TestTemplate/*.sh has so many
# scripts that return the location of certain files). 
# Relative paths are useful, because it allows us to send each other directories
# containing the test cases and run them without any further modifications. 
#

VERSION="0.5"

from utils import * 
import mobility_factory 
import application_factory
import macphy_factory
from security_node_manager import generateSecurityConfigsFromJSON

import sys
import json
import copy
import os
import shutil
import subprocess
import hashlib
import base64
import tarfile
import tempfile

USE_JSON_LINT=True

# We use json lint for extra error messages when parsing the json 
try:
    from demjson.jsonlint import lintcheck_data
except:
    USE_JSON_LINT=False

TEST_GENERATOR_PATH = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))

# generate an imn file that's compatible with CORE 4.3
def generate_imn_file(imn_file_name, N, area_width, area_height, macphy_module):

    of = open(imn_file_name, 'w')

    # NOTE: the CORE format uses inconsistent newlines and spaces, but
    # the following seems to work (after much trial and error)

    for i in range(1, N+1):
        of.write('node n%d {\n' % i)
        of.write('    type router\n')
        of.write('    model PC\n')
        of.write('    network-config {\n')
        of.write('\thostname n%d\n' % i)

        of.write(macphy_module.get_interfaces(i))

        of.write('    }\n')
        of.write('    canvas c1\n')
        of.write('    iconcoords {0.0 0.0}\n')
        of.write('    labelcoords {0.0 0.0}\n')

        of.write(macphy_module.get_interface_peers(i))

        of.write('    services {DefaultRoute HaggleService}\n')
        of.write('}\n')
        of.write('\n')
        
    of.write(macphy_module.get_macphy_node_configs())

    of.write(macphy_module.get_link_strings())

    of.write('\ncanvas c1 {\n')
    of.write('    name {Canvas1}\n')
    of.write('    refpt {0 0 0 0 0}\n')
    of.write('    scale {100}\n')
    of.write('    size {%d %d}\n' % (area_width, area_height))
    of.write('}\n')
    of.write('\n')
    of.write('option global {\n')
    of.write('    interface_names no\n')
    of.write('    ip_addresses yes\n')
    of.write('    ipv6_addresses yes\n')
    of.write('    node_labels yes\n')
    of.write('    link_labels yes\n')
    of.write('    ipsec_configs yes\n')
    of.write('    exec_errors yes\n')
    of.write('    show_api no\n')
    of.write('    background_images no\n')
    of.write('    annotations yes\n')
    of.write('    grid yes\n')
    of.write('    traffic_start 0\n')
    of.write('}\n')
    of.close()

# generate the bash script that's responsible for test start-up 
# i.e. creating the application files to be published
def generate_startup_file(template_file, startup_file, files, num_nodes):
    dd_template_string = "dd if=/dev/urandom of=%s_n%s bs=1024 count=%s >& /dev/null\n"
    dd_string = ""
    for i in xrange(1, num_nodes+1):
        for f_tuple in files:
            dd_string = dd_string + (dd_template_string % (f_tuple[0], i, f_tuple[1]))
    replace_file(template_file, startup_file, [ ('%%dd_string%%',  dd_string) ])
    chmodx(startup_file)

# generate the bash script that's responsible for test tear-down.
def generate_teardown_file(template_file, teardown_file, files, kill_hard):
    rm_template_string = "rm -f %s_n*\n"
    rm_string = ""
    for f_tuple in files:
        rm_string = rm_string + (rm_template_string % (f_tuple[0], ))
    replace_file(template_file, teardown_file, [ ('%%rm_string%%',  rm_string), ('%%kill_hard%%', kill_hard) ])
    chmodx(teardown_file)

def create_security_configs(cfg, num_nodes):
    path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "haggleNodeIDTable.txt")
    if not os.path.exists(path):
        raise Exception("Need haggleNodeIDTable.txt for security config!")
    IDTable = []
    with open(path) as f:
        IDTable = f.read().replace('[', '').replace(']','').split('\n')
    if len(IDTable) < num_nodes:
        raise Exception("Need at least %s IDs in haggleNodeIDTable.txt! Have only %s" % (num_nodes, len(IDTable)))

    cfg['haggleNodeIDs'] = cfg.get('haggleNodeIDs', {})
    for i, haggleNodeID in enumerate(IDTable):
        cfg['haggleNodeIDs']['n%s' % (i+1)] = cfg['haggleNodeIDs'].get('n%s' % (i+1), haggleNodeID)
    
    nodes = generateSecurityConfigsFromJSON(cfg)
    return nodes

# caching
MOBILITY_CACHE = {}
MOBILITY_TMP_FOLDER = '/tmp/mobility_tmp_folder'
def generate_cached_scen_file(mobility_module, mobility_cfg, test_output_path, suffix):
    cache_key = json.dumps(mobility_cfg)
    if not cache_key in MOBILITY_CACHE:
        if os.path.exists(MOBILITY_TMP_FOLDER):
            shutil.rmtree(MOBILITY_TMP_FOLDER)
        os.makedirs(MOBILITY_TMP_FOLDER)
        if not mobility_module.generate_scen_file(os.path.join(MOBILITY_TMP_FOLDER, suffix)):
            return False
        tmpfil = tempfile.NamedTemporaryFile()
        MOBILITY_CACHE[cache_key] = tmpfil
        cwd = os.path.abspath(os.getcwd())
        os.chdir(MOBILITY_TMP_FOLDER)
        with tarfile.TarFile(tmpfil.name, 'w') as tar:
            tar.add('.')
        os.chdir(cwd)
        tmpfil.flush()
        shutil.rmtree(MOBILITY_TMP_FOLDER)

    tmpfil = MOBILITY_CACHE[cache_key]
    with tarfile.TarFile(tmpfil.name) as tar:
        tar.extractall(test_output_path)
    return True

APPLICATION_CACHE = {}
def generate_cached_app_file(application_module, application_cfg, output_path):
    cache_key = json.dumps(application_cfg)
    if not cache_key in APPLICATION_CACHE:
        tmpfil = tempfile.NamedTemporaryFile()
        APPLICATION_CACHE[cache_key] = tmpfil
        if not application_module.generate_app_file(tmpfil.name):
            return False
        tmpfil.flush()
    tmpfil = APPLICATION_CACHE[cache_key]
    shutil.copyfile(tmpfil.name, output_path)
    return True

SECURITY_CACHE = {}
SECURITY_NODE_MANAGER_CACHE = {}
SECURITY_TMP_FOLDER = '/tmp/security_tmp_folder'
def generate_cached_security_configs(security_cfg, num_nodes, config_path, per_node_configs, config_replace_dict, test_spec_path, test_output_path):
    cache_nm_key = json.dumps(security_cfg)
    if not cache_nm_key in SECURITY_NODE_MANAGER_CACHE:
        security_node_manager = create_security_configs(security_cfg, num_nodes)
        SECURITY_NODE_MANAGER_CACHE[cache_nm_key] = security_node_manager
    security_node_manager = SECURITY_NODE_MANAGER_CACHE[cache_nm_key]

    cache_entry = dict(num_nodes=num_nodes, config_path=config_path, per_node_configs=per_node_configs, config_replace_dict=config_replace_dict, cache_nm_key=cache_nm_key)
    cache_key = json.dumps(cache_entry)
    if not cache_key in SECURITY_CACHE:
        if os.path.exists(SECURITY_TMP_FOLDER):
            shutil.rmtree(SECURITY_TMP_FOLDER)
        os.makedirs(SECURITY_TMP_FOLDER)

        for i in xrange(1, num_nodes+1):
            node_name = 'n%s' % i
            base_cfg_file = per_node_configs.get(node_name, config_path)
            if base_cfg_file != config_path:
                base_cfg_file = os.path.abspath(os.path.join(test_spec_path, base_cfg_file))
            security_node_manager[node_name].createConfig(haggleConfigLocation=base_cfg_file, fullDebugging=False, **security_cfg.get('perNodeSecurityParameters', {}).get(node_name, {}))
            with open('/tmp/temp_node_cfg_file', 'w') as output:
                output.write(security_node_manager[node_name].config)
            replace_file('/tmp/temp_node_cfg_file', os.path.join(SECURITY_TMP_FOLDER, 'config.xml.%s' % node_name), config_replace_dict.items())

        tmpfil = tempfile.NamedTemporaryFile()
        SECURITY_CACHE[cache_key] = tmpfil
        cwd = os.path.abspath(os.getcwd())
        os.chdir(SECURITY_TMP_FOLDER)
        with tarfile.TarFile(tmpfil.name, 'w') as tar:
            tar.add('.')
        os.chdir(cwd)
        tmpfil.flush()
        shutil.rmtree(SECURITY_TMP_FOLDER)

    tmpfil = SECURITY_CACHE[cache_key]
    with tarfile.TarFile(tmpfil.name) as tar:
        tar.extractall(test_output_path)
    return True

# the main function to generate the bash files called by TestRunner
def create_test(        \
    test_spec_path,     \
    json_str,           \
    test_name,          \
    test_output_path,   \
    result_output_path, \
    config,             \
    num_nodes,          \
    duration,           \
    warmup,             \
    seed,               \
    area_width,         \
    area_height,        \
    files,              \
    macphys_cfgs,       \
    mobility_cfg,       \
    application_cfg,    \
    test_template=os.path.join(TEST_GENERATOR_PATH, 'TestTemplate/'), \
    cpu_limit="none",   \
    haggled="/usr/local/bin/haggle", \
    fail_file="/tmp/faillog", \
    pretty_settings="",
    custom_validate={},
    neighbor_monitor={},
    security_cfg={},
    observer={}):

    # these are hard coded based on our experiments across a variety of systems
    START_KILL_DELAY=10
    HARD_KILL_DELAY=300
    SHUTDOWN_DELAY=max(2*num_nodes,10) # this is time for buffers to flush and ifconfig to be output to apps_output
    # sometimes we need haggletest to try and connect multiple times (i.e. if there are 
    # many nodes and the cpu is constrained)
    MAX_CONNECT_TRIES=6
    MAX_CONNECT_TIME=MAX_CONNECT_TRIES * (MAX_CONNECT_TRIES + 1) / 2
    TEST_SPEC_PATH = os.path.abspath(os.path.dirname(os.path.realpath(test_spec_path)))

    test_output_path = os.path.abspath(os.path.join(TEST_SPEC_PATH, test_output_path)) + '/'
    result_output_path = os.path.abspath(os.path.join(TEST_SPEC_PATH, result_output_path)) + '/'
    # some of the modules need top leve parameters 
    try:
        mobility_cfg['parameters'] # some mobility models have no explicit params
    except:
        mobility_cfg['parameters'] = {}

    num_nodes = int(num_nodes)
    duration = int(duration)
    warmup = int(warmup)
    seed = int(seed)
    area_width = int(area_width)
    area_height = int(area_height)

    testset(mobility_cfg['parameters'], "num_nodes",   num_nodes)
    testset(mobility_cfg['parameters'], "area_width",  area_width)
    testset(mobility_cfg['parameters'], "area_height", area_height)
    testset(mobility_cfg['parameters'], "duration",    duration + START_KILL_DELAY + HARD_KILL_DELAY + MAX_CONNECT_TIME + SHUTDOWN_DELAY)
    testset(mobility_cfg['parameters'], "seed",        seed)
        
    app_duration = duration-warmup-MAX_CONNECT_TIME-SHUTDOWN_DELAY
    if app_duration < 10:
        raise Exception("Application duration is too short!")
    testset(application_cfg['parameters'],  "num_nodes", num_nodes)
    testset(application_cfg['parameters'],   "duration",  app_duration)
    testset(application_cfg['parameters'],       "seed",      seed)
    testset(application_cfg['parameters'],      "files",     files)

    mobility_module = mobility_factory.get_mobility_module(mobility_cfg)

    if not mobility_module:
        raise Exception("Could not load mobility module.")

    macphy_module = macphy_factory.get_macphy_module(num_nodes, macphys_cfgs)
    if not macphy_module:
        raise Exception("Could not load mac phys module.")

    application_module = application_factory.get_application_module(application_cfg)
    if not application_module:
        raise Exception("Could not load application module.")

    config_path = os.path.abspath(os.path.join(TEST_SPEC_PATH, config['path']))

    # allow autogeneration of pretty test names
    pretty_print_name = ""
    pretty_print_name += "N%d" % num_nodes
    pretty_print_name += "D%d" % duration
    pretty_print_name += "s%d" % seed
    pretty_print_name += "-"
    pretty_print_name += "%dx%d" % (area_width, area_height)
    if str(cpu_limit) != "none":
        pretty_print_name += "cl%.2f" % cpu_limit

    pretty_print_name += "-"
    # add config pretty name
    try:
        pretty_print_name += config['pretty_name']
    except:
        pretty_print_name += os.path.basename(config_path)
    
    # unfortunately the names are getting too long!
    try:
        pretty_enable_mobility = pretty_settings['enable_mobility'] == "true"
    except:
        pretty_enable_mobility = False

    try:
        pretty_enable_macphy = pretty_settings['enable_macphy'] == "true"
    except:
        pretty_enable_macphy = False

    try:
        pretty_enable_app = pretty_settings['enable_app'] == "true"
    except:
        pretty_enable_app = False

    try:
        pretty_enable_security = pretty_settings['enable_security'] == "true"
    except:
        pretty_enable_security = False

    try:
        pretty_verbosity = int(pretty_settings['verbosity'])
    except:
        pretty_verbosity = 0

    try:
        max_len = max(int(pretty_settings['max_len']), 10)
    except:
        max_len = 100

    if pretty_enable_mobility:
        pretty_print_name += "-"
        pretty_print_name += mobility_module.get_pretty_name(pretty_verbosity)
    if pretty_enable_macphy:
        pretty_print_name += "-"
        pretty_print_name += macphy_module.get_pretty_name(pretty_verbosity)
    if pretty_enable_app:
        pretty_print_name += "-"
        pretty_print_name += application_module.get_pretty_name(pretty_verbosity)
    if pretty_enable_security:
        cfg = config.get('security_cfg', security_cfg)
        pname = cfg.get('pretty_name', '')
        if len(pname) > 0:
            pretty_print_name += "-"
            pretty_print_name += pname

    test_name  = test_name.replace("%%pretty%%", str(pretty_print_name))
    test_output_path = test_output_path.replace("%%pretty%%", str(pretty_print_name))
    result_output_path = result_output_path.replace("%%pretty%%", str(pretty_print_name))

    if len(pretty_print_name) > max_len:
        # file name is too large!
        pretty_print_name = pretty_print_name[:max_len-2] + ".."

    # done w/ pretty print of name

    try:
        mkdir(test_output_path)
    except:
        raise Exception("Could not create test directory: (%s), does it already exist?" % test_output_path)

    # copy json that created the test, for our bookkeeping 
    json_file = open(test_output_path + 'test.json', 'w')
    json_file.write(json_str)
    json_file.close()

    # copy standard test template files over to new test
    replace_file(test_template + 'echo_duration.sh', test_output_path + 'echo_duration.sh', [ ( '%%duration%%', duration + MAX_CONNECT_TIME + START_KILL_DELAY + SHUTDOWN_DELAY) ])
    chmodx(test_output_path + 'echo_duration.sh')

    replace_file(test_template + 'echo_output_path.sh', test_output_path + 'echo_output_path.sh', [ ( '%%output_path%%', result_output_path ) ])
    chmodx(test_output_path + 'echo_output_path.sh')

    replace_file(test_template + 'cpulimit_pid.sh', test_output_path + 'cpulimit_pid.sh', [ ('%%cpu_limit%%', cpu_limit), ('%%app_duration%%', app_duration) ])
    chmodx(test_output_path + 'cpulimit_pid.sh')

    cp(test_template + 'ls_db.py', test_output_path + 'ls_db.py')
    chmodx(test_output_path + 'ls_db.py')

    cp(test_template + 'stats.awk', test_output_path + 'stats.awk')
    chmodx(test_output_path + 'stats.awk')

    cp(test_template + 'generate_gexf.py', test_output_path + 'generate_gexf.py')
    chmodx(test_output_path + 'generate_gexf.py')

    replace_file(test_template + 'echo_stats.sh', test_output_path + 'echo_stats.sh', [ ('%%num_nodes%%', num_nodes), ('%%duration%%', duration) ] )
    chmodx(test_output_path + 'echo_stats.sh')

    cp(test_template + 'echo_test_appname.sh', test_output_path + 'echo_test_appname.sh')
    chmodx(test_output_path + 'echo_test_appname.sh')

    cp(test_template + 'echo_testscenario.sh', test_output_path + 'echo_testscenario.sh')
    chmodx(test_output_path + 'echo_testscenario.sh')

    replace_file(test_template + 'start_haggle.sh', test_output_path + 'start_haggle.sh', [ ( '%%haggled%%', haggled ), ( '%%connect_tries%%', MAX_CONNECT_TRIES) ])
    chmodx(test_output_path + 'start_haggle.sh')

    replace_file(test_template + 'start_resource_monitor.sh', test_output_path + 'start_resource_monitor.sh', [ ( '%%duration%%', duration ) ])
    chmodx(test_output_path + 'start_resource_monitor.sh')

    replace_file(test_template + 'start_neighbor_monitor.sh', test_output_path + 'start_neighbor_monitor.sh', [ ( '%%duration%%', duration ) ] + [ ('%%' + k + '%%', v) for (k, v) in neighbor_monitor.items() ])
    chmodx(test_output_path + 'start_neighbor_monitor.sh')

    replace_file(test_template + 'start_observer.sh', test_output_path + 'start_observer.sh', [ ( '%%duration%%', duration ) ] + [ ('%%' + k + '%%', v) for (k, v) in observer.items() ])
    chmodx(test_output_path + 'start_observer.sh')

    replace_file(test_template + 'echo_fail_file.sh', test_output_path + 'echo_fail_file.sh', [ ( '%%fail_file%%', fail_file ) ])
    chmodx(test_output_path + 'echo_fail_file.sh')

    replace_file(test_template + 'validate.sh', test_output_path + 'validate.sh', [ ( '%%num_nodes%%', num_nodes ), ( '%%fail_file%%', fail_file) ])
    chmodx(test_output_path + 'validate.sh')

    try:
        validate_replace_dict = custom_validate['replace_dict'] 
    except:
        validate_replace_dict = {}

    try:
        replace_file(custom_validate['path'], test_output_path + 'custom_validate.sh', validate_replace_dict.items() + [ ( '%%num_nodes%%', num_nodes ), ( '%%fail_file%%', fail_file) ])
        chmodx(test_output_path + 'custom_validate.sh')
    except:
        None

    replace_file(test_template + 'stop_haggle.sh', test_output_path + 'stop_haggle.sh', [ ('%%kill_soft%%', duration + START_KILL_DELAY), ('%%kill_hard%%', HARD_KILL_DELAY) ])
    chmodx(test_output_path + 'stop_haggle.sh')

    # start config templating
    try:
        config_replace_dict = config['replace_dict']
    except:
        config_replace_dict = {}
    try:
        per_node_configs = config['per_node_configs']
    except:
        per_node_configs = {}

    security_cfg = config.get('security_cfg', security_cfg)
    if len(security_cfg) > 0:
        generate_cached_security_configs(security_cfg, num_nodes, config_path, per_node_configs, config_replace_dict, TEST_SPEC_PATH, test_output_path)
    else:
        replace_file(config_path, test_output_path + 'config.xml', config_replace_dict.items())
        for (k, v) in per_node_configs.items():
            v = os.path.abspath(os.path.join(TEST_SPEC_PATH, v))
            replace_file(v, test_output_path + 'config.xml.%s' % k, config_replace_dict.items())
    # end config templating

    mkdir(test_output_path + "CoreService")
    replace_file(test_template + 'CoreService/haggle.py', test_output_path + 'CoreService/haggle.py', [ ('%%warmup%%', warmup) ])
    cp(test_template + 'CoreService/__init__.py', test_output_path + 'CoreService/__init__.py')
    cp(test_template + 'CoreService/utils.py', test_output_path + 'CoreService/utils.py')

    generate_imn_file(test_output_path + 'mobile.imn.template', num_nodes, area_width, area_height, macphy_module)

    generate_startup_file(test_template + 'start_up.sh', test_output_path + 'start_up.sh', files, num_nodes)

    generate_teardown_file(test_template + 'tear_down.sh', test_output_path + 'tear_down.sh', files, HARD_KILL_DELAY)

    # copy over application specific test files

    if not generate_cached_scen_file(mobility_module, mobility_cfg, test_output_path, 'mobile'):
        raise Exception("Mobility module could not generate scen file.")

    if not generate_cached_app_file(application_module, application_cfg, test_output_path + 'app.sh'):
        raise Exception("Application module could not generate app file")

    chmodx(test_output_path + 'app.sh')

    
    test_gen_path=os.path.abspath(os.path.dirname(__file__))
    test_template_path=os.path.abspath(test_template)
    test_output_gen_archive=os.path.abspath(test_output_path + 'testgenerator')
    test_output_template_archive=os.path.abspath(test_output_path + 'testtemplate')

    shutil.make_archive(str(test_output_gen_archive), 'gztar', test_gen_path)
    shutil.make_archive(str(test_output_template_archive), 'gztar', test_template_path)

    print "Test was created: %s" % test_output_path

# The below code is used to parse the json file and expand the meta characters.

# SW: this ugly two-headed behemoth is to replace "repeat()" with new test cases
# not for the faint of heart
def expand_list_permutations(val, lists):
    if not isinstance(val, list):
        print "Wrong type!"
        return
    for i in val:
        if not isinstance(i, dict) and not isinstance(i, list):
            for l in lists:
                l += [i]
        elif isinstance(i, dict):
            sub_perms = [ i ]
            expand_permutations(i, sub_perms)
            new_lists = []
            for l in lists:
                for j in sub_perms:
                    new_l = copy.deepcopy(l)
                    new_l += [ j ]
                    new_lists += [ new_l ]
            lists[:] = new_lists
        else:
            sub_perms = [ [] ]
            expand_list_permutations(i, sub_perms)
            new_lists = []
            for l in lists:
                for j in sub_perms:
                    new_l = copy.deepcopy(l)
                    new_l += [ j ]
                    new_lists += [ new_l ]
            lists[:] = new_lists

def expand_permutations(d, new_dicts):
    if not isinstance(d, dict):
        print "Wrong type!"
        return
    for (key,val) in d.items():
        if isinstance(val, dict) and "^repeat" in val:
            vals = val['^repeat']
            if not isinstance(vals, list):
                print "invalid repeat format (not a list)!"
                return
            sub_dicts = []
            for i in vals:
                for new_dict in new_dicts:
                    if isinstance(i, dict):
                        sub_perms = [ i ]
                        expand_permutations(i, sub_perms)
                        for j in sub_perms:
                            sc = copy.deepcopy(new_dict)
                            sc[key] = j
                            sub_dicts += [sc]
                    else:
                        sc = copy.deepcopy(new_dict)
                        sc[key] = i
                        sub_dicts += [sc]
            new_dicts[:] = sub_dicts
        elif isinstance(val, dict):
            sub_dicts = []
            for new_dict in new_dicts:
                sub_perms = [ val ]
                expand_permutations(val, sub_perms)
                for j in sub_perms:
                    sc = copy.deepcopy(new_dict)
                    sc[key] = j
                    sub_dicts += [sc]
            new_dicts[:] = sub_dicts
        elif isinstance(val, list):
            sub_dicts = []
            lists = [ [] ]
            expand_list_permutations(val, lists)
            for l in lists:
                for new_dict in new_dicts:
                    sc = copy.deepcopy(new_dict)
                    sc[key] = l
                    sub_dicts += [sc]
            new_dicts[:] = sub_dicts
# end repeat

## assignment metacharacter
def replace_string(s, assignments):
    new_s = str(s)
    for (k,v) in assignments.items():
        #print k,v, new_s
        new_s = new_s.replace(k, str(v))
    return new_s

def get_assignments(test, output_assignments):
    if isinstance(test, dict):
        try:
            get_assignments(test['^assign'], output_assignments)
            for (key, val) in test.items():
                if key != '^assign':
                    val = str(val)
                    if hasattr(output_assignments, key):
                        raise Exception("assignment already exists (namespace collision!)")
                    output_assignments[key] = val
        except:
            for (key,val) in test.items():
                get_assignments(val, output_assignments)
    if isinstance(test, list):
        for e in test:
            get_assignments(e, output_assignments)
    return

def replace_assignments(test, assignments):
    if isinstance(test, basestring):
        test = replace_string(test, assignments)
    if isinstance(test, dict):
        # strip meta assign
        try:
            a = replace_assignments(test['^assign'], assignments)
            return a
        except:
            None
        for (key,val) in test.items():
            before = val
            val = replace_assignments(val, assignments)
            new_key = replace_string(key, assignments)
            if new_key != key:
                del test[key]
            test[new_key] = val
    if isinstance(test, list):
        for i in range(len(test)):
            test[i] = replace_assignments(test[i], assignments)
    return test

def preprocess_assignments(tests):
    all_tests = []
    for test in tests:
        output_assignments = {}
        get_assignments(test, output_assignments)
        all_tests += [ replace_assignments(test, output_assignments) ]
    return all_tests

# end assignment preprocessing

def append_to_path(name, to_append):
    if name[len(name)-1] == '/':
        name = name[:len(name)-1]
    return name + to_append

def preprocess_ranges(tests):
    all_tests = []
    for test in tests:
        new_tests = [test]
        expand_permutations(test, new_tests)
        if len(new_tests) != 1:
            repeat_count = 1
            for test in new_tests:
                # make sure names are unique after expansion
                test['test_name'] = append_to_path(test['test_name'], '-%d' % repeat_count)
                test['test_output_path'] = append_to_path(test['test_output_path'], '-%d' % repeat_count)
                test['result_output_path'] = append_to_path(test['result_output_path'], '-%d' % repeat_count)
                repeat_count += 1
        all_tests += new_tests
    return all_tests

# validate that we have the files we need to use TestGenerator:
def check_env():
    devnull = open('/dev/null', 'w')
    ret = subprocess.call("which bm", stdout=devnull, stderr=devnull, shell=True)
    if ret != 0:
        print "Could not locate bonnmotion (bm) in PATH."
        return False
    return True

def print_usage():
    print "./test_generator.py sample_test_spec.json"

def main():

    if len(sys.argv) != 2:
        print "Invalid parameters."
        print_usage()
        sys.exit(2)

    if not check_env():
        print "Problems with environment, aborting."
        sys.exit(1)

    try:
        json_file_name = sys.argv[1]
        json_data = open(json_file_name).read()
    except Exception, e:
        print "Could not open file: %s" % json_file_name
        sys.exit(1)
        
    if USE_JSON_LINT:
        try:
            sys.stdout.write('JSON lint check: ')
            if not lintcheck_data(json_data, True):
                print "[FAILED] lintcheck failed."
                sys.exit(1)
        except Exception, e:
            print "[FAILED] Failed calling lint check."
            print e
            sys.exit(1)

    try:
        tests=json.loads(json_data)
    except ValueError, e: 
        print "Malformed JSON (see http://json.org for formal spec.) file: " + sys.argv[1]
        print e
        sys.exit(1)
    except Exception, e:
        print "Uknown error loading JSON: "
        print e
        sys.exit(1)

    try:
        tests=preprocess_ranges(tests)
    except Exception, e:
        print "Could not preprocess ranges"
        print e
        sys.exit(1)

    try:
        tests=preprocess_assignments(tests)
    except Exception, e:
        print "Could not preprocess assignments"
        print e
        sys.exit(1)

    try:
        for test in tests:
            pretty_json = json.dumps(test, sort_keys=True, indent=4, separators=(',', ': '))
            create_test(sys.argv[1], pretty_json, **test)
    except Exception, e:
        print "Could not create test"
        print e
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()

