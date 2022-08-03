#!/usr/bin/env python3

import sys
from clang.cindex import *
from dataclasses import dataclass, field
from collections import defaultdict

@dataclass
class Type:
    name: str
    namespace: list = field(default_factory=list)

basic_types = [
    Type("uint32_t"),
    Type("uint64_t"),
    Type("WrappedFloat"),
    Type("WrappedDouble"),
    Type("String"),
]

types = {}
for t in basic_types:
    types[t.name] = t

@dataclass
class Function:
    name: str
    namespace: str
    cls: Type = None
    ret: Type = None
    args = list[(str, Type)]

exports = []

def GetType(cursor):
    if (cursor.spelling in types):
        return types[cursor.spelling]
    return None

def GetClass(cursor):
    cls = None
    namespace = []
    while cursor := cursor.semantic_parent:
        if cursor.kind == CursorKind.TRANSLATION_UNIT:
            break
        assert cursor.kind == CursorKind.NAMESPACE
        namespace.append(cursor.spelling)
        for child in cursor.get_children():
            if child.kind == CursorKind.ANNOTATE_ATTR and child.spelling == "foo_class":
                cls = namespace[-1]
    if not cls:
        return (namespace, None)
    if cls not in types:
        t = Type(cls)
        exports.append(t)
        types[cls] = t
    else:
        t = types[cls]
        assert t.namespace == namespace
    return (namespace, types[cls])

def HandleParam(cursor):
    print("  Param: " + cursor.spelling)
    for c in cursor.get_children():
        print(f"   Unknown {c.kind}: {c.spelling}")

def HandleFunction(cursor):
    print("Function: " + cursor.spelling)

    name = cursor.spelling
    namespace, cls = GetClass(cursor)

    fn = Function(name, namespace, cls)
    is_abi = False
    is_visible = False

    for c in cursor.get_children():
        match(c.kind):
            case CursorKind.VISIBILITY_ATTR:
                is_visible = c.spelling == "default"
            case CursorKind.PARM_DECL:
                a = HandleParam(c)
                if a:
                    fn.args.append(a)
            case CursorKind.TYPE_REF:
                fn.ret = GetType(c)
            case CursorKind.FUNCTION_DECL:
                for cc in c.get_children():
                    print(f"   Unknown {cc.kind}: {cc.spelling}")
            case CursorKind.ANNOTATE_ATTR:
                if c.spelling == "foo_abi":
                    is_abi = True
            case CursorKind.COMPOUND_STMT:
                pass
            case unknown:
                print(f"  Unknown {unknown}: {cursor.spelling}")

    if is_abi:
        exports.append(fn)

def HasAttr(cursor, attr):
    for c in cursor.get_children():
        print(c.kind, c.spelling)
        if c.kind == CursorKind.ANNOTATE_ATTR and c.spelling == attr:
            return True
    return False

def GetChild(cursor, kind):
    for c in cursor.get_children():
        if c.kind == kind:
            return c
    return None

def GetChildSpelling(cursor, kind):
    for c in cursor.get_children():
        if c.kind == kind:
            return c.spelling
    return None

namespaces = defaultdict(list)
namespace_exports = []

def HandleVar(cursor):
    for c in cursor.get_children():
        match (c.kind):
            case CursorKind.TYPE_REF:
                print(f"  Type: {c.spelling}")
            case n:
                print(f"  Unknown {c.kind}: {c.spelling} {c.location}")

def HandleNamespace(cursor):
    print(cursor.location)
    for c in cursor.get_children():
        match (c.kind):
            case CursorKind.FUNCTION_DECL:
                print(f"Function: {cursor.spelling}")
                HandleFunction(c)
            case CursorKind.VAR_DECL:
                print(f"Var: {cursor.spelling}")
                HandleVar(c)
            case n:
                print(f"Unknown {c.kind}: {c.spelling} {c.location}")


def FindExports(TU):
    # if (cursor.kind.is_invalid()):
    #     raise Exception("Invalid cursor")
    for cursor in TU.get_children():
        match(cursor.kind):
            case CursorKind.NAMESPACE:
                #print ("Namespace: " + cursor.spelling)
                namespaces[cursor.spelling].append(cursor)
            case CursorKind.NAMESPACE_ALIAS:
                print ("Namespace alias: " + cursor.spelling)
                if cursor.spelling.startswith("zap_module_"):
                    exported_namespace = GetChildSpelling(cursor, CursorKind.NAMESPACE_REF)
                    print(f"  Exporting namespace: {exported_namespace}")
                    namespace_exports.append(exported_namespace)
            #case CursorKind.FUNCTION_DECL:
                #HandleFunction(cursor)
                #print(f"Visibility: {cursor.spelling}")
            #    pass
            # case n:
            #     print(f"Unknown {cursor.kind}: {cursor.spelling}")


def ParseTu(filename, args):
    index = Index.create()
    try:
        TU = index.parse(filename, args) #options= TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    except TranslationUnitLoadError:
        print("Error: Unable to parse translation unit.")
        return None

    FindExports(TU.cursor)

    for export in namespace_exports:
        for namespace in namespaces[export]:
            HandleNamespace(namespace)

    print("Exports:")
    for export in exports:
        print(export)

def main():
    if sys.version_info.major < 3 or (sys.version_info.major == 3 and sys.version_info.minor < 10):
        print("Error: Requires Python 3.10 or higher")
        return 1

    args = [
        '-I/home/phire/dolphin/dolphin/Source/Core/Plugins/PluginABI/',
        '-I/home/phire/dolphin/dolphin/Source/Core/Plugins/PluginCpp/',
        '-i/home/phire/dolphin/dolphin/Source/Core/',
    ]

    ParseTu(sys.argv[1], args)
    return 0

if __name__ == "__main__":
    sys.exit(main())