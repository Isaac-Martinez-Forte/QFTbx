#!/usr/bin/env python3
"""Ports a .qft from version 2 or 3 to version 4.

The data does not change: the sections move into <inputs> and <results>, so
that a file reads down the page - what the user described first, what came
out of it at the bottom. The controller structure travels with the inputs,
which is what it is: the box the search is asked to look in.

    tools/port_qft_to_v4.py file.qft [file.qft ...]

Each file is rewritten in place. A file already at version 4 is left alone.
"""
import sys
import xml.etree.ElementTree as ET

INPUTS = ("plant", "specifications", "omega", "controller")
RESULTS = ("templates", "boundaries", "loop-shaping")


def indent(element, level=0):
    pad = "\n" + "    " * level
    if len(element):
        if not (element.text or "").strip():
            element.text = pad + "    "
        for i, child in enumerate(element):
            indent(child, level + 1)
            if not (child.tail or "").strip():
                child.tail = pad + ("    " if i < len(element) - 1 else "")
    if level and not (element.tail or "").strip():
        element.tail = pad


def port(path):
    tree = ET.parse(path)
    root = tree.getroot()

    version = root.get("version")
    if version == "4":
        return "already at 4"
    if version not in ("2", "3"):
        return "NOT PORTED: version %s" % version

    sections = {child.tag: child for child in root}
    for child in list(root):
        root.remove(child)

    inputs = ET.SubElement(root, "inputs")
    for tag in INPUTS:
        if tag in sections:
            inputs.append(sections.pop(tag))

    if any(tag in sections for tag in RESULTS):
        results = ET.SubElement(root, "results")
        for tag in RESULTS:
            if tag in sections:
                results.append(sections.pop(tag))

    # Anything the file carried that this does not know about stays at the
    # root rather than being dropped: a port that loses a section silently is
    # worse than one that refuses.
    for leftover in sections.values():
        root.append(leftover)

    # A version-2 file measured its epsilon in the complex plane, which
    # version 3 made explicit. The port says so, since version 4 reads the
    # attribute and a missing one would be read as the other plane.
    if version == "2":
        for epsilon in root.iter("epsilon"):
            epsilon.set("metric", "complex")

    root.set("version", "4")
    indent(root)
    tree.write(path, encoding="UTF-8", xml_declaration=True)
    return "ported from %s" % version


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        raise SystemExit(2)
    for path in sys.argv[1:]:
        print("%-62s %s" % (path, port(path)))
