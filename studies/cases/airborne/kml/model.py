# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

from math import sin, cos, atan2, degrees, radians, pi, asin

TIMESTEP = 5 # seconds
EARTH_RADIUS = 6371 # km

class Body(object):

    def __init__(self):
        self.position = [0,0,0]
        self.trace = []

    @property
    def lat(self):
        return self.position[0]

    @lat.setter
    def lat(self, value):
        self.position[0] = value

    @property
    def long(self):
        return self.position[1]

    @long.setter
    def long(self, value):
        self.position[1] = value

    @property
    def alt(self):
        return self.position[2]

    @alt.setter
    def alt(self, value):
        self.position[2] = value

    def relocateTo(self, pos):
        self.position = pos

    def moveTo(self, pos):
        self.trace.append(self.position)
        self.position = pos

    def toKML(self, template, output, name):
        template_str = open(template).read()
        data = template_str.replace('%%NAME%%', name)
        coords = []
        for pos in (self.trace + [self.position]):
            coord = [pos[1], pos[0], (pos[2] - EARTH_RADIUS)*1000]
            coords.append(','.join(map(str, coord)))
        data = data.replace('%%COORDINATES%%', '\n'.join(coords))
        with open(output, 'w') as out:
            out.write(data)

class Satellite(Body):

    def __init__(self):
        super(Satellite, self).__init__()

    @staticmethod
    def loadFromSTK(filename):
        sat = Satellite()
        with open(filename) as input:
            for (i, line) in enumerate(input.readlines()[6:]):
                pos = map(float, line.split()[4:7])
                if i == 0:
                    sat.relocateTo(pos)
                else:
                    sat.moveTo(pos)
        return sat

class Aircraft(Body):

    def __init__(self, speed, bearing, name):
        super(Aircraft, self).__init__()
        self.speed = float(speed) / (60*60)
        self.bearing = float(bearing)
        self.name = name

    def travel(self):
        distance = self.speed * TIMESTEP
        dist = distance / self.alt # Angular distance
        brng = radians(self.bearing)

        lat1 = radians(self.lat)
        lon1 = radians(self.long)

        lat2 = asin( sin(lat1) * cos(dist) +
                        cos(lat1) * sin(dist) *cos (brng) )
        lon2 = lon1 + atan2(sin(brng) * sin(dist) * cos(lat1),
                               cos(dist) - sin(lat1) * sin(lat2));

        dLon = lon1 - lon2
        y = sin(dLon) * cos(lat1)
        x = cos(lat2) * sin(lat1) - sin(lat2) * cos(lat1) * cos(dLon)
        brng = atan2(y, x)
        brng = (degrees(brng) + 180) % 360

        # normalize lon2
        lon2 = (lon2 + 3 * pi) % (2 * pi) - pi

        self.bearing = brng
        self.moveTo([degrees(lat2), degrees(lon2), self.alt])

def main():
    awac1 = Aircraft(2410, 0.1, 'awac1')
    awac1.position = [38.05667, -116.20222, float(12.19 + EARTH_RADIUS)]

    awac2 = Aircraft(2410, 180.1, 'awac2')
    awac2.position = [35.6, -115.9, float(12.19 + EARTH_RADIUS)]

    j1 = Aircraft(2410, 90.1, 'jet1')
    j1.position = [38.9, -112.672211, float(12.19 + EARTH_RADIUS)]

    j2 = Aircraft(2410, 270.1, 'jet2')
    j2.position = [38.72, -111.75, float(12.19 + EARTH_RADIUS)]

    j3 = Aircraft(2410, 90.1, 'jet3')
    j3.position = [35.0, -113.0, float(12.19 + EARTH_RADIUS)]

    j4 = Aircraft(2410, 270.1, 'jet4')
    j4.position = [34.8, -112.1, float(12.19 + EARTH_RADIUS)]

    aircraft = [awac1, awac2, j1, j2, j3, j4]

    ship = Body()
    ship.position = [36.8,-119.0, float(5.0 + EARTH_RADIUS)]
    ground = Body()
    ground.position = [36.8,-122.5, float(5.0 + EARTH_RADIUS)]
    nanosat = Body()
    nanosat.position = [36.8,-116.0, float(700.0 + EARTH_RADIUS)]

    movements = [24, 6] * 8
    for movement in movements:
        for x in xrange(0, movement):
            for air in aircraft:
                air.travel()

        for air in aircraft:
            air.bearing = (air.bearing + 90) % 360

    for air in aircraft:
        air.toKML('sample.kml', air.name + '.kml', air.name)

    ship.toKML('sample.kml', 'ship.kml', 'ship')
    ground.toKML('sample.kml', 'ground.kml', 'ground')
    nanosat.toKML('sample.kml', 'nanosat.kml', 'nanosat')

if __name__ == '__main__':
    main()

