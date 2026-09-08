# Window Blinds Controller: revision 2 workspace

Start here in VS Code using the **blinds-cad** Python 3.12 environment.
This folder contains the original reference viewer and a compact mechanical
fit prototype with the belt entering on the wall side. Start with
[LAYOUT.md](LAYOUT.md) to view the replacement L
bracket, 8:1 herringbone drive and winding drum. The model is not a print
release; fastening, bearing fits and PCB placement remain unfinished.

## 1. Bring in the Onshape references

In Onshape, save a named version of the current document. From the **Assembly
tab**, export one STEP file in the assembled positions. Include the wall,
window/sill constraints, existing stepper motor and current populated PCB.
Any legacy mechanism can remain in the export as historical context.

Save the result as:

`Mechanical/rev2/references/reference_assembly.step`

Use STEP rather than STL for these CAD references. Select millimetres if a
units option is shown. Keep the original assembly coordinates. Do not move
each part independently to the origin.

Also place in `references/`:

- A labelled assembly screenshot identifying the wall/sill, motor and PCB.
- Dimensioned drawings or screenshots of measurements that exist only in
  sketches, including the space below the sill and the blind cord path.
- A front/side installation photo if the model does not show the real cord
  route or the window's opening clearance.

STEP carries geometry, not the Onshape sketch constraints or feature history.
Do not remeasure dimensions already documented in Onshape. Record missing
ones explicitly. The electrical KiCad file in this repository is useful for
the PCB outline and holes; actual connectors, modules and wiring still need
their physical clearance represented.

## 2. Display the references

Open `preview.py` in VS Code so OCP CAD Viewer can recognise and start its
viewer. Make sure the viewer is running, then use a terminal with
`blinds-cad` active, from the repository root:

```powershell
python Mechanical/rev2/preview.py
```

The terminal reports sizes, positions, solid count and geometric validity.
Confirm one known dimension and the assembly orientation before using the
model for layout. CadQuery's imported solid indices are not reliable part
names; use the labelled screenshot to identify the components.

If you want to check the viewer before exporting:

```powershell
python Mechanical/rev2/preview.py --demo
```

The sample is a 40 x 30 x 6 mm block with an 8 mm through hole. It is a setup
sample, not a controller part. If needed, start the viewer through the OCP
CAD Viewer sidebar. With multiple viewers, pass their actual port through
`--port`.

To inspect another STEP file or check geometry without a viewer:

```powershell
python Mechanical/rev2/preview.py --file Mechanical/Case.step --check-only
```

## 3. Continue with local Codex

Ask local Codex to read `DESIGN_BRIEF.md` and `LAYOUT.md`, then work from
`layout.py` and `gears.py`. Run the layout clearance and tooth-interference
checks before changing the prototype. Preserve the existing reference
coordinates and wall fixing centres. The generic `preview.py --check-only`
still flags the known invalid Surface 8 shell; the layout excludes that shell.

The required reduction is now fixed at 8:1. Keep changes and measured or
provisional dimensions documented alongside the modelling scripts. Finish
the actual belt alignment, mechanical attachments and PCB arrangement before
developing physical fit samples and a functional print release.

## Dependencies

If your completed Quickstart already runs, keep using that environment.
`requirements.txt` records the core package versions used to check this
starter. It does not lock every transitive dependency. If you need to match
these versions, activate `blinds-cad` and run:

```powershell
python -m pip install -r Mechanical/rev2/requirements.txt
```

## References for this workflow

- [Onshape STEP export](https://cad.onshape.com/help/Content/File/exporting_files.htm)
- [CadQuery documentation](https://cadquery.readthedocs.io/en/latest/)
- [OCP CAD Viewer](https://github.com/bernhard-42/vscode-ocp-cad-viewer)
