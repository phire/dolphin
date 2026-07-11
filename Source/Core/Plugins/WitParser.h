

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

    template<typename T>
    constexpr bool is_convertible_to() {
        return is_convertible_to<T>(kind);
    }

    template<typename T>
    static constexpr bool is_convertible_to(const Kind wit_kind) {
        if constexpr (std::is_same_v<T, uint8_t>) return wit_kind == Kind::U8;
        if constexpr (std::is_same_v<T, uint16_t>) return wit_kind == Kind::U16;
        if constexpr (std::is_same_v<T, uint32_t>) return wit_kind == Kind::U32;
        if constexpr (std::is_same_v<T, uint64_t>) return wit_kind == Kind::U64;
        if constexpr (std::is_same_v<T, int8_t>) return wit_kind == Kind::S8;
        if constexpr (std::is_same_v<T, int16_t>) return wit_kind == Kind::S16;
        if constexpr (std::is_same_v<T, int32_t>) return wit_kind == Kind::S32;
        if constexpr (std::is_same_v<T, int64_t>) return wit_kind == Kind::S64;
        if constexpr (std::is_same_v<T, float>) return wit_kind == Kind::F32;
        if constexpr (std::is_same_v<T, double>) return wit_kind == Kind::F64;
        if constexpr (std::is_same_v<T, char>) return wit_kind == Kind::Char;
        if constexpr (std::is_same_v<T, bool>) return wit_kind == Kind::Bool;
        if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>) return wit_kind == Kind::String;

        return false;
    }
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

using InterfaceItem = std::variant<Resource, Varient, Record, Flags, Enum, TypeAlias, Use, Func>;

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


using WorldDefinition = std::variant<Export, Import, Use, Include, Resource, Varient, Record, Flags, Enum, TypeAlias>;

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
