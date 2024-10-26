#!/usr/bin/env python

import os

from pylaser import write, Point, BoxEdge, Group, Polyline, Circle, Arc, Rect


PLY_THICKNESS = 5.9

SLIDER_DEPTH = 250
SLIDER_WIDTH = 9.8 + 0.2

CUT_LENGTH = 40

OUTER_BOX_WIDTH = 380
OUTER_BOX_HEIGHT = 180
OUTER_BOX_DEPTH = SLIDER_DEPTH + PLY_THICKNESS
OUTER_BOX_PANEL_EDGE = PLY_THICKNESS * 4
OUTER_BOX_PANEL_INNER_HOLE_MARGIN = PLY_THICKNESS * 2

INNER_BOX_WIDTH = OUTER_BOX_WIDTH - PLY_THICKNESS * 2 - SLIDER_WIDTH * 2
INNER_BOX_HEIGHT = OUTER_BOX_HEIGHT - PLY_THICKNESS * 3
INNER_BOX_DEPTH = 180

AXIS_SEG_SIZE = 100

TRI_BASE = 18
TRI_STEP = 28
TRI_EDGE = 22


def main():
    width = 457.2 + 20
    height = 609.6 + 20
    width_mid = width / 2
    radius = 10
    
    play_overlay = Group(
        Polyline(Point(radius, 0), Point(width - radius, 0)),
        Arc(Point(width - radius, radius), radius, 90, 180),
        Polyline(Point(width, radius), Point(width, height - radius)),
        Arc(Point(width - radius, height - radius), radius, 0, 90),
        Polyline(Point(width - radius, height), Point(radius, height)),
        Arc(Point(radius, height - radius), radius, 270, 0),
        Polyline(Point(0, height - radius), Point(0, radius)),
        Arc(Point(radius, radius), radius, 180, 270),
        rel=Point(10, 10),
    )
    play_overlay.append(Circle(Point(width_mid - 160, 30), 8))
    play_overlay.append(Circle(Point(width_mid + 160, 30), 8))

    test_y = height + 40
    test_w = 240
    play_overlay.append(Rect(Point(0, test_y - 20), Point(test_w, test_y + 20)))
    for i, side in enumerate((7.5, 8.0, 8.5, 9.0, 9.5, 10.0)):
        half = side/2
        x = 20 + i * 16
        play_overlay.append(Rect(Point(x - half, test_y - half), Point(x + half, test_y + half)))

    for i, dia in enumerate((12.5, 13.0, 13.5, 14.0, 14.5, 15.0)):
        half = dia/2
        x = test_w - 16 - i * 20
        play_overlay.append(Circle(Point(x, test_y), half))

    write('/tmp/aos.svg', play_overlay)
    write('/tmp/aos.dxf', play_overlay)
    
    os.system('google-chrome /tmp/aos.svg')


