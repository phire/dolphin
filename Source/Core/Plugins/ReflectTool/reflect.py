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
    Type("float"),
    Type("double"),
    Type("String"),
]

types = {}
for t in basic_types:
    types[t.name] = t

@dataclass
class Function:
    name: str
    ret: Type = None
    args: list[(str, Type)] = field(default_factory=list)


@dataclass
class Class:
    name: str
    methods: dict[str, Function] = field(default_factory=dict)
    members: dict[str, Type] = field(default_factory=dict)

@dataclass
class Module:
    name: str
    global_functions: dict[str, Function] = field(default_factory=dict)
    classes: dict[str, Class] = field(default_factory=dict)



exports = []

def GetType(cursor):
    if (cursor.spelling in types):
        return types[cursor.spelling]
    return None

# def GetClass(cursor):
#     cls = None
#     namespace = []
#     while cursor := cursor.semantic_parent:
#         if cursor.kind == CursorKind.TRANSLATION_UNIT:
#             break
#         assert cursor.kind == CursorKind.NAMESPACE
#         namespace.append(cursor.spelling)
#         for child in cursor.get_children():
#             if child.kind == CursorKind.ANNOTATE_ATTR and child.spelling == "foo_class":
#                 cls = namespace[-1]
#     if not cls:
#         return (namespace, None)
#     if cls not in types:
#         t = Type(cls)
#         exports.append(t)
#         types[cls] = t
#     else:
#         t = types[cls]
#         assert t.namespace == namespace
#     return (namespace, types[cls])

def HandleParam(cursor):
    print("  Param: " + cursor.spelling)
    for c in cursor.get_children():
        print(f"   Unknown {c.kind}: {c.spelling}")

def HandleFunction(cursor, module, cls):
    if HasAttr(cursor, "zap_ignore"):
        return None

    print("   Function: " + cursor.get_usr())

    name = cursor.spelling

    fn = Function(name, module, cls)
    is_abi = False
    is_visible = False

    for c in cursor.get_children():
        match(c.kind):
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
                print(f"  Annotate: {c.spelling}")
                if c.spelling == "foo_abi":
                    is_abi = True
            # case CursorKind.COMPOUND_STMT:
            #     pass
            case unknown:
                print(f"  Unknown {unknown}: {cursor.spelling}")

    return fn


def HandleCall(cursor : Cursor):
    name = cursor.spelling
    args = []
    for c in cursor.get_children():
        match (c.kind):
            case CursorKind.UNEXPOSED_EXPR:
                return
            case n:
                raise Exception(f"Unknown arg expression {n} for {name}")

def HasAttr(cursor, attr):
    for c in cursor.get_children():
        #print(c.kind, c.spelling)
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


# def HandleVar(cursor):
#     for c in cursor.get_children():
#         match (c.kind):
#             case CursorKind.TYPE_REF:
#                 print(f"  Type: {c.spelling}")
#             case n:
#                 print(f"  Unknown {c.kind}: {c.spelling} {c.location}")

def ExportStruct(cursor, module):
    cls = Class(cursor.spelling)
    for c in cursor.get_children():
        match (c.kind):
            case CursorKind.FIELD_DECL:
                cls.fields[c.spelling] = GetType(c)
            case CursorKind.CXX_METHOD:
                if fn := HandleFunction(c, module, cls):
                    if fn.name in cls.methods:
                        raise Exception(f"Duplicate method {fn.name}")
                cls.methods[fn.name] = fn
            case n:
                print(f"Unknown {n}: {c.spelling} {c.location}")


def ExportNamespace(cursor, module):
    mod = Module(module)
    for c in cursor.get_children():
        match (c.kind):
            case CursorKind.FUNCTION_DECL:
                if fn := HandleFunction(c, module, None):
                    if fn.name in mod.global_functions:
                        raise Exception(f"Duplicate function {fn.name}")
                    mod.global_functions[fn.name] = fn
            case CursorKind.STRUCT_DECL:
                if cls := ExportStruct(c, module):
                    if cls.name in mod.classes:
                        raise Exception(f"Duplicate class {cls.name}")
                    mod.classes[cls.name] = cls
            case n:
                print(f"Unknown {n}: {c.spelling} {c.location}")

