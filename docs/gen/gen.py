#!/usr/bin/env python3

"""This script focuses on removing useless stuff from Doxygen output.
i.e. it removes all files which dont have any documentation(pass 1), 
and then removes all objects and classes which dont have any documentation(pass 2)"""

import os
import re
from lxml import etree

XML_OUTPUT_DIR = "./xml"
MARKDOWN_OUTPUT_DIR = "./docs/wiki"

def has_doc(elem):
    # Check if briefdescription or detaileddescription has content.
    brief = elem.find("briefdescription")
    if brief is not None:
        if (brief.text and brief.text.strip()) or len(brief) > 0:
            return True
    detailed = elem.find("detaileddescription")
    if detailed is not None:
        if (detailed.text and detailed.text.strip()) or len(detailed) > 0:
            return True
    return False

def any_documented_member(compounddef):
    # Return True if at least one memberdef in any section is documented.
    for section in compounddef.findall("sectiondef"):
        for mdef in section.findall("memberdef"):
            if has_doc(mdef):
                return True
    return False

def strip_undocumented_members(compounddef):
    # Remove all undocumented memberdef and empty sections.
    for section in compounddef.findall("sectiondef"):
        bad = [m for m in section.findall("memberdef") if not has_doc(m)]
        for m in bad:
            section.remove(m)
        if not section.findall("memberdef"):
            compounddef.remove(section)

def main():
    index_path = os.path.join(XML_OUTPUT_DIR, "index.xml")
    if not os.path.exists(index_path):
        print(f"Error: {index_path} not found.")
        return

    tree = etree.parse(index_path)
    root = tree.getroot()

    # Pass 1: drop files with zero documentation
    to_drop = []
    for comp in root.findall("compound"):
        refid = comp.get("refid")
        if not refid:
            continue
        fname = os.path.join(XML_OUTPUT_DIR, refid + ".xml")
        if not os.path.exists(fname):
            to_drop.append(comp)
            continue

        try:
            sub = etree.parse(fname)
            cdef = sub.getroot().find("compounddef")
            if cdef is None:
                to_drop.append(comp)
                continue
            if not (has_doc(cdef) or any_documented_member(cdef)):
                os.remove(fname)
                to_drop.append(comp)
                print(f"Removed (no docs): {fname}")
        except Exception as e:
            print(f"Error processing {fname}: {e}")
            to_drop.append(comp)

    for comp in to_drop:
        root.remove(comp)
    tree.write(index_path, encoding="utf-8", xml_declaration=True)
    print(f"Pass 1 done. Removed {len(to_drop)} files.")

    # Pass 2: clean members in the rest
    tree = etree.parse(index_path)
    root = tree.getroot()
    for comp in root.findall("compound"):
        refid = comp.get("refid")
        if not refid:
            continue
        fname = os.path.join(XML_OUTPUT_DIR, refid + ".xml")
        if not os.path.exists(fname):
            continue
        try:
            sub = etree.parse(fname)
            cdef = sub.getroot().find("compounddef")
            if cdef is not None:
                strip_undocumented_members(cdef)
                sub.write(fname, encoding="utf-8", xml_declaration=True)
        except Exception as e:
            print(f"Error cleaning {fname}: {e}")

    print("Pass 2 done. Undocumented members removed.")

if __name__ == "__main__":
    main()

