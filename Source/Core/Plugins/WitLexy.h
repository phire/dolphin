

#pragma once

#include <lexy/dsl.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/grammar.hpp>


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



template <typename T>
struct PrimitiveType {
    using type = T;
};

using U8_t = PrimitiveType<uint8_t>;
using U16_t = PrimitiveType<uint16_t>;
using U32_t = PrimitiveType<uint32_t>;
using U64_t = PrimitiveType<uint64_t>;
using S8_t = PrimitiveType<int8_t>;
using S16_t = PrimitiveType<int16_t>;
using S32_t = PrimitiveType<int32_t>;
using S64_t = PrimitiveType<int64_t>;
using F32_t = PrimitiveType<float>;
using F64_t = PrimitiveType<double>;
using Bool_t = PrimitiveType<bool>;

auto constexpr U8 = U8_t{};
auto constexpr U16 = U16_t{};
auto constexpr U32 = U32_t{};
auto constexpr U64 = U64_t{};
auto constexpr S8 = S8_t{};
auto constexpr S16 = S16_t{};
auto constexpr S32 = S32_t{};
auto constexpr S64 = S64_t{};
auto constexpr F32 = F32_t{};
auto constexpr F64 = F64_t{};
auto constexpr Bool = Bool_t{};

struct CharType {
    using type = char32_t;
};

struct StringType {
    using type = std::string;
};

template <typename T>
struct ListTypeT {
    std::unique_ptr<T> element_type;
};

template <typename T>
using TypeV = std::variant<
    PrimitiveType<uint8_t>,
    PrimitiveType<uint16_t>,
    PrimitiveType<uint32_t>,
    PrimitiveType<uint64_t>,
    PrimitiveType<int8_t>,
    PrimitiveType<int16_t>,
    PrimitiveType<int32_t>,
    PrimitiveType<int64_t>,
    PrimitiveType<float>,
    PrimitiveType<double>,
    PrimitiveType<bool>,
    CharType,
    StringType
    //ListTypeT<T>
>;

// Type level fixed-point operator - https://stackoverflow.com/a/53504373
template <template<class> class K>
struct Fix : K<Fix<K>> {
    using K<Fix<K>>::K;
};

using Type = Fix<TypeV>;
//using ListType = ListTypeT<Type>;

struct NamedType {
    Ident id;
    Type type;
};

struct ParamList : public std::vector<NamedType> {};

struct FuncType {
    ParamList params;
    std::optional<Type> result;
};

struct Method {
    Ident id;
    FuncType type;
};

struct Resource {
    Ident id;
    std::vector<Method> methods;
};

struct Interface {
    Ident id;
    std::vector<Resource> resources;
};

struct PackageDecl {
    Path path;
};

struct WitFile {
    Path package_decl;
    std::vector<Interface> interfaces;
};

namespace dsl = lexy::dsl;

struct ident {
    static constexpr auto rule = [] {
        auto head = dsl::ascii::alpha / dsl::lit_c<'-'>;
        auto tail = dsl::ascii::alnum / dsl::lit_c<'-'>;
        auto id = dsl::identifier(head, tail);

        return id
            .reserve(
                LEXY_KEYWORD("as", id),
                LEXY_KEYWORD("async", id),
                LEXY_KEYWORD("bool", id),
                LEXY_KEYWORD("borrow", id),
                LEXY_KEYWORD("char", id),
                LEXY_KEYWORD("constructor", id),

                LEXY_KEYWORD("enum", id),
                LEXY_KEYWORD("export", id),
                LEXY_KEYWORD("f32", id),
                LEXY_KEYWORD("f64", id),
                LEXY_KEYWORD("flags", id),
                LEXY_KEYWORD("from", id),

                LEXY_KEYWORD("func", id),
                LEXY_KEYWORD("future", id),
                LEXY_KEYWORD("import", id),
                LEXY_KEYWORD("include", id),
                LEXY_KEYWORD("interface", id),
                LEXY_KEYWORD("list", id),

                LEXY_KEYWORD("map", id),
                LEXY_KEYWORD("option", id),
                LEXY_KEYWORD("own", id),
                LEXY_KEYWORD("package", id),
                LEXY_KEYWORD("record", id),
                LEXY_KEYWORD("resource", id),

                LEXY_KEYWORD("result", id),
                LEXY_KEYWORD("s16", id),
                LEXY_KEYWORD("s32", id),
                LEXY_KEYWORD("s64", id),
                LEXY_KEYWORD("s8", id),
                LEXY_KEYWORD("static", id),

                LEXY_KEYWORD("stream", id),
                LEXY_KEYWORD("string", id),
                LEXY_KEYWORD("tuple", id),
                LEXY_KEYWORD("type", id),
                LEXY_KEYWORD("u16", id),
                LEXY_KEYWORD("u32", id),

                LEXY_KEYWORD("u64", id),
                LEXY_KEYWORD("u8", id),
                LEXY_KEYWORD("use", id),
                LEXY_KEYWORD("variant", id),
                LEXY_KEYWORD("with", id),
                LEXY_KEYWORD("world", id)
            );
    }();

    static constexpr auto value = lexy::as_string<std::string_view>;
};