Namespaces = defaultdict(list)
namespace_exports = []


def MatchZapNamespaceExport(cursor: Cursor):
    # We are matching the following c++ code:
    # namespace namespace_to_export {
    #    namespace zap_module { <-- Cursor
    #         extern const Module* zap_module_obj
    #         static const Zap::ModuleInfo zap_module_info (
    #             "module_name"
    #             "module description",
    #             {module info},
    #             zap_module_obj
    #         );
    #     }
    # }

    # Check for the definition of zap_module_info
    infoCursor = next(filter(lambda c: c.spelling == "zap_module_info", cursor.get_children()))
    if infoCursor:
        infoConstructor : Cursor = next(filter(lambda c: c.kind == CursorKind.CALL_EXPR, infoCursor.get_children()))

        if infoConstructor and infoConstructor.type.spelling == "const Zap::ModuleInfo":
            args = list(infoConstructor.get_children())

            # fixme: this will only work with string literals
            name = next(args[0].get_children()).spelling.replace('"', '')
            desc = next(args[1].get_children()).spelling.replace('"', '')
            # todo: version

            # grab usr to namespace_to_export
            parent = cursor.semantic_parent
            if parent.kind != CursorKind.NAMESPACE:
                raise Exception("ZAP_NAMESPACE_MODULE must be defined inside a namespace")
            module_namespace = parent.get_usr()

            # push export info for later
            namespace_exports.append((module_namespace, name, desc))

        else:
            raise Exception("invalid zap_module_info")

def FindExports(node: Cursor):
    for cursor in node.get_children():
        match(cursor.kind):
            case CursorKind.NAMESPACE:
                if cursor.spelling in ["std", "fmt"]: # ignore common external namespaces
                    continue
                if cursor.spelling == "zap_module":
                    MatchZapNamespaceExport(cursor)
                else:
                    Namespaces[cursor.get_usr()].append(cursor) # collect all namespaces so we can return if they are exported
                    FindExports(cursor)

def ParseTu(filename, args):
    index = Index.create()

    try:
        TU = index.parse(filename, args) #options= TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    except TranslationUnitLoadError:
        print("Error: Unable to parse translation unit.")
        return None

    if any(diag.severity >= Diagnostic.Error for diag in TU.diagnostics):
        for diag in TU.diagnostics:
            print(diag)
        return None

    FindExports(TU.cursor)
    for (export, _, _) in namespace_exports:
        for namespace in Namespaces[export]:
            ExportNamespace(namespace, export)

    print(f"Module {export}:")
    print

def default_args():
    # Clang adds some default include paths which libclang doesn't have a proper way of accessing
    # so we run clang verbosely on a dummy c++ file and extract the default include paths
    import subprocess, re
    clang = subprocess.run(['clang', '-E', '-x', 'c++', '-v', '-'], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)

    if clang.returncode != 0:
        raise Exception("Failed to run clang")

    lines = clang.stderr.decode('utf-8').splitlines()

    start = lines.index('#include <...> search starts here:') + 1
    end = lines.index('End of search list.')

    return [f'-isystem{include.strip()}' for include in lines[start:end] ]

def main():
    if sys.version_info.major < 3 or (sys.version_info.major == 3 and sys.version_info.minor < 10):
        print("Error: Requires Python 3.10 or higher")
        return 1

    args = [
        '-I/home/phire/dolphin/dolphin/Source/Core/Plugins/PluginABI/',
        '-I/home/phire/dolphin/dolphin/Source/Core/Plugins/PluginCpp/',
        '-I/home/phire/dolphin/dolphin/Source/Core/',
        '-I/home/phire/dolphin/dolphin/Source/',
        '-std=c++17',
        '-D__STDC_CONSTANT_MACROS',
        '-D__STDC_LIMIT_MACROS'
    ]

    args.extend(default_args())

    ParseTu(sys.argv[1], args)
    return 0

if __name__ == "__main__":
    sys.exit(main())