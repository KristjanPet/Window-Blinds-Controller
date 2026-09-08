# Revision 2: agreed design brief

## Goal

Redesign the custom mechanics of the Window Blinds Controller for wall
mounting, with the motor underneath the window. Its placement relative to the
sill (above or below it) still needs clarification. Make the custom
structural parts and enclosure suitable for 3D printing. This is a full
mechanical revision, not simply a copy of the aluminium holder in plastic.

## Fixed references from the user

- Existing stepper motor. Repository hardware documentation identifies it as
  NEMA 17, model 17HS4401. Keep that motor; establish its actual mounting and
  shaft dimensions from the supplied CAD or verified drawing.
- Existing wall/window geometry and its physical installation constraints.
- Current PCB for now. Keep its dimensions, mounting features, populated
  components, connector access and wiring clearance. A removable PCB tray is
  a proposed way to accommodate a later board revision, not an approved design.

All other custom mechanical parts, drive layout and gearing may be redesigned.
The existing 1:8 timing-belt reduction is historical context, not a required
ratio for revision 2. Standard purchased parts such as belts, shafts, bearings
and fasteners may be needed; their selection has not been decided.

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
below. Printer/build volume, print material, cord geometry and drive-load
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
| Front Holder | Existing wall-mounted holder, called the back holder by the user. Use only its wall screw positions; its body may be redesigned or removed. |
| Surface 8 | Width reference only. The user will supply the relevant width and its endpoints; do not derive it from this imported surface. |

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

### Remaining inputs for layout

- The width represented by Surface 8, including the two features it measures.
- Whether the motor belongs above the sill, beneath the opening, or physically
  below the sill.
- Motor mounting/shaft dimensions and populated PCB dimensions. The existing
  repository PCB layout can provide board outline and hole positions, while
  component heights and connector/wire space need verification.
- Cord route and drive interface, then load, access and printing constraints
  as needed for the chosen arrangement.

## Current status

The installation reference has been inspected and its roles recorded.
The scripts remain setup/reference-inspection tools; no revision-2 parts
are ready for printing.