auto constexpr id = dsl::p<ident>;

struct semvar {
    static constexpr auto rule = dsl::times<3>(dsl::integer<unsigned>, dsl::sep(dsl::period));

    static constexpr auto value = lexy::construct<SemVer>;
};

struct path {
    struct namespaces : lexy::transparent_production {
        static constexpr auto is_namespace = dsl::peek(id + dsl::colon);
        static constexpr auto rule = dsl::list(is_namespace >> id + dsl::colon);
        static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    };

    struct paths : lexy::transparent_production {
        static constexpr auto rule =
            dsl::opt(dsl::list(dsl::peek(id + dsl::slash) >> id + dsl::slash));
        static constexpr auto value = lexy::as_list<std::vector<std::string>>;
    };

    static constexpr auto rule = [] {
        auto version = dsl::opt(dsl::lit_c<'@'> >> dsl::p<semvar>);
        auto full_path = dsl::p<namespaces> + dsl::p<paths> + id + version;

        //return ident_only | dsl::else_ >> full_path;
        return full_path;
    }();

    static constexpr auto value = lexy::construct<Path>;
};

struct simple_path {
    static constexpr auto rule = [] { return dsl::peek_not(id + dsl::colon) >> id; }();
    static constexpr auto value = lexy::construct<Path>;
};

template <typename T, typename L>
struct type_map_t {
    static constexpr auto rule = [] { return L{}; }();
    static constexpr auto value = lexy::constant<Type>(Type{T{}}); //lexy::callback<Type>([] constexpr { return Type{T{}}; });
};

template <typename T, typename L>
constexpr auto type_map(T, L = {}) {
    return dsl::p<type_map_t<T, L>>;
};

struct type {
    static constexpr auto rule = [] {
        return type_map(U8, LEXY_LIT("u8"))
             | type_map(U16, LEXY_LIT("u16"))
             | type_map(U32, LEXY_LIT("u32"))
             | type_map(U64, LEXY_LIT("u64"))
             | type_map(S8, LEXY_LIT("s8"))
             | type_map(S16, LEXY_LIT("s16"))
             | type_map(S32, LEXY_LIT("s32"))
             | type_map(S64, LEXY_LIT("s64"))
             | type_map(F32, LEXY_LIT("f32"))
             | type_map(F64, LEXY_LIT("f64"))
             | type_map(Bool, LEXY_LIT("bool"))
            //  | type_map(CharType, LEXY_LIT("char"))
            //  | type_map(StringType, LEXY_LIT("string"))

             ;

    }();

    static constexpr auto value = lexy::forward<Type>;
};

struct named_type {
    static constexpr auto rule = [] {
        return dsl::p<ident> + dsl::colon + dsl::p<type>;
    }();

    static constexpr auto value = lexy::construct<NamedType>;
};

struct params {
    static constexpr auto rule =
        dsl::parenthesized.opt_list(dsl::p<named_type>, dsl::sep(dsl::comma));

    static constexpr auto value = lexy::as_list<std::vector<NamedType>>;
};

struct method {
    static constexpr auto rule = [] {
        auto results = dsl::opt(LEXY_LIT("->") >> dsl::p<type>);

        return id + dsl::colon + LEXY_LIT("func") + dsl::p<params> + results + dsl::semicolon;
    }();

    static constexpr auto value = lexy::construct<Method>;
};

struct resource {
    struct items : lexy::transparent_production {
        static constexpr auto rule = [] {
            auto items = dsl::p<method>;
            return dsl::curly_bracketed.opt_list(items);
        }();

        static constexpr auto value = lexy::as_list<std::vector<Method>>;
    };

    static constexpr auto rule = [] {
        return LEXY_LIT("resource") + id + dsl::p<items>;
    }();

    static constexpr auto value = lexy::construct<Resource>;

};

struct interface {
    struct items : lexy::transparent_production {
        static constexpr auto rule = [] {
            auto items = dsl::p<resource>;
            return dsl::curly_bracketed.opt_list(items);
        }();

        static constexpr auto value = lexy::as_list<std::vector<Resource>>;
    };

    static constexpr auto rule = [] {
        return LEXY_LIT("interface") + id + dsl::p<items>;
    }();

    static constexpr auto value = lexy::construct<Interface>;
};

struct witfile {
    struct items : lexy::transparent_production {
        static constexpr auto rule = [] {
            auto items = dsl::p<interface>;
            return dsl::terminator(dsl::eof).list(items);
        }();

        static constexpr auto value = lexy::as_list<std::vector<Interface>>;
    };

    static constexpr auto whitespace =
        dsl::ascii::blank | dsl::newline
        | LEXY_LIT("//") >> dsl::until(dsl::newline)
        | LEXY_LIT("/*") >> dsl::until(LEXY_LIT("*/"));
    static constexpr auto rule = []{
        auto pkg_decl = LEXY_LIT("package") + dsl::p<path> + dsl::semicolon;
        // return pkg_decl + dsl::terminator(dsl::eof).list(dsl::p<interface>);
        return pkg_decl + dsl::p<items>;
    }();

    static constexpr auto value = lexy::construct<WitFile>;

};
}