#!/usr/bin/env python3

import os

import math

from pylaser import save, Polyline, Point, Group


class BoxEdge(Polyline):

    def __init__(self, start, end, depth, length, right=True, cut_ends=False, min_end_length=None):
        """ A box edge with cuts from start point to end point. Cuts of depth and (max)length.
        The edge can be right side (True) left side (False), cuts will go the other way. To start and
        end with cut, set cut_ends. Set min_end_length if needed will be depth by default. """

        min_end_length = depth or min_end_length

        x_distance = end.x - start.x
        y_distance = end.y - start.y
        distance = math.sqrt(x_distance ** 2 + y_distance ** 2)

        unit_x = x_distance / distance
        unit_y = y_distance / distance

        if right:
            cut_x = -unit_y
            cut_y = unit_x
        else:
            cut_x = unit_y
            cut_y = -unit_x

        # Want to get the cuts in the middle so the first and last step may be shorter. Find out the
        # number of cuts and the start and end distance. Also count needs to be uneven.

        count = int((distance - min_end_length * 2) // length)
        count -= 1 - count % 2
        end_length = (distance - length * count) // 2

        print("count", count, "end_length", end_length, "distance", distance, "length", length, "min_end_length", min_end_length)

        points = []
        ping = -1 if cut_ends else 1

        def add(l, x, y):
            points.append(Point(x, y))
            x += l * unit_x
            y += l * unit_y
            if ping:
                points.append(Point(x, y))
                x += depth * cut_x * ping
                y += depth * cut_y * ping
            return x, y

        x, y = start
        x, y = add(end_length, x, y)
        ping = -ping
        for _ in range(count):
            x, y = add(length, x, y)
            ping = -ping
        ping = 0
        add(end_length, x, y)
        points.append(end)

        super().__init__(*points)


be1 = BoxEdge(Point(0, 20), Point(200, 20), 20, 50, False, cut_ends=True)

# pl = Polyline(
#     Point(0, 100),
#     Point(0, 0),
#     Point(100, 0),
# )

g = Group(be1)

save('/tmp/ladda.svg', g)

os.system('firefox /tmp/ladda.svg')
