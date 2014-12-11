# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

#!/usr/bin/env python
import os
import sys
import argparse
import glob
import shutil
import re

def convert_testsuite(template, original, destination):
  for test in glob.glob(os.path.join(original, '*')):
    convert_test(template, original, destination, os.path.basename(test))


def convert_test(template, original, destination, test):
  test_dir = os.path.join(destination, test)
  original_dir = os.path.join(original, test)

  # 1. Make a copy of the AndroidTestGenerator/TestTemplate directory.
  shutil.copytree(template, test_dir)

  # 2. Copy over app.sh, custom_validate.sh, echo_duration.sh, mobile.imn.template,
  #    mobile.scen, and any config.xml files from the old test.
  copy_old_files(original_dir, test_dir)

  # 3. Fill in template information.
  # 4. Edit start_up.sh
  # 5. Edit tear_down.sh
  fill_in_templates(original_dir, template, test_dir)

  # 6. Edit app.sh
  edit_app(original_dir, test_dir)

  # 7. Edit mobile.imn.template
  edit_imn(original_dir, test_dir)


def copy_old_files(original_dir, test_dir):
  files = ['custom_validate.sh', 'echo_duration.sh', 'mobile.scen', 'config.xml*']

  for f in files:
    filepaths = glob.glob(os.path.join(original_dir, f))
    for fs in filepaths:
      target = os.path.join(test_dir, os.path.basename(fs))
      shutil.copy2(fs, target)

def fill_in_templates(original_dir, template, test_dir):
  template_fill(template, test_dir, 'echo_num_devices.sh',
                [(r'%%num_devices%%', '%d' % get_num_devices(original_dir))])

  template_fill(template, test_dir, 'echo_output_path.sh',
                [(r'%%output_path%%', get_output_path(original_dir))])

  tmp_files = get_tmp_files(original_dir)
  template_fill(template, test_dir, 'start_up.sh',
                [(r'%%dd_string%%', dd_string(tmp_files)),
                 (r'%%push_string%%', push_string(tmp_files))])
  template_fill(template, test_dir, 'tear_down.sh',
                [(r'%%rm_string%%', rm_string(tmp_files))])

def get_num_devices(original_dir):
  imn_file = os.path.join(original_dir, 'mobile.imn.template')
  device_count = 0
  with open(imn_file) as f:
    for line in f:
      if "type router" in line:
        device_count += 1
  return device_count

def get_output_path(original_dir):
  output_path_file = os.path.join(original_dir, 'echo_output_path.sh')
  output_path_expr = r'ODIR=\$\(dirname "([^"]+)"\)'
  with open(output_path_file) as f:
    return re.search(output_path_expr, f.read()).group(1)

def get_tmp_files(original_dir):
  files = []
  pattern = r'dd if=/dev/urandom of=/tmp/(\S+) bs=(\d+) count=(\d+)'

  with open(os.path.join(original_dir, 'start_up.sh')) as f:
    for line in f:
      result = re.search(pattern, line)
      if result:
        files.append((line, result.group(1)))

  return files

def dd_string(files):
  return ''.join([string for (string, filename) in files])

def push_string(files):
  template = 'adb_n $NUM_DEVICES "push /tmp/%s /data/tmp/%s" &> /dev/null'
  return '\n'.join([template % (filename, filename) for (string, filename) in files])

def rm_string(files):
  template = 'rm /tmp/%s'
  return '\n'.join([template % filename for (string, filename) in files])

def edit_app(original_dir, test_dir):
  replacements = [
    (r'/tmp',                       '/data/tmp'),
    (r'/sbin/ifconfig eth0',        'busybox ifconfig usb0'),
    (r'/sbin/ifconfig eth[1-9]',    ''),
    (r'mktemp',                     'mktemp /data/tmp/temp.XXXX')]
  template_fill(original_dir, test_dir, 'app.sh', replacements)

def edit_imn(original_dir, test_dir):
  replacements = [
    (r'type router',                'type rj45'),
    (r'^\s+model PC\n',             ''),
    (r'interface eth[^!]+\t!',      'model\n\t!'),
    (r'^\s+services {[^}]+}\n',     ''),
    (r'interface-peer {eth0 n',     'interface-peer {0 n')]

  with open(os.path.join(original_dir, 'mobile.imn.template'), 'r') as infile, open(os.path.join(test_dir, 'mobile.imn.template'), 'w') as outfile:
    contents = infile.read()

    warn_mobility(contents, test_dir)

    hostnames = re.findall(r'hostname n(\d+)', contents)
    for hostname in hostnames:
      num = int(hostname)
      replacements.append((r'hostname n%d$' % num, 'hostname usb%d' % (num - 1)))

    for (string, replacement) in replacements:
      contents = re.sub(string, replacement, contents, flags=re.MULTILINE)

    outfile.write(contents)

def warn_mobility(contents, test_dir):
  if '%%scen_path%%' in contents:
    filename = os.path.join(test_dir, 'mobile.imn.template')
    print 'WARNING: The file'
    print '  %s' % filename
    print 'has not been fully converted because this script does not yet support mobility'
    print 'models. Please refer to the section "Changing .imn files from CORE 4.3 to 4.6"'
    print 'of the file README-porting-core for details on how to finish the conversion yourself.'

def template_fill(template_dir, test_dir, file, replace_tuples):
  with open(os.path.join(template_dir, file), 'r') as infile, open(os.path.join(test_dir, file), 'w') as outfile:
    for line in infile:
      for (string, replacement) in replace_tuples:
        line = re.sub(string, replacement, line)
      outfile.write(line)

def main():
  default_template = 'TestTemplate'

  if os.getenv('ANDROID_TESTRUNNER'):
    testrunner = os.getenv('ANDROID_TESTRUNNER')
  else:
    testrunner = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../testrunner")

  default_template = os.path.join(testrunner, '../testgenerator/TestTemplate')

  default_original = 'Testsuite'
  default_destination = 'AndroidTestsuite'

  parser = argparse.ArgumentParser(description='Converts an LXC testsuite to a corresponding Android testsuite.')
  parser.add_argument('-t', '--template', default=default_template, dest='template', metavar='TEMPLATE', help='The TestTemplate directory.')
  parser.add_argument('-s', '--source', default=default_original, dest='original', metavar='SOURCE', help='The directory containing the testsuite you want to convert.')
  parser.add_argument('-o', '--output', default=default_destination, dest='destination', metavar='OUTPUT', help='The directory to output the converted testsuite.')
  parser.add_argument('-f', '--force', dest='force', action='store_true', help='Remove and replace existing Android testsuite if found.')
  args = parser.parse_args()

  template = os.path.abspath(args.template)
  original = os.path.abspath(args.original)
  destination = os.path.abspath(args.destination)

  if not os.path.exists(template):
    print 'Template path %s does not exist.' % template
    sys.exit(1)

  if not os.path.exists(original):
    print 'Original test path %s does not exist.' % original
    sys.exit(1)

  if os.path.exists(destination):
    if args.force:
      shutil.rmtree(destination)
    else:
      print 'Android testsuite %s already exists. Use -f to overwrite.' % destination
      sys.exit(1)

  os.mkdir(destination)
  convert_testsuite(template, original, destination)

if __name__ == '__main__':
  main()
