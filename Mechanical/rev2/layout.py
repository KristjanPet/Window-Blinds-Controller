"""Revision-2 mechanical layout: wall L bracket, NEMA 17 and 8:1 winding drive.

This is an adjustable fit prototype, not a print release. By default the gear
blanks show their complete rotation envelopes; --detailed builds prototype
herringbone teeth. Original wall/window coordinates are preserved. The old
holder and invalid Surface 8 are omitted from the new assembly.

From Mechanical/rev2, with blinds-cad active:
    python layout.py
    python layout.py --detailed
    python layout.py --check-only
    python layout.py --detailed --check-only --export output/layout.step
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass, replace
import hashlib
import json
from math import atan2, degrees, sqrt
from pathlib import Path

import cadquery as cq

ROOT = Path(__file__).resolve().parent
REFERENCE = ROOT / "references" / "reference_assembly.step"
REFERENCE_SHA256 = "4d4527a92f58adcbdecbcba8dd407cb208fe05319b24cb82187566fccf82b341"


@dataclass(frozen=True)
class Parameters:
    # Fixed CAD references, in mm.
    wall_x: float = 237.0053
    screw_y: float = -4.3652
    lower_screw_z: float = 35.8
    upper_screw_z: float = 128.8
    sill_z: float = 21.3
    window_bottom_z: float = 69.3
    back_wall_y: float = 10.9348
    back_wall_clearance: float = 2.0
    mounting_return_thickness: float = 6.0
    backplate_thickness: float = 6.0
    running_clearance: float = 1.0

    # Nominal motor envelope; actual shaft, connector and D-flat need checking.
    motor_size: float = 42.0
    motor_length: float = 40.0
    motor_shaft_d: float = 5.0
    motor_shaft_length: float = 24.0
    motor_face_y: float = -16.0
    motor_axis_z: float = 45.3

    # A single external gear pair, with the output above the motor.
    pinion_teeth: int = 20
    output_teeth: int = 160
    transverse_module: float = 0.75
    gear_width: float = 8.0
    helix_angle: float = 20.0
    backlash: float = 0.18

    # User's 25 mm cylinder / 25 mm surrounding wall: provisional interpretation.
    drum_core_d: float = 25.0
    flange_radial_height: float = 25.0  # Gives diameter 75 mm, NOT confirmed.
    winding_width: float = 16.0        # 15 mm belt + 0.5 mm clearance per side.
    flange_thickness: float = 2.0
    belt_entry_side: int = 1          # +X: wall side, opposite the first sketch model.

    # Two user-supplied 8 x 22 x 7 bearings seated in the rotating drum/gear.
    bearing_id: float = 8.0
    bearing_od: float = 22.0
    bearing_width: float = 7.0
    bearing_pocket_d: float = 22.2     # Test-print allowance, not a measured fit.
    axle_d: float = 8.0               # Fixed steel rod, clamped by the bracket.
    axle_hole_d: float = 8.1

    @property
    def back_y1(self):
        return self.back_wall_y - self.back_wall_clearance

    @property
    def back_y0(self):
        return self.back_y1 - self.backplate_thickness

    @property
    def gear_front_y(self):
        return self.back_y0 - self.running_clearance - self.gear_width

    @property
    def drive_axis_x(self):
        # Closest output-axis position with a full rotating gear envelope clear
        # of the wall return. The curved window also constrains the axial stack.
        radius = self.transverse_module * (self.output_teeth + 2) / 2
        return self.wall_x - self.mounting_return_thickness - self.running_clearance - radius

    @property
    def motor_axis_x(self):
        # Motor plate meets the inner face of the wall return; body stays clear.
        return self.wall_x - self.mounting_return_thickness - 24

    @property
    def centre_distance(self):
        return self.transverse_module * (self.pinion_teeth + self.output_teeth) / 2

    @property
    def output_z(self):
        dx = self.motor_axis_x - self.drive_axis_x
        if abs(dx) >= self.centre_distance:
            raise ValueError("The horizontal shaft separation exceeds the gear centre distance.")
        return self.motor_axis_z + sqrt(self.centre_distance**2 - dx**2)

    @property
    def flange_d(self):
        return self.drum_core_d + 2 * self.flange_radial_height

    @property
    def drum_back_y(self):
        return self.gear_front_y - 1

    @property
    def drum_front_y(self):
        return self.drum_back_y - 2 * self.flange_thickness - self.winding_width

    def validate(self):
        if self.output_teeth != 8 * self.pinion_teeth:
            raise ValueError("The required reduction is eight motor turns per drum turn.")
        if self.winding_width < 15 or self.drum_core_d <= self.bearing_pocket_d:
            raise ValueError("Keep room for the belt and the bearing pockets.")
        if self.flange_radial_height < 0 or self.gear_width <= 0:
            raise ValueError("Invalid flange or gear size.")
        if self.belt_entry_side not in (-1, 1):
            raise ValueError("Belt entry side must be -1 or +1.")
        if self.drum_back_y > self.gear_front_y:
            raise ValueError("Drum width reaches into the gear face; reposition the supports.")


@dataclass
class Part:
    name: str
    shape: cq.Shape
    color: str
    role: str


def box(x0, x1, y0, y1, z0, z1):
    return cq.Solid.makeBox(x1-x0, y1-y0, z1-z0, cq.Vector(x0, y0, z0))


def cy(radius, y0, y1, x, z):
    return cq.Solid.makeCylinder(radius, y1-y0, cq.Vector(x, y0, z), cq.Vector(0, 1, 0))


def cx(radius, x0, x1, y, z):
    return cq.Solid.makeCylinder(radius, x1-x0, cq.Vector(x0, y, z), cq.Vector(1, 0, 0))


def panel(points, y0, y1):
    # XZ's positive extrusion normal is -Y.
    return cq.Workplane("XZ", origin=(0, y1, 0)).polyline(points).close().extrude(y1-y0).val()


def clamp_cut(shape, x, z, y0, y1, p):
    shape = shape.cut(cy(p.axle_hole_d/2, y0-1, y1+1, x, z))
    shape = shape.cut(box(x-.5, x+.5, y0-1, y1+1, z, z+16))
    # Transverse M3 clamp bolt; nut/head access remains exposed from each side.
    return shape.cut(cx(1.7, x-16, x+16, (y0+y1)/2, z+9))


def build(p=Parameters(), detailed=False):
    p.validate()
    x, z, wall = p.drive_axis_x, p.output_z, p.wall_x
    mx, y0, y1 = p.motor_axis_x, p.back_y0, p.back_y1
    parts = []

    # L backplate with a wall return and ribs. Everything is suspended above sill.
    return_x = wall-p.mounting_return_thickness
    back = box(x-18, wall, y0, y1, 26.3, 50.3)
    back = back.fuse(box(wall-20, wall, y0, y1, 26.3, 138.8))
    back = back.fuse(box(return_x, wall, p.screw_y-14, p.screw_y+14, 26.3, 138.8))
    back = back.fuse(panel([(x+25, 49), (return_x, 49), (return_x, 122)], y0, y1))
    back = back.fuse(box(x-12, x+12, y0, y1, 40, z))
    back = back.fuse(panel([(x+10, 43), (return_x, 43), (x+10, z)], y0, y1))
    back = back.fuse(cy(14, y0, y1, x, z))
    back = clamp_cut(back, x, z, y0, y1, p)
    for hz in (p.lower_screw_z, p.upper_screw_z):
        back = back.cut(cx(2.25, wall-21, wall+1, p.screw_y, hz))
        back = back.cut(cx(5, wall-21, wall-3, p.screw_y, hz))
    # Motor shaft tip passes through the backplate without rubbing it.
    back = back.cut(cy(3, y0-1, y1+1, mx, p.motor_axis_z))

    # Face-mounted motor; its body has air above and below, with no shelf contact.
    mf = p.motor_face_y
    motor_plate = box(mx-24, mx+24, mf, mf+4,
                      p.motor_axis_z-21, p.motor_axis_z+21)
    motor_plate = motor_plate.cut(cy(11.2, mf-1, mf+5, mx, p.motor_axis_z))
    for dx in (-15.5, 15.5):
        for dz in (-15.5, 15.5):
            motor_plate = motor_plate.cut(cy(1.7, mf-1, mf+5, mx+dx, p.motor_axis_z+dz))
            motor_plate = motor_plate.cut(cy(3.2, mf+2, mf+5, mx+dx, p.motor_axis_z+dz))
    # Relieve the plate's upper inside corner for the conservative full roll.
    motor_plate = motor_plate.cut(cy(p.flange_d/2+p.running_clearance, mf-1, mf+5, x, z))
    # Supports stay below the pinion's complete rotation envelope.
    for dx in (-24, 18):
        motor_plate = motor_plate.fuse(box(mx+dx, mx+dx+6, mf+4, y0+1, 26.3, 34.3))
    back = back.fuse(motor_plate)

    # The former front fork cannot follow the drive toward the curved window.
    # A short steel axle is now supported from the reinforced rear bracket.
    # Its clamp, groove/retaining ring and loaded stiffness are still prototypes.
    parts.append(Part("Compact L wall bracket - fit prototype", back, "#27ad58", "bracket"))

    motor = box(mx-p.motor_size/2, mx+p.motor_size/2,
                p.motor_face_y-p.motor_length, p.motor_face_y,
                p.motor_axis_z-p.motor_size/2, p.motor_axis_z+p.motor_size/2)
    motor = motor.fuse(cy(11, p.motor_face_y, p.motor_face_y+2, mx, p.motor_axis_z))
    motor = motor.fuse(cy(p.motor_shaft_d/2, p.motor_face_y,
                         p.motor_face_y+p.motor_shaft_length, mx, p.motor_axis_z))
    parts.append(Part("NEMA 17 - nominal body and shaft", motor, "#df4141", "motor"))

    def gear(teeth, gx, hz, hand, phase):
        if detailed:
            from gears import herringbone
            print(f"Building {teeth}-tooth herringbone gear...", flush=True)
            s = herringbone(teeth, p.transverse_module, p.gear_width,
                            hand*p.helix_angle, p.backlash, phase)
            return s.rotate((0, 0, 0), (1, 0, 0), -90).translate((gx, p.gear_front_y, hz))
        return cy(p.transverse_module*(teeth+2)/2, p.gear_front_y,
                  p.gear_front_y+p.gear_width, gx, hz)

    # In the profile XY plane, +Y becomes -Z when the gear axis is turned to Y.
    phase = degrees(atan2(-(z-p.motor_axis_z), x-mx))
    pinion = gear(p.pinion_teeth, mx, p.motor_axis_z, 1, phase)
    pinion = pinion.cut(cy((p.motor_shaft_d+.1)/2, mf-1, y1+1, mx, p.motor_axis_z))
    suffix = "prototype teeth" if detailed else "rotation envelope, no teeth"
    parts.append(Part(f"20T motor pinion - {suffix}", pinion, "#ce3030", "pinion"))

    rotor = gear(p.output_teeth, x, z, -1, phase+180+180/p.output_teeth)
    yf = p.drum_front_y
    ya = yf+p.flange_thickness
    yb = ya+p.winding_width
    ye = p.drum_back_y
    rotor = rotor.fuse(cy(p.flange_d/2, yf, ya, x, z))
    rotor = rotor.fuse(cy(p.drum_core_d/2, ya, yb, x, z))
    rotor = rotor.fuse(cy(p.flange_d/2, yb, ye, x, z))
    rotor = rotor.fuse(cy(p.drum_core_d/2, ye, p.gear_front_y+.2, x, z))
    # The bearings turn with the rotor on the fixed axle. End spacers contact
    # only their inner rings; the retaining hardware must not clamp the rotor.
    rotor = rotor.cut(cy((p.axle_d+.6)/2, yf-1, 10, x, z))
    bearing_starts = (yf+.5, p.gear_front_y+p.gear_width-.5-p.bearing_width)
    rotor = rotor.cut(cy(p.bearing_pocket_d/2, yf-1,
                        bearing_starts[0]+p.bearing_width+.2, x, z))
    rotor = rotor.cut(cy(p.bearing_pocket_d/2, bearing_starts[1]-.2,
                        p.gear_front_y+p.gear_width+1, x, z))
    parts.append(Part(f"160T output gear and drum - {suffix}", rotor, "#ed982f", "rotor"))
    for i, start in enumerate(bearing_starts, 1):
        bearing = cy(p.bearing_od/2, start, start+p.bearing_width, x, z)
        bearing = bearing.cut(cy(p.bearing_id/2, start-1, start+p.bearing_width+1, x, z))
        parts.append(Part(f"608 bearing {i} - 8 x 22 x 7 envelope", bearing, "#9da9b4", "bearing"))
    axle_start = yf-3
    groove_start, groove_end = yf-2, yf-1
    axle = cy(4, axle_start, y1, x, z)
    groove = cy(4.2, groove_start, groove_end, x, z).cut(cy(3.7, groove_start-1, groove_end+1, x, z))
    axle = axle.cut(groove)
    parts.append(Part(f"Fixed 8 mm steel axle - {y1-axle_start:.1f} mm, prototype groove", axle, "#526271", "axle"))
    ring = cy(6, groove_start, groove_end, x, z).cut(cy(3.7, groove_start-1, groove_end+1, x, z))
    ring = ring.cut(box(x, x+7, groove_start-1, groove_end+1, z-1.5, z+1.5))
    parts.append(Part("Axle retaining ring - provisional hardware envelope", ring, "#667f86", "retainer"))
    for label, a, b in [("front", groove_end, bearing_starts[0]),
                        ("rear", bearing_starts[1]+p.bearing_width, y0)]:
        if b <= a:
            raise ValueError("The rotor reaches an axle support; adjust axial spacing.")
        spacer = cy(5.5, a, b, x, z).cut(cy(4.1, a-1, b+1, x, z))
        parts.append(Part(f"{label} inner-ring spacer - length {b-a:.1f} mm", spacer, "#667f86", "spacer"))

    # Blue route is illustrative: a vertical belt meeting an intermediate roll.
    # It is not a measured entry coordinate or verified winding simulation.
    illustrative_roll_r = (p.drum_core_d/2+p.flange_d/2)/2
    belt_x = x+p.belt_entry_side*illustrative_roll_r
    belt = box(belt_x-.25, belt_x+.25, ya+.5, ya+15.5, z, 190)
    side = "wall side" if p.belt_entry_side == 1 else "away from wall"
    parts.append(Part(f"Belt entry on {side} - illustrative, 15 mm wide", belt, "#189ee1", "route"))
    partial_roll = cy(illustrative_roll_r, ya+.5, ya+15.5, x, z)
    partial_roll = partial_roll.cut(cy(p.drum_core_d/2, ya, ya+16, x, z))
    parts.append(Part("Partial belt roll - illustrative diameter, capacity unverified",
                      partial_roll, "#189ee1", "route"))
    return parts


def references(path):
    if not path.is_file():
        raise ValueError(f"Missing reference: {path}. Keep your existing reference_assembly.step in references/.")
    if hashlib.sha256(path.read_bytes()).hexdigest() != REFERENCE_SHA256:
        raise ValueError("Reference changed: re-identify Wall and Window before using these coordinates.")
    roots = cq.importers.importStep(str(path)).vals()
    if len(roots) != 4 or not all(s.isValid() for s in roots[:3]):
        raise ValueError("Unexpected reference geometry.")
    return roots[0], roots[1]


def checks(parts, p, wall, window):
    """Static solid checks plus full rotation envelopes against fixed obstacles.

    Gear meshing, bearing races, clamps and torque transmission are NOT proved
    by these checks. Intentional mounting contact and motor-face contact are
    allowed. The belt route is only illustrative and is excluded.
    """
    results = []
    ok = True
    for part in parts:
        if part.role == "route":
            continue
        valid = part.shape.isValid() and len(part.shape.Solids()) == 1 and part.shape.Volume() > 0
        wv = part.shape.intersect(wall).Volume()
        kv = part.shape.intersect(window).Volume()
        d = part.shape.distance(window)
        good = valid and wv < 1e-5 and kv < 1e-5
        ok &= good
        results.append(dict(part=part.name, valid=valid,
                            wall_overlap_mm3=round(wv, 6),
                            window_overlap_mm3=round(kv, 6),
                            window_distance_mm=round(d, 4)))
    # Conservative blank cylinders contain every tooth position, every flange
    # position and the provisional fully wound roll between the flanges.
    envelopes = build(p, detailed=False)
    fixed_parts = [part for part in parts if part.role in ("bracket", "motor", "axle", "spacer", "retainer")]
    for moving in [q for q in envelopes if q.role in ("pinion", "rotor")]:
        if moving.role == "rotor":
            roll = cy(p.flange_d/2, p.drum_front_y+p.flange_thickness,
                      p.drum_back_y-p.flange_thickness, p.drive_axis_x, p.output_z)
            roll = roll.cut(cy(p.drum_core_d/2, p.drum_front_y-1,
                              p.drum_back_y+1, p.drive_axis_x, p.output_z))
            envelope = moving.shape.fuse(roll)
        else:
            envelope = moving.shape
        for fixed_name, fixed in [(b.name, b.shape) for b in fixed_parts]+[("Wall", wall), ("Window", window)]:
            overlap = envelope.intersect(fixed).Volume()
            distance = envelope.distance(fixed)
            ok &= overlap < 1e-5
            results.append(dict(rotation_envelope=moving.role, obstacle=fixed_name,
                                overlap_mm3=round(overlap, 6), distance_mm=round(distance, 4)))
    return bool(ok), results


def mesh_check(parts, p):
    """Sample one pinion tooth period at nine correctly coupled rotations.

    Detects interference in the prototype teeth, not contact stress, noise or
    manufacturing error. Positive backlash means the unloaded teeth can have
    a small gap; a physical pair is still required before a print release.
    """
    pinion = next(q.shape for q in parts if q.role == "pinion")
    rotor = next(q.shape for q in parts if q.role == "rotor")
    x = p.drive_axis_x
    result = []
    for i in range(9):
        angle = 360/p.pinion_teeth*i/8
        small = pinion.rotate((p.motor_axis_x, 0, p.motor_axis_z), (p.motor_axis_x, 1, p.motor_axis_z), angle)
        large = rotor.rotate((x, 0, p.output_z), (x, 1, p.output_z), -angle/8)
        volume = small.intersect(large).Volume()
        result.append(dict(pinion_degrees=angle, output_degrees=-angle/8,
                           interference_mm3=round(volume, 7)))
    return all(r["interference_mm3"] < 1e-5 for r in result), result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=REFERENCE)
    parser.add_argument("--detailed", action="store_true", help="Build prototype herringbone teeth (slower).")
    parser.add_argument("--check-only", action="store_true", help="No viewer; validate prototype clearances.")
    parser.add_argument("--check-mesh", action="store_true", help="With --detailed, sample tooth interference over one pitch.")
    parser.add_argument("--export", type=Path, help="Write a named STEP assembly for CAD review, not a print release.")
    parser.add_argument("--report", type=Path, help="Write the geometry-check report as JSON.")
    parser.add_argument("--parts-only", action="store_true", help="Hide wall and window in the viewer/export.")
    parser.add_argument("--core-diameter", type=float, default=25)
    parser.add_argument("--flange-height", type=float, default=25,
                        help="Provisional RADIAL wall height above the core; default gives flange diameter 75.")
    parser.add_argument("--port", type=int)
    args = parser.parse_args()
    if args.check_mesh and not args.detailed:
        parser.error("--check-mesh requires --detailed; blank rotation envelopes intentionally overlap.")
    p = replace(Parameters(), drum_core_d=args.core_diameter, flange_radial_height=args.flange_height)
    try:
        wall, window = references(args.reference.expanduser().resolve())
        parts = build(p, args.detailed)
        good, results = checks(parts, p, wall, window)
    except (ValueError, RuntimeError) as exc:
        print(f"Layout failed: {exc}")
        return 2
    print(f"Prototype: {p.pinion_teeth}:{p.output_teeth} teeth; 8:1 reduction; centre distance {p.centre_distance:.2f} mm.")
    print(f"Drum core {p.drum_core_d:.1f} mm diameter; winding width {p.winding_width:.1f} mm; provisional flange diameter {p.flange_d:.1f} mm.")
    entry = "wall side (+X)" if p.belt_entry_side == 1 else "side away from the wall (-X)"
    print(f"Axes X: drum {p.drive_axis_x:.3f}, motor {p.motor_axis_x:.3f}; belt enters on the {entry}.")
    print("Old holder removed. Surface 8 omitted. PCB and motor connector have not been placed.")
    print("Geometry/rotation-envelope checks:", "PASS" if good else "FAIL")
    for r in results:
        if "rotation_envelope" in r:
            print(f"  {r['rotation_envelope']} vs {r['obstacle']}: overlap {r['overlap_mm3']:.6f} mm^3, gap {r['distance_mm']:.3f} mm")
        elif not r["valid"] or r["wall_overlap_mm3"] or r["window_overlap_mm3"]:
            print(" ", r)
    print("Fit prototype only: fasteners, pinion attachment, bearing fits, belt capacity and loaded operation need verification.")
    report = dict(status="fit prototype, not a print release", passed=good,
                  detailed_teeth=args.detailed, parameters=p.__dict__, checks=results,
                  placement=dict(drum_axis_x=p.drive_axis_x, drum_axis_z=p.output_z,
                                 motor_axis_x=p.motor_axis_x, motor_axis_z=p.motor_axis_z,
                                 gear_front_y=p.gear_front_y, drum_front_y=p.drum_front_y,
                                 wall_return_to_gear_envelope=p.running_clearance,
                                 backplate_to_back_wall=p.back_wall_clearance))
    if args.check_mesh:
        mesh_ok, mesh_results = mesh_check(parts, p)
        good &= mesh_ok
        report.update(passed=good, mesh_samples=mesh_results)
        print("Sampled tooth-interference check:", "PASS" if mesh_ok else "FAIL")
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    if not good:
        return 2
    if args.export:
        assembly = cq.Assembly(name="Revision 2 - fit prototype")
        for part in parts:
            assembly.add(part.shape, name=part.name.replace("/", "-"), color=cq.Color(part.color))
        if not args.parts_only:
            assembly.add(wall, name="Wall and sill - fixed", color=cq.Color(.58, .58, .58, .25))
            assembly.add(window, name="Window opening - keep clear", color=cq.Color(.55, .65, .72, .20))
        args.export.parent.mkdir(parents=True, exist_ok=True)
        assembly.export(str(args.export))
        print("STEP written:", args.export)
    if not args.check_only:
        from ocp_vscode import show
        objects = [q.shape for q in parts]
        names = [q.name for q in parts]
        colors = [q.color for q in parts]
        alphas = [1.0]*len(parts)
        if not args.parts_only:
            objects += [wall, window]
            names += ["Wall and sill - fixed", "Window opening - keep clear"]
            colors += ["#928a82", "#919faa"]
            alphas += [.15, .22]
        show(*objects, names=names, colors=colors, alphas=alphas, port=args.port,
             axes=True, grid=True, up="Z")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
