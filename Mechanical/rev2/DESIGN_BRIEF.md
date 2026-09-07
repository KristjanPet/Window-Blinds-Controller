# Revision 2: agreed design brief

## Goal

Redesign the custom mechanics of the Window Blinds Controller for wall
mounting, with the motor underneath the window/windowsill. Make the custom
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
- Existing models and measurements are in Onshape. Exported references must
  be supplied before final layout or dimensions can be established.
- New parametric model source will be Python/CadQuery kept in this repository.
  STEP is for CAD exchange and STL is for printing. STEP does not preserve the
  original Onshape feature tree.
- Keep explanations and user instructions short and concrete.

## First modelling milestone

1. Import the STEP assembly without transforming its components.
2. Identify the fixed wall/window geometry, motor and populated PCB; verify
   scale against a known physical dimension.
3. Record the coordinates, available volume below the sill, cord route and
   access required to assemble, tension and service the mechanism.
4. Extract documented measurements. Mark missing dimensions as unknown; do
   not silently replace them with typical values.
5. Propose a compact drive arrangement and explain what makes it fit.

The final mounting surface and permitted fixing locations, printer/build
volume, print material, cord geometry and drive-load requirements have not
yet been established for this revision. Determine them before they affect a
design choice. Do not assume that nominal motor holding torque is available
throughout motion or that mesh validity proves strength, thermal performance
or fit. Use physical fit prints and loaded tests for those checks.

## Current status

The starter scripts are setup/reference-inspection tools. No installation
dimensions have been approved and no revision-2 parts are ready for printing.
