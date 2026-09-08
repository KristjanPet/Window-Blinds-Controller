# Revision 2: agreed design brief

## Goal

Redesign the custom mechanics of the Window Blinds Controller for wall
mounting, with the motor between the sill and the window opening. Suspend the
assembly from the wall, preferably touching neither the sill nor the window.
Use a new L-shaped wall holder to support the complete mechanism and current
PCB/enclosure. Make the custom structural parts and enclosure suitable for
3D printing.

## Fixed references from the user

- Existing stepper motor. Repository hardware documentation identifies it as
  NEMA 17, model 17HS4401. Keep that motor; establish its actual mounting and
  shaft dimensions from the supplied CAD or verified drawing.
- Existing wall/window geometry and its physical installation constraints.
- Current PCB for now. Keep its dimensions, mounting features, populated
  components, connector access and wiring clearance. A removable PCB tray is
  a proposed way to accommodate a later board revision, not an approved design.

## Mechanism requirements confirmed by the user

- Remove the existing holder and replace it with an L-shaped wall bracket,
  retaining the existing two wall screw positions.
- Support the whole mechanism from this bracket. Aim for clearance from both
  the sill and the moving window, without relying on the sill for support.
- Keep every component as close to the mounting wall as its running,
  installation and window clearances allow. The revised layout also moves
  the gear/drum stack toward the rear wall.
- Reverse the first preview's belt entry to the opposite tangent. The current
  interpretation is the wall-facing side of the drum (+X), with the belt
  still descending from above.
