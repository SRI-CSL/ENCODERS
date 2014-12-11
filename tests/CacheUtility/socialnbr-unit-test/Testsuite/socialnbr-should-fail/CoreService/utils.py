import os
import pwd
import commands

cur_dir = os.path.dirname(os.path.abspath(__file__))

def get_test_dir():
    test_dir = commands.getoutput('bash %s' % cur_dir + '/echo_test_dir.sh') 
    if test_dir[-1] == '/':
        return test_dir[:-1]
    return test_dir

def get_username():
    return commands.getoutput('bash %s' % cur_dir + '/echo_user.sh') 

def get_app_name(): 
    return commands.getoutput('bash %s' %  get_test_dir() + '/echo_test_appname.sh')

def get_cfg_name():
    return get_test_dir() + "/config.xml" 

def get_faillog_name():
    return commands.getoutput('bash %s' %  get_test_dir() + '/echo_fail_file.sh')

def get_tmp_name():
    return commands.getoutput('bash %s' %  get_test_dir() + '/echo_output_path.sh');
