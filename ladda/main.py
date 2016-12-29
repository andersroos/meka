#!/usr/bin/env python3

import os

from pylaser import write, Point, BoxEdge, Group, Polyline, Circle

PLY_THICKNESS = 5.9

SLIDE_LENGTH = 250

CUT_LENGTH = 40

INNER_BOX_WIDTH = 0
INNER_BOX_HEIGHT = 0
INNER_BOX_DEPTH = 0

OUTER_BOX_WIDTH = 380
OUTER_BOX_HEIGHT = 180
OUTER_BOX_DEPTH = SLIDE_LENGTH + PLY_THICKNESS

AXIS_SEG_SIZE = 100

TRI_BASE = 20

def get_axis(rel=Point(0, 0)):
    g = Group(rel=rel)
    for x in range(0, 6 * AXIS_SEG_SIZE, 2 * AXIS_SEG_SIZE):
        g.append(Polyline(Point(x, 0), Point(x + AXIS_SEG_SIZE, 0)))
    for y in range(0, 6 * AXIS_SEG_SIZE, 2 * AXIS_SEG_SIZE):
        g.append(Polyline(Point(0, y), Point(0, y + AXIS_SEG_SIZE)))
    return g

def get_outer_box_top(rel=Point(0, 0)):
    # Outer box top seen from the front.

    g = Group(rel=rel)
    # Front edge.
    g.append(BoxEdge(Point(0, 0), Point(OUTER_BOX_WIDTH, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    # Side edges.
    g.append(BoxEdge(Point(0, 0), Point(0, OUTER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=False, cut_end=False))

    g.append(BoxEdge(Point(OUTER_BOX_WIDTH, 0), Point(OUTER_BOX_WIDTH, OUTER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    # Back edge.
    g.append(Polyline(Point(0, OUTER_BOX_DEPTH), Point(OUTER_BOX_WIDTH, OUTER_BOX_DEPTH)))

    # Vent.
    def triangle(x, y, top_up, min_x, max_x):
        if top_up:
            uy = 1
        else:
            y += TRI_BASE
            uy = -1

        if x < min_x - TRI_BASE * 2:
            return None

        if x < min_x - TRI_BASE:
            return Polyline(
                Point(min_x, y),
                Point(min_x, y + uy * (x + TRI_BASE * 2 - min_x)),
                Point(x + TRI_BASE * 2, y),
                Point(min_x, y),
            )

        if x < min_x:
            return Polyline(
                Point(min_x, y),
                Point(min_x, y + uy * (min_x - x)),
                Point(x + TRI_BASE, y + uy * TRI_BASE),
                Point(x + TRI_BASE * 2, y),
                Point(min_x, y),
            )

        if x <= max_x - TRI_BASE * 2:
            return Polyline(
                Point(x, y),
                Point(x + TRI_BASE, y + TRI_BASE * uy),
                Point(x + 2 * TRI_BASE, y),
                Point(x, y),
            )

        if x <= max_x - TRI_BASE:
            return Polyline(
                Point(x, y),
                Point(x + TRI_BASE, y + uy * TRI_BASE),
                Point(max_x, y + uy * (x + TRI_BASE * 2 - max_x)),
                Point(max_x, y),
                Point(x, y),
            )

        if x < max_x:
            return Polyline(
                Point(x, y),
                Point(max_x, y + uy * (max_x - x)),
                Point(max_x, y),
                Point(x, y),
            )

        return None

    top_up = True
    for x in range(-TRI_BASE * 2, int(OUTER_BOX_WIDTH), int(TRI_BASE * 1.5)):
        for y in (60, 120, 180):
            g.append(triangle(x, y, top_up, PLY_THICKNESS * 2, OUTER_BOX_WIDTH - PLY_THICKNESS * 2))
        top_up = not top_up

    return g


def get_outer_box_inner_top(rel=Point(0, 0)):
    # Outer box innter top seen from the front. To be glued on inner top
    g = Group(rel=rel)
    g.append(Polyline(
        Point(PLY_THICKNESS, PLY_THICKNESS),
        Point(OUTER_BOX_WIDTH - PLY_THICKNESS, PLY_THICKNESS),
        Point(OUTER_BOX_WIDTH - PLY_THICKNESS, OUTER_BOX_DEPTH),
        Point(PLY_THICKNESS, OUTER_BOX_DEPTH),
        Point(PLY_THICKNESS, PLY_THICKNESS)
    ))
    return g


group = Group(
    get_outer_box_top(),
    get_outer_box_inner_top(),
    get_axis(rel=Point(-100, -100)), rel=Point(120, 120))

write('/tmp/ladda.svg', group)
write('/tmp/ladda.dxf', group)

os.system('firefox /tmp/ladda.svg')
