#!/usr/bin/env python3
"""Print SBA 13.0.9 player/object APIs from the official interop assembly."""

import re
import sys

import dnfile

TERMS = re.compile(
    r"player|bear|character|seat|sit|death|kill|die|damage|respawn|"
    r"animation|trigger|interact|message|life|health",
    re.IGNORECASE,
)


def text(value):
    if value is None:
        return ""
    return str(value)


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: inspect_game_api.py Assembly-CSharp.dll")

    pe = dnfile.dnPE(sys.argv[1])
    if not pe.net or not pe.net.mdtables:
        raise SystemExit("The file is not a .NET assembly")

    emitted = 0
    for type_row in pe.net.mdtables.TypeDef:
        namespace = text(type_row.Namespace)
        name = text(type_row.TypeName)
        full_name = (namespace + "." if namespace else "") + name
        methods = []
        for method_ref in type_row.MethodList:
            method = method_ref.row
            method_name = text(method.Name)
            if TERMS.search(method_name):
                signature = bytes(method.Signature).hex() if method.Signature else ""
                methods.append((method_name, signature))

        if not TERMS.search(full_name) and not methods:
            continue
        print(full_name)
        for method_name, signature in methods:
            print(f"  {method_name} sig={signature}")
        emitted += 1
        if emitted >= 250:
            print("Output capped at 250 matching types.")
            break

    if emitted == 0:
        print("No matching player, interaction, death, animation, or trigger types found.")


if __name__ == "__main__":
    main()