def tri(x, y, top_up, min_x, max_x, tri_base):
    if top_up:
        uy = 1
    else:
        y += tri_base
        uy = -1

    if x < min_x - tri_base * 2:
        return None

    if x < min_x - tri_base:
        return Polyline(
            Point(min_x, y),
            Point(min_x, y + uy * (x + tri_base * 2 - min_x)),
            Point(x + tri_base * 2, y),
            Point(min_x, y),
        )

    if x < min_x:
        return Polyline(
            Point(min_x, y),
            Point(min_x, y + uy * (min_x - x)),
            Point(x + tri_base, y + uy * tri_base),
            Point(x + tri_base * 2, y),
            Point(min_x, y),
        )

    if x <= max_x - tri_base * 2:
        return Polyline(
            Point(x, y),
            Point(x + tri_base, y + tri_base * uy),
            Point(x + 2 * tri_base, y),
            Point(x, y),
        )

    if x <= max_x - tri_base:
        return Polyline(
            Point(x, y),
            Point(x + tri_base, y + uy * tri_base),
            Point(max_x, y + uy * (x + tri_base * 2 - max_x)),
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

def get_axis(rel=Point(0, 0)):
    g = Group(rel=rel)
    for x in range(0, 6 * AXIS_SEG_SIZE, 2 * AXIS_SEG_SIZE):
        g.append(Polyline(Point(x, 0), Point(x + AXIS_SEG_SIZE, 0)))
    for y in range(0, 6 * AXIS_SEG_SIZE, 2 * AXIS_SEG_SIZE):
        g.append(Polyline(Point(0, y), Point(0, y + AXIS_SEG_SIZE)))
    return g

def get_outer_box_top(rel=Point(0, 0)):
    # Outer box top y=0 at the front.

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
    for x in range(int(OUTER_BOX_WIDTH / 5), int(OUTER_BOX_WIDTH), int(OUTER_BOX_WIDTH / 5)):
        g.append(tri(x - 10, OUTER_BOX_DEPTH - 6, True, 0, OUTER_BOX_WIDTH, 8))

    # Vents.
    top_up = True
    for x in range(int(TRI_EDGE - TRI_BASE), int(OUTER_BOX_WIDTH), TRI_STEP):
        for y in (60, 120, 180):
            g.append(tri(x, y, top_up, TRI_EDGE, OUTER_BOX_WIDTH - TRI_EDGE, TRI_BASE))
        top_up = not top_up

    return g


def get_outer_box_bottom(rel=Point(0, 0)):
    # y=0 at the front.

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

    # Back edge / hole.
    edge = PLY_THICKNESS * 3
    g.append(Polyline(
        Point(0, OUTER_BOX_DEPTH),
        Point(edge, OUTER_BOX_DEPTH),
        Point(edge, edge),
        Point(OUTER_BOX_WIDTH - edge, edge),
        Point(OUTER_BOX_WIDTH - edge, OUTER_BOX_DEPTH),
        Point(OUTER_BOX_WIDTH, OUTER_BOX_DEPTH),
    ))

    return g


def get_outer_box_front(rel=Point(0, 0)):
    # Outer box front y=0 at bottom.
    g = Group(rel=rel)

    # Top edge.
    g.append(BoxEdge(Point(0, OUTER_BOX_HEIGHT), Point(OUTER_BOX_WIDTH, OUTER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=True, cut_end=True))

    # Side edges.
    g.append(BoxEdge(Point(0, 0), Point(0, OUTER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=False, cut_end=False))

    g.append(BoxEdge(Point(OUTER_BOX_WIDTH, 0), Point(OUTER_BOX_WIDTH, OUTER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    # Bottom edge.
    g.append(BoxEdge(Point(0, 0), Point(OUTER_BOX_WIDTH, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))

    # Panel hole.
    g.append(Polyline(
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_HEIGHT - OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_WIDTH - OUTER_BOX_PANEL_EDGE, OUTER_BOX_HEIGHT - OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_WIDTH - OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
    ))

    return g

def get_outer_box_panel_rim(rel=Point(0, 0)):
    g = Group(rel=rel)

    edge = OUTER_BOX_PANEL_EDGE + OUTER_BOX_PANEL_INNER_HOLE_MARGIN

    dent_height = OUTER_BOX_HEIGHT * 0.5
    dent_width = SLIDER_WIDTH * 1.2

    # Outer edge.
    g.append(Polyline(
        Point(PLY_THICKNESS, PLY_THICKNESS),
        Point(PLY_THICKNESS, OUTER_BOX_HEIGHT - PLY_THICKNESS - dent_height),
        Point(PLY_THICKNESS + dent_width, OUTER_BOX_HEIGHT - PLY_THICKNESS - dent_height),
        Point(PLY_THICKNESS + dent_width, OUTER_BOX_HEIGHT - PLY_THICKNESS),

        Point(OUTER_BOX_WIDTH - PLY_THICKNESS - dent_width, OUTER_BOX_HEIGHT - PLY_THICKNESS),
        Point(OUTER_BOX_WIDTH - PLY_THICKNESS - dent_width, OUTER_BOX_HEIGHT - PLY_THICKNESS - dent_height),
        Point(OUTER_BOX_WIDTH - PLY_THICKNESS, OUTER_BOX_HEIGHT - PLY_THICKNESS - dent_height),

        Point(OUTER_BOX_WIDTH - PLY_THICKNESS, PLY_THICKNESS),
        Point(PLY_THICKNESS, PLY_THICKNESS),
    ))

    # Inner hole.
    g.append(Polyline(
        Point(edge, edge),
        Point(edge, OUTER_BOX_HEIGHT - edge),
        Point(OUTER_BOX_WIDTH - edge, OUTER_BOX_HEIGHT - edge),
        Point(OUTER_BOX_WIDTH - edge, edge),
        Point(edge, edge),
    ))

    return g


def get_outer_box_empty_panel(rel=Point(0, 0)):

    # Panel front
    g1 = Group()
    g1.append(Polyline(
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_HEIGHT - OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_WIDTH - OUTER_BOX_PANEL_EDGE, OUTER_BOX_HEIGHT - OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_WIDTH - OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
        Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE),
    ))
    g1.append(Circle(Point(OUTER_BOX_PANEL_EDGE, OUTER_BOX_PANEL_EDGE), 4))

    # Panel inside.
    edge = OUTER_BOX_PANEL_EDGE + OUTER_BOX_PANEL_INNER_HOLE_MARGIN
    g2 = Group(rel=Point(0, -OUTER_BOX_HEIGHT))
    g2.append(Polyline(
        Point(edge, edge),
        Point(edge, OUTER_BOX_HEIGHT - edge),
        Point(OUTER_BOX_WIDTH - edge, OUTER_BOX_HEIGHT - edge),
        Point(OUTER_BOX_WIDTH - edge, edge),
        Point(edge, edge),
    ))

    return Group(g1, g2, rel=rel)

def get_outer_box_pinnar(rel=Point(0, 0)):

    g = Group(rel=rel)

    height = OUTER_BOX_DEPTH - PLY_THICKNESS
    width = 30

    for x in range(0, width * 5, width):
        g.append(Polyline(Point(x, 0), Point(x, height)))

    g.append(Polyline(Point(0, 0), Point(width * 4, 0)))
    g.append(Polyline(Point(0, height), Point(width * 4, height)))

    return g


def get_outer_box_side(rel=Point(0, 0)):
    # Outer box side, y=0 at front edge, x=0 at top edge, right side positive x, left side negative x.
    g = Group(rel=rel)

    # Top edge.
    g.append(BoxEdge(Point(0, 0), Point(0, OUTER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=True, cut_end=True))

    # Back edge
    g.append(Polyline(Point(0, OUTER_BOX_DEPTH), Point(OUTER_BOX_HEIGHT, OUTER_BOX_DEPTH)))

    # Front edge.
    g.append(BoxEdge(Point(0, 0), Point(OUTER_BOX_HEIGHT, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))

    # Bottom edge.
    g.append(BoxEdge(Point(OUTER_BOX_HEIGHT, 0), Point(OUTER_BOX_HEIGHT, OUTER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))

    return g


def get_slider_marker(rel=Point(0, 0)):
    # y=0 is front
    return Group(
        Polyline(
            Point(0, 0),
            Point(0, SLIDER_DEPTH),
            Point(SLIDER_WIDTH, SLIDER_DEPTH),
            Point(SLIDER_WIDTH, 0),
            Point(0, 0),
            Point(SLIDER_WIDTH, SLIDER_DEPTH),
            Point(SLIDER_WIDTH, 0),
            Point(0, SLIDER_DEPTH),
        ),
        rel=rel)


def get_inner_box_bottom(rel=Point(0, 0)):
    # y=0 at front
    g = Group(rel=rel)

    # Front edge
    g.append(BoxEdge(Point(0, 0), Point(INNER_BOX_WIDTH, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))
    # Back edge
    g.append(BoxEdge(Point(0, INNER_BOX_DEPTH), Point(INNER_BOX_WIDTH, INNER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=True, cut_end=True))

    # Hole for cable.
    g.append(Circle(Point(INNER_BOX_WIDTH / 2, INNER_BOX_DEPTH - 5 - PLY_THICKNESS), 4))

    # Sides
    g.append(BoxEdge(Point(0, 0), Point(0, INNER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=False, cut_end=False))
    g.append(BoxEdge(Point(INNER_BOX_WIDTH, 0), Point(INNER_BOX_WIDTH, INNER_BOX_DEPTH),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    # Vents.
    top_up = True
    for x in range(11, int(INNER_BOX_WIDTH), 41):
        for y in range(30, 140, 33):
            g.append(tri(x, y, top_up, 32, INNER_BOX_WIDTH - 32, 20))
        top_up = not top_up

    return g

def get_inner_box_front_back(rel=Point(0, 0)):
    # y=0 at bottom
    g = Group(rel=rel)

    # Top edge.
    g.append(Polyline(Point(0, INNER_BOX_HEIGHT), Point(INNER_BOX_WIDTH, INNER_BOX_HEIGHT)))

    # Edges.
    g.append(BoxEdge(Point(0, 0), Point(0, INNER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=False, cut_end=False))
    g.append(BoxEdge(Point(INNER_BOX_WIDTH, 0), Point(INNER_BOX_WIDTH, INNER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    # Bottom edge.
    g.append(BoxEdge(Point(0, 0), Point(INNER_BOX_WIDTH, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=False, cut_end=False))

    return g


def get_inner_box_side(rel=Point(0, 0)):
    # y=0 at bottom
    g = Group(rel=rel)

    # Top edge.
    g.append(Polyline(Point(0, INNER_BOX_HEIGHT), Point(INNER_BOX_DEPTH, INNER_BOX_HEIGHT)))

    # Edges.
    g.append(BoxEdge(Point(0, 0), Point(0, INNER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=False, cut_start=True, cut_end=True))
    g.append(BoxEdge(Point(INNER_BOX_DEPTH, 0), Point(INNER_BOX_DEPTH, INNER_BOX_HEIGHT),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))

    # Bottom edge.
    g.append(BoxEdge(Point(0, 0), Point(INNER_BOX_DEPTH, 0),
                     depth=PLY_THICKNESS, length=CUT_LENGTH,
                     right=True, cut_start=True, cut_end=True))

    return g


if __name__ == '__main__':
    main()

