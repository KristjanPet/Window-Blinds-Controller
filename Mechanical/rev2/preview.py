"""Inspect the revision-2 STEP references or display a labelled setup sample.

Run from the repository root with the blinds-cad Python interpreter:
    python Mechanical/rev2/preview.py
    python Mechanical/rev2/preview.py --demo
    python Mechanical/rev2/preview.py --check-only

The original STEP coordinates are preserved. This does not generate a drive design.
"""

from __future__ import annotations

import argparse
from importlib.metadata import version
from pathlib import Path
import sys

import cadquery as cq


DEFAULT_REFERENCE = Path(__file__).resolve().parent / "references" / "reference_assembly.step"


def describe(shape: cq.Shape, label: str) -> bool:
    """Report imported geometry; validity does not establish mechanical fitness."""
    solids = shape.Solids()
    bbox = shape.BoundingBox()
    valid = bool(solids) and shape.isValid()
    print(f"Model: {label}")
    print(f"CadQuery: {version('cadquery')} | OCP: {version('cadquery-ocp')}")
    print("Dimension convention: mm; check import scale against a known physical dimension.")
    print(f"Overall size: {bbox.xlen:.3f} x {bbox.ylen:.3f} x {bbox.zlen:.3f} mm")
    print(f"Solids: {len(solids)}")
    for index, solid in enumerate(solids, start=1):
        box = solid.BoundingBox()
        solid_valid = solid.isValid() and solid.Volume() > 0
        valid = valid and solid_valid
        print(
            f"  solid_{index:03d}: size=({box.xlen:.3f}, {box.ylen:.3f}, {box.zlen:.3f}) "
            f"min=({box.xmin:.3f}, {box.ymin:.3f}, {box.zmin:.3f}) mm "
            f"volume={solid.Volume():.3f} mm^3 valid={solid_valid}"
        )
    print(f"Solid geometry check: {'PASS' if valid else 'FAIL'}")
    return valid


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--demo", action="store_true", help="Show a sample block for setup only.")
    source.add_argument("--file", type=Path, help="Inspect another STEP file without relocating it.")
    parser.add_argument("--check-only", action="store_true", help="Check geometry without a viewer.")
    parser.add_argument("--port", type=int, help="OCP viewer port, if more than one viewer is running.")
    args = parser.parse_args()

    if args.demo:
        label = "SETUP SAMPLE - not a blinds part"
        shape = cq.Workplane("XY").box(40, 30, 6).faces(">Z").hole(8).val()
    else:
        path = (args.file or DEFAULT_REFERENCE).expanduser().resolve()
        if not path.is_file():
            print(f"Reference file is missing: {path}", file=sys.stderr)
            print("Export the assembled Onshape references as described in Mechanical/rev2/README.md.", file=sys.stderr)
            print("Use --demo only to check the viewer setup.", file=sys.stderr)
            return 2
        try:
            imported = cq.importers.importStep(str(path))
            shape = cq.Compound.makeCompound(imported.vals())
        except Exception as exc:
            print(f"STEP import failed: {exc}", file=sys.stderr)
            return 2
        label = path.name

    if not describe(shape, label):
        return 2
    if args.check_only:
        return 0

    # Keeping this import in the file also allows the VS Code extension to
    # recognise it and start the viewer when the file is opened.
    from ocp_vscode import show, set_port

    if args.port is not None:
        set_port(args.port)
    try:
        show(shape, names=[label])
    except Exception as exc:
        print(f"Viewer connection failed: {exc}", file=sys.stderr)
        print("Start OCP CAD Viewer in VS Code, then run this file again.", file=sys.stderr)
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
