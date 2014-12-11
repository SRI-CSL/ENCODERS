# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

from __future__ import print_function
import xml.etree.ElementTree as etree
import argparse
import sys

def parse(filename, nem, timestep):
  query = './/{http://www.opengis.net/kml/2.2}coordinates'

  tree = etree.parse(filename)
  coordinates = tree.find(query).text.strip().split('\n')

  result = []
  t = 0.0

  for coords in coordinates:
    lon, lat, alt = coords.split(',')
    line = {}
    line['time'] = t
    line['nem'] = nem
    line['latitude'] = float(lat)
    line['longitude'] = float(lon)
    line['altitude'] = float(alt)
    result.append(line)
    t = t + timestep

  return result

def coord2str(coords):
  return "%f nem:%d location gps %f,%f,%f" % \
    (coords['time'], coords['nem'], coords['latitude'],
     coords['longitude'], coords['altitude'])

def main():
  parser = argparse.ArgumentParser(description='Convert a set of KML files into EEL coordinates for EMANE.')
  parser.add_argument('--timestep', '-t', metavar='SECONDS', dest='timestep', type=float, default=5.0)
  parser.add_argument('files', metavar='FILES', type=str, nargs='+', help='A list of files to parse.')
  args = parser.parse_args()

  i = 1
  results = []

  for filename in args.files:
    results = results + parse(filename, i, args.timestep)
    i += 1

  results.sort(key=lambda result: result['time'])

  minlat = 10000
  maxlat = -10000
  minlon = 10000
  maxlon = -10000

  for result in results:
    minlat = min(minlat, result['latitude'])
    maxlat = max(maxlat, result['latitude'])
    minlon = min(minlon, result['longitude'])
    maxlon = max(maxlon, result['longitude'])
    print(coord2str(result))

  print ("Longitude: %f to %f" % (minlon, maxlon), file=sys.stderr)
  print ("Latitude: %f to %f" % (minlat, maxlat), file=sys.stderr)

if __name__ == '__main__':
  main()