- Retain the NEMA 17 motor and require an **8:1 speed reduction**: eight motor
  revolutions for one winding-drum revolution (the user's "1:8" ratio).
- Prioritize quiet, smooth operation. Transmission type, tooth geometry and
  stage count remain design choices; the reduction ratio is fixed.
- Mount the winding cylinder coaxially with the larger driven gear/pulley.
  Surface 8 represents this cylinder. Its nominal axial winding width is
  **15 mm**, matching a lifting belt approximately 15 mm wide or slightly less.
  The 15 mm measurement is a width, not a drum diameter.
- The blinds' lifting belt descends between the window and wall, following
  the route visible in the existing project. Align the winding drum with that
  route; establish the exact entry point and direction before placing it.
- The user reports that a 25 mm cylinder with a 25 mm surrounding wall held
  enough lifting belt. Belt thickness and winding length are unavailable.
  Treat that empirical capacity as the starting point; do not require the same
  unavailable measurements again.
- The sketch confirms the motor under the opening and the large gear/drum
  beside it, with the lifting belt descending from above. The user allows
  the bearing/support arrangement to be chosen.
- The user has 8 x 22 x 7 mm bearings. Use two as the starting bearing arrangement.
- Allow side clearance around the approximately 15 mm lifting belt. The first
  prototype uses a 16 mm winding channel and a 25 mm core diameter.
- The meaning of "25 mm wall around it" remains ambiguous. The first prototype
  conservatively treats it as radial height, giving 75 mm flange diameter.
  Both dimensions remain adjustable; smaller flanges must still hold the belt.
- Keep the current PCB and its required component, connector and wire space.

Other custom mechanical parts and the detailed drive layout may be
redesigned. Standard purchased parts such as transmission belts, shafts,
bearings and fasteners may be needed; their selection has not been decided.
The lifting belt and a possible transmission timing belt are distinct parts.

## Working setup

- The user has completed the Windows setup: VS Code, the blinds-cad Python
  environment, CadQuery, OCP CAD Viewer and local Codex.
- Keep the current instructions focused on Windows.
- Existing models and measurements are in Onshape. The user has supplied
  reference_assembly.step for the installation constraints. Motor and populated
  PCB geometry are not included in this assembly.
- New parametric model source will be Python/CadQuery kept in this repository.
  STEP is for CAD exchange and STL is for printing. STEP does not preserve the
  original Onshape feature tree.
- Keep explanations and user instructions short and concrete.

## First modelling milestone

1. Import the STEP assembly without transforming its components.
2. Use the reference roles below; obtain the motor and populated PCB geometry
   and verify scale against a known physical dimension.
3. Record the coordinates, available volume at the intended motor position,
   cord route and access required to assemble, tension and service the mechanism.
4. Extract documented measurements. Mark missing dimensions as unknown; do
   not silently replace them with typical values.
5. Propose a compact drive arrangement and explain what makes it fit.

The existing wall mounting surface and two fixing centres are identified
below. Printer/build volume, print material, lifting-belt details and drive-load
requirements have not yet been established for this revision. Determine them
before they affect a design choice. Do not assume that nominal motor holding torque is available
throughout motion or that mesh validity proves strength, thermal performance
or fit. Use physical fit prints and loaded tests for those checks.

## Supplied installation reference

Source: the user's uploaded `reference_assembly.step`. Preserve its original
coordinates. All coordinates and lengths below are in millimetres and are
measurements of the imported CAD, pending a physical scale check.

Source file SHA-256:
`4d4527a92f58adcbdecbcba8dd407cb208fe05319b24cb82187566fccf82b341`.

### Roles confirmed by the user

| STEP object | Role in revision 2 |
| --- | --- |
| Wall | Fixed wall and shelf/sill geometry; new parts must respect these obstructions. |
| Window | Curved window-opening clearance volume; keep this space empty. It is not a component to print. |
| Front Holder | Existing wall-mounted holder, called the back holder by the user. Remove it; retain only its wall screw positions for the new L bracket. |
| Surface 8 | Winding-cylinder reference. User confirms 15 mm nominal axial winding width for the lifting belt. Diameter and final winding clearance remain to be determined. |

The import contains three individually valid solids plus an invalid shell
corresponding to the tessellated Surface 8 representation. This explains the
whole-import validity failure in the current CAD environment; it does not
establish that the original Onshape surface is defective. Exclude Surface 8
from solid collision checks and manufacturing exports. Do not use the old
holder body as a permanent obstruction.

### Extracted mounting references

The holder's mounting face is on the inner wall plane `x = 237.0053`.
Both wall fixing axes are parallel to X. The wall and holder independently
give matching Y/Z centres:

| Fixing | X at wall contact | Y | Z |
| --- | ---: | ---: | ---: |
| Lower | 237.0053 | -4.3652 | 35.8000 |
| Upper | 237.0053 | -4.3652 | 128.8000 |

Vertical centre spacing: **93.0000 mm**.
The horizontal sill top is at `z = 21.3000`, placing these centres
14.5000 and 107.5000 mm above it. Keep the hole centres; determine fastener and
clearance diameters separately, because the wall's modelled holes differ
in diameter.

The window-opening reference spans `z = 69.3000` to `142.3000`. Its lower
face is 48.0000 mm above the sill top where their horizontal footprints
overlap. This is a geometric separation, not a confirmed usable box or an
allowance for the enclosure, print tolerances and window clearance. Check
candidate parts against the actual curved volume with an agreed margin.

### Preliminary motor fit check

A temporary 42 x 40 x 42 mm rectangular motor-body envelope, with its minimum
corner at (140, -70, 24.3), has no solid overlap with either Wall or Window.
OpenCascade reports 3.0 mm minimum distance to each. This demonstrates one
possible placement for the motor body within the 48 mm vertical separation.

The 42 mm frame and 40 mm body length are preliminary nominal dimensions
consistent with the
[MotionKing 17HS4401 family specification](https://www.motionking.com/products/Hybrid_Stepper_Motors/17HS_Stepper_Motor_42mm_1.8degree.htm).
Verify the actual motor: the envelope excludes its shaft, pilot boss,
connector, leads, screws, bracket and transmission. Its location is a fit
study only; final alignment must follow the belt route. It does not establish
that the complete assembly fits.

### Remaining inputs for layout

- Resolve whether the user's 25 mm surrounding wall means radial height.
  Verify capacity against the previously adequate drum rather than demanding
  unavailable belt thickness/length measurements.
- The coloured sketch has been supplied. The user requested the opposite
  belt-entry side after the first preview; the compact layout uses the
  wall-facing tangent (+X). Check its exact alignment against the installation;
  the blue route is illustrative.
- Actual motor mounting/shaft dimensions and populated PCB dimensions. The
  existing repository PCB layout can provide board outline and hole positions,
  while component heights and connector/wire space need verification.
- Drive load, assembly/service access, printer and material constraints as
  needed for the chosen arrangement.

### Design approach to evaluate

Use the L bracket's wall leg for the existing fixings and its projecting arm
to carry the motor, transmission and drum. Evaluate ribs/gussets and output
shaft support on the bracket so lifting-belt loads do not depend on a printed
cantilever shaft alone. Size the mechanism around the fully wound drum and
check all new parts against the original wall and window-clearance solids.

The compact fit prototype uses a 20T/160T external herringbone pair (8:1), with
0.75 mm transverse module, 8 mm face width and 67.5 mm shaft spacing. The
output gear/drum contains two 608 bearings and turns on a fixed 8 mm steel
axle clamped at the rear of the L bracket. The 39 mm axle is cantilevered;
the earlier front fork was removed to clear the curved window when moved
toward the wall. Retaining-ring and groove geometry is provisional. This
arrangement still requires a loaded stiffness check, finished retention,
tooth finishing and noise trials.

The output axis is now at X/Z = 169.2553/101.2570 mm and the motor axis at
207.0053/45.3000 mm. Both axes are parallel to Y. This moves the output
99.2553 mm and the motor 137.0053 mm closer to the mounting wall than the
first preview, with a 128.5 mm maximum gear-envelope projection from that
wall. The gear's full rotation envelope has 1 mm clearance to the wall leg.

See [LAYOUT.md](LAYOUT.md) for the source, dimensions, checks and limitations.
The detailed gears and conservative rotation envelopes clear the existing
references and supports; sampled tooth positions have no solid interference.

## Current status

An editable compact fit prototype is available in layout.py and gears.py.
It includes the replacement L bracket, nominal motor, herringbone gear pair,
drum, two 608 bearing envelopes, a shorter fixed steel axle, a provisional
retaining ring and inner-ring spacers. The belt enters the wall-facing side.
The old holder and Surface 8 are excluded.

The nominal motor retains 3 mm sill/window clearance. The current layout
passes solid and conservative rotation-envelope checks and nine coupled
tooth-interference samples. The drum's conservative envelope clears the
window by 2.577 mm and the nominal motor by 1.263 mm. These are geometric
margins before print errors or deflection. The exact belt entry, flange interpretation,
pinion attachment, belt anchor, bearing retention/fits, clamp/assembly
hardware, populated PCB placement and loaded/thermal performance remain
unfinished. No revision-2 parts are released for printing.
