

#pragma once

#include <lexy/dsl.hpp>
#include <lexy/callback/object.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/grammar.hpp>

#include "WitFile.h"


#include "Common/SmallVector.h"

namespace WitLexy
{
namespace grammar
{


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

struct type;

struct list_type {
    static constexpr auto rule = [] {
        return dsl::peek(LEXY_LIT("list")) >> LEXY_LIT("list") + dsl::angle_bracketed(dsl::recurse<type>);
    }();

    static constexpr auto value = lexy::callback<Type>([] (Type&& t) constexpr {
        //return std::make_unique<ListType>(ListType{std::move(t)});
        Type result;
        result.type_tree.emplace_back(TypeKind::ListStart);
        for (auto&& kind : t.type_tree) {
            result.type_tree.emplace_back(kind);
        }
        result.type_tree.emplace_back(TypeKind::ListEnd);
        return result;
    });
};

struct type {
    template<TypeKind t, auto L>
    struct type_map_t {
        static constexpr auto rule = [] { return L; }();
        static constexpr auto value = lexy::callback<Type>([]() { return Type({t}); });
        // static constexpr auto value = lexy::constant<Type>(Type(t));
    };

    static constexpr auto rule = [] {
        return dsl::p<type_map_t<TypeKind::U8, LEXY_LIT("u8")>>
             | dsl::p<type_map_t<TypeKind::U16, LEXY_LIT("u16")>>
             | dsl::p<type_map_t<TypeKind::U32, LEXY_LIT("u32")>>
             | dsl::p<type_map_t<TypeKind::U64, LEXY_LIT("u64")>>
             | dsl::p<type_map_t<TypeKind::S8, LEXY_LIT("s8")>>
             | dsl::p<type_map_t<TypeKind::S16, LEXY_LIT("s16")>>
             | dsl::p<type_map_t<TypeKind::S32, LEXY_LIT("s32")>>
             | dsl::p<type_map_t<TypeKind::S64, LEXY_LIT("s64")>>
             | dsl::p<type_map_t<TypeKind::F32, LEXY_LIT("f32")>>
             | dsl::p<type_map_t<TypeKind::F64, LEXY_LIT("f64")>>
             | dsl::p<type_map_t<TypeKind::Bool, LEXY_LIT("bool")>>
             | dsl::p<type_map_t<TypeKind::CharType, LEXY_LIT("char")>>
             | dsl::p<type_map_t<TypeKind::StringType, LEXY_LIT("string")>>
             //| dsl::p<list_type>
            ;

    }();

    //static constexpr auto value = lexy::forward<Type>;
    static constexpr auto value = lexy::forward<Type>;
};

struct named_type {
    static constexpr auto rule = [] { return dsl::p<ident> + dsl::colon + dsl::p<type>; }();

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
} // namespace grammar
} // namespace WitLexy