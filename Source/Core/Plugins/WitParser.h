

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace Wit {


struct SemVer {
    unsigned major;
    unsigned minor;
    unsigned patch;
};

struct Path {
    std::vector<std::string> namespaces;
    std::vector<std::string> path;
    std::string name;
    std::optional<SemVer> version;
};

struct PackageDecl : public Path {};

struct UsePath : public Path {};

struct Gate {
    enum class Type {
        None,
        Unstable,
        Since,
        Deprecated,
    };
    constexpr bool is_none() const { return m_type == Type::None; }

    constexpr Gate() : m_type(Type::None), m_value(std::monostate{}) {}
    constexpr Gate(Type type, std::variant<std::monostate, std::string, SemVer> value) : m_type(type), m_value(value) {}

    Type m_type;
    std::variant<std::monostate, std::string, SemVer> m_value;
};

struct UseMap {
    std::string name;
    std::string alias;
};

struct Use {
    Gate gate;
    UsePath path;
    std::vector<UseMap> items;
};

struct Rename {
    Gate gate;
    UsePath path;
    std::string name;
};

struct Resource {
    Gate gate;
    std::string name;
    std::vector<struct Func> methods;
};

struct Varient {
    Gate gate;
    std::string name;
    std::vector<std::pair<std::string, struct Ty>> cases;
};

struct Record {
    Gate gate;
    std::string name;
    std::vector<std::pair<std::string, struct Ty>> fields;
};

struct Flags {
    Gate gate;
    std::string name;
    std::vector<std::string> fields;
};

struct Enum {
    Gate gate;
    std::string name;
    std::vector<std::string> cases;
};

struct Ty {
    enum class Kind {
        U8, U16, U32, U64,
        S8, S16, S32, S64,
        F32, F64,
        Char, Bool, String,
        Tuple, List, FixedLengthList, Option, Result,
        Map, Handle, Future, Stream, Ignore, Id,
    };

    Kind kind;
    std::vector<Ty> params;
    std::optional<std::string> id;

    constexpr Ty(Kind kind_, std::vector<Ty> params_ = {}) : kind(kind_), params(std::move(params_)) {}
    constexpr Ty(Kind kind_, std::string id_) : kind(kind_), id(id_) {}
    constexpr Ty() : kind(Kind::Ignore) {}
};

struct TypeAlias {
    Gate gate;
    std::string name;
    Ty ty;
};

using TypeDef = std::variant<Resource, Varient, Record, Flags, Enum, TypeAlias>;

struct Include {
    Gate gate;
    std::string name;
};

struct NamedType {
    std::string name;
    Ty type;
};

struct FuncType {
    bool async = false;
    bool static_ = false;
    bool constructor = false;
    constexpr FuncType() = default;
    constexpr FuncType(auto params_, auto results_) : params(std::move(params_)), results(std::move(results_)) {}
    std::vector<NamedType> params;
    std::vector<Ty> results;
};

struct Func {
    Gate gate;
    std::string name;
    FuncType ty;
};

using InterfaceItem = std::variant<TypeDef, Use, Func>;

struct Interface {
    Gate gate;
    std::string name;
    std::vector<InterfaceItem> items;
};


using ExternType = std::variant<Func, Interface, Rename, UsePath>;


struct Export {
    Gate gate;
    ExternType item;
};

struct Import {
    Gate gate;
    ExternType item;
};


using WorldDefinition = std::variant<Export, Import, Use, TypeDef, Include>;

struct World {
    Gate gate;
    std::string name;
    std::vector<WorldDefinition> items;
};


using PackageItem = std::variant<World, Interface>;


struct Nested {
    PackageDecl package_decl;
    std::vector<PackageItem> items;
};

struct WitFile {
    PackageDecl package_decl;

};

}

std::vector<Wit::PackageItem> parse_wit();
