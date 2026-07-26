#pragma once

#include <vector>
#include <string>
#include <optional>
#include <ranges>


#include "Common/EnumFormatter.h"

namespace WitLexy
{

struct SemVer {
    unsigned major;
    unsigned minor;
    unsigned patch;
};

using Ident = std::string_view;

struct Path {
    std::vector<std::string> namespaces;
    std::vector<std::string> path;
    std::string name;
    std::optional<SemVer> version;

    constexpr Path() = default;
    constexpr Path(std::string name_) : name(std::move(name_)) {}
    constexpr Path(std::vector<std::string> ns, std::vector<std::string> p, std::string n)       : namespaces(std::move(ns)), path(std::move(p)), name(std::move(n)) {}
    constexpr Path(std::vector<std::string> ns, std::vector<std::string> p, std::string_view n, std::optional<SemVer> v) : namespaces(std::move(ns)), path(std::move(p)), name(std::move(n)), version(v) {}
};

enum class TypeKind {
    Void,
    U8,
    U16,
    U32,
    U64,
    S8,
    S16,
    S32,
    S64,
    F32,
    F64,
    Bool,
    CharType,
    StringType,
    ListStart,
    ListEnd,
    TupleStart,
    TupleEnd,
};

struct Type {
    std::vector<TypeKind> type_tree;
};

struct NamedType {
    Ident id;
    Type type;
    constexpr NamedType(Ident&& id_, Type&& type_) : id(std::move(id_)), type(std::move(type_)) {}
};

struct ParamList : public std::vector<NamedType> {};

struct FuncType {
    size_t num_params;
    ParamList params;
    Type result;
};

struct Method {
    Ident id;
    FuncType type;
    // constexpr Method(Ident id_, std::vector<NamedType> &&params) : id(std::move(id_)), type(FuncType{params.size(), {std::move(params)}, Type({TypeKind::Void})}) {}
    constexpr Method(Ident id_, std::vector<NamedType> &&params, std::optional<Type> &&result) : id(std::move(id_)), type(FuncType{params.size(), {std::move(params)}, result.value_or(Type({TypeKind::Void}))}) {}
};

struct Resource {
    Ident id;
    std::vector<Method> methods;
    constexpr Resource(Ident id_, std::vector<Method> &&methods_) : id(std::move(id_)), methods(std::move(methods_)) {}
};

struct Interface {
    Ident id;
    std::vector<Resource> resources;
    constexpr Interface(Ident id_, std::vector<Resource> &&resources_) : id(std::move(id_)), resources(std::move(resources_)) {}
};

struct PackageDecl {
    Path path;
};

struct WitFile {
    Path package_decl;
    std::vector<Interface> interfaces;
    constexpr WitFile(Path package_decl_, std::vector<Interface> &&interfaces_) : package_decl(std::move(package_decl_)), interfaces(std::move(interfaces_)) {}
};

}

template <>
struct fmt::formatter<WitLexy::TypeKind> : EnumFormatter<WitLexy::TypeKind::TupleEnd>
{
   constexpr formatter() : EnumFormatter({"Void", "U8", "U16", "U32", "U64", "S8", "S16", "S32", "S64", "F32", "F64", "Bool", "CharType", "StringType", "ListStart", "ListEnd", "TupleStart", "TupleEnd"}) {}
};

template <>
struct fmt::formatter<WitLexy::Type>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const WitLexy::Type& type_info, FormatContext& ctx) const {
        if (type_info.type_tree.size() == 1) {
            return fmt::format_to(ctx.out(), "{:n}", type_info.type_tree[0]);
        }

        fmt::format_to(ctx.out(), "[{:n}", type_info.type_tree[0]);
        for (auto kind : type_info.type_tree | std::views::drop(1)) {
            fmt::format_to(ctx.out(), ", {:n}", kind);
        }
        fmt::format_to(ctx.out(), "]");
        return ctx.out();
    }
};