"""Involute gear geometry for a revision-2 fit study (not rated production gears).

Module and pressure angle are specified in the transverse plane. Opposite
helix hands are required for the two parallel-axis gears. Tooth root relief,
backlash and the printed tooth finish still require a physical mesh trial.
"""
from math import acos, cos, degrees, pi, radians, sin, tan

import cadquery as cq


def profile(teeth, module, backlash=0.18, pressure_angle=20.0):
    """Closed involute outline, using spline flanks and circular tip/root arcs.

    Each gear loses backlash/2 from its pitch-circle tooth thickness, so the
    mating pair has the specified total nominal tangential backlash.
    """
    if teeth < 20 or module <= 0 or backlash < 0:
        raise ValueError("This unshifted prototype profile requires >=20 teeth.")
    rp = module * teeth / 2
    rb = rp * cos(radians(pressure_angle))
    rr = rp - 1.25 * module
    ra = rp + module
    start = max(rb, rr)
    inv_pitch = tan(radians(pressure_angle)) - radians(pressure_angle)
    half_pitch = pi / (2 * teeth) - backlash / (4 * rp)

    def half(r):
        a = acos(min(1.0, rb / r))
        return half_pitch + inv_pitch - (tan(a) - a)

    def v(r, a):
        return cq.Vector(r * cos(a), r * sin(a), 0)

    def arc(r, a, b):
        return cq.Edge.makeThreePointArc(v(r, a), v(r, (a + b) / 2), v(r, b))

    edges = []
    for tooth in range(teeth):
        a = 2 * pi * tooth / teeth
        hs, ht = half(start), half(ra)
        if ht <= 0 or 2 * hs >= 2 * pi / teeth:
            raise ValueError("Backlash/module combination produces a degenerate tooth.")
        if rr < start - 1e-8:
            edges.append(cq.Edge.makeLine(v(rr, a - hs), v(start, a - hs)))
        radii = [start + (ra - start) * i / 10 for i in range(11)]
        edges.append(cq.Edge.makeSpline([v(r, a - half(r)) for r in radii]))
        edges.append(arc(ra, a - ht, a + ht))
        edges.append(cq.Edge.makeSpline([v(r, a + half(r)) for r in reversed(radii)]))
        if rr < start - 1e-8:
            edges.append(cq.Edge.makeLine(v(start, a + hs), v(rr, a + hs)))
        edges.append(arc(rr, a + hs, a + 2 * pi / teeth - hs))
    return cq.Wire.assembleEdges(edges)


def herringbone(teeth, module, width, helix_angle, backlash=0.18, phase=0.0):
    """Make an unbored gear along +Z, with opposing helices meeting at mid-face."""
    outline = profile(teeth, module, backlash)
    half_width = width / 2
    twist = degrees(half_width * tan(radians(helix_angle)) / (module * teeth / 2))
    lower = cq.Workplane("XY").newObject([outline]).toPending().twistExtrude(
        half_width, twist, combine=False, clean=False
    ).val()
    upper = lower.mirror("XY", (0, 0, half_width))
    return lower.fuse(upper).rotate((0, 0, 0), (0, 0, 1), phase)
