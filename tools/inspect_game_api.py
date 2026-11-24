#!/usr/bin/env python3
"""Print likely SBA 13.0.9 player and interaction APIs from Assembly-CSharp."""

import re
import sys

import dnfile

TYPE_TERMS = re.compile(
    r"player|bear|character|sit|seat|respawn|interact",
    re.IGNORECASE,
)
METHOD_TERMS = re.compile(
    r"die|death|kill|damage|sit|seat|respawn|interact|trigger|animation",
    re.IGNORECASE,
)


def text(value):
    return "" if value is None else str(value)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: inspect_game_api.py Assembly-CSharp.dll")

    pe = dnfile.dnPE(sys.argv[1])
    if not pe.net or not pe.net.mdtables:
        raise SystemExit("The file is not a .NET assembly")

    emitted = 0
    for type_row in pe.net.mdtables.TypeDef:
        namespace = text(type_row.TypeNamespace)
        name = text(type_row.TypeName)
        full_name = (namespace + "." if namespace else "") + name
        if not TYPE_TERMS.search(full_name):
            continue
        methods = [text(method.row.Name) for method in type_row.MethodList]
        relevant = [method for method in methods if METHOD_TERMS.search(method)]
        if not relevant:
            continue
        print(full_name)
        print("  " + ", ".join(relevant))
        emitted += 1
        if emitted >= 300:
            print("Output capped at 300 matching types.")
            break

    if emitted == 0:
        print("No matching player, interaction, death, animation, or trigger types found.")


if __name__ == "__main__":
    main()
