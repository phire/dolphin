#pragma once


#include "WitParser.h"

#include <vector>
#include <string_view>
#include <expected>
#include <optional>
#include <algorithm>
#include <type_traits>
#include <functional>
#include <version>

#include "Common/Logging/Log.h"

#include "Binding/Binding.h"

struct Token {
    enum class Type {
        Whitespace,
        Operator,
        Keyword,
        Integer,
        Identifier,
        End,
    };

    Type type;
    std::string_view text;
};

static constexpr std::string_view keywords[] = {
    "as",        "async",     "bool",      "borrow",    "char",      "constructor",
    "enum",      "export",    "f32",       "f64",       "flags",     "from",
    "func",      "future",    "import",    "include",   "interface", "list",
    "map",       "option",    "own",       "package",   "record",    "resource",
    "result",    "s16",       "s32",       "s64",       "s8",        "static",
    "stream",    "string",    "tuple",     "type",      "u16",       "u32",
    "u64",       "u8",        "use",       "variant",   "with",      "world",
};

constexpr bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

constexpr bool is_ident_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

constexpr bool is_keyword(std::string_view text)
{
    for (std::string_view kw : keywords)
    {
        if (kw == text)
            return true;
    }

    return false;
}



using parse_error = std::tuple<std::string_view, std::string_view>;

#define TRY_OR_RETURN(expr) ({ auto&& _result = expr; if (!_result) return std::unexpected(_result.error()); *_result;  })



struct Input : public std::string_view {
    constexpr Input(std::string_view input) : std::string_view(input) {
        consume_whitespace();
    }

    constexpr bool match(std::string_view text) {
        if (starts_with(text)) {
            if (is_keyword(text)) {
                // If the text is a keyword, we need to make sure that the next character is not a valid identifier character.
                if (size() > text.size() && is_ident_char((*this)[text.size()])) {
                    return false;
                }
            }

            remove_prefix(text.size());
            consume_whitespace();
            return true;
        }
        return false;
    }

    constexpr std::expected<std::monostate, parse_error> expect(std::string_view text) {
        if (match(text)) {
            return consume_whitespace().transform([](auto) { return std::monostate{}; });
        }

        return std::unexpected(std::make_tuple("Expected token", *this));
    }

    constexpr std::expected<size_t, parse_error> integer() {
        size_t len = 0;
        while (len < size() && is_digit((*this)[len])) {
            ++len;
        }

        if (len == 0) {
            return std::unexpected(std::make_tuple("Expected integer", *this));
        }

        size_t value = 0;
        for (size_t i = 0; i < len; ++i) {
            value = value * 10 + static_cast<size_t>((*this)[i] - '0');
        }

        remove_prefix(len);

        return consume_whitespace().transform([value](auto) { return value; });
    }

    constexpr std::expected<std::string_view, parse_error> consume(std::size_t n) {
        std::string_view result = substr(0, n);
        remove_prefix(n);

        return consume_whitespace().transform([result](auto) { return result; });
    }

    constexpr size_t first_non_ident() {
        size_t len = 0;
        while (len < size() && is_ident_char((*this)[len])) {
            ++len;
        }
        return len;
    }

private:

    constexpr std::expected<bool, parse_error> consume_whitespace() {
        auto orig_size = size();
        while (!empty()) {
            switch (front()) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    remove_prefix(1);
                    continue;
                case '/':
                    if (size() > 1) {
                        if ((*this)[1] == '/') {
                            const size_t newline_pos = find('\n');
                            remove_prefix(std::min(newline_pos + 1, size()));
                            continue;
                        } else if ((*this)[1] == '*') {
                            if (auto result = consume_multiline_comment(); !result) {
                                return std::unexpected(result.error());
                            }
                            continue;
                        }
                    }
                    [[fallthrough]];
                default:
            }
            break;
        }
        return size() != orig_size;
    }

    // Wit allows for nested multiline comments, so we need to deal with that.
    constexpr std::expected<void, parse_error> consume_multiline_comment() {
        auto error_pos = *this;
        remove_prefix(2); // remove starting "/*"
        int depth = 1;
        while (!empty() && depth > 0) {
            size_t open_pos = find_first_of("/*");
            size_t close_pos = find("*/");
            if (open_pos == std::string_view::npos || close_pos < open_pos) {
                if (close_pos == std::string_view::npos) {
                    // Unterminated comment
                    remove_prefix(size());
                }
                remove_prefix(close_pos + 2);
                depth--;
            } else {
                error_pos = *this;
                remove_prefix(open_pos + 2);
                depth++;
            }
        }

        if (depth > 0) {
            // Rewind to error pos (because some callers ignore the error)
            *this = error_pos;
            return std::unexpected(std::make_tuple("Unterminated comment", error_pos));
        }
        return {};
    }
};

// template<typename Parser, typename Until>
// struct RepeatUntil {
//     Parser parser;
//     Until until;
//     RepeatUntil(Parser p, Until u) : parser(p), until(u) {}

//     template<typename Tuple, typename Result = decltype(std::tuple_cat(Tuple(), std::make_tuple(*std::declval<std::expected<typename std::invoke_result_t<Parser, Input&>::value_type, parse_error>>())))>
//     constexpr std::expected<Result, parse_error> operator()(Tuple prev, Input& input) {
//         auto result = parser(input);

//         if (result.has_value()) {
//             auto tuple = std::tuple_cat(prev, std::make_tuple(*result));
//             if (until(input)) {
//                 return std::expected<decltype(tuple)>(std::move(tuple));
//             } else {
//                 return operator()(tuple, input);
//             }
//         }
//         return std::unexpected(result.error());
//     }
// };

// constexpr auto repeat(Input& input, auto parser, auto until) {
//     // auto iter = [&]<typename Tuple, typename Result>(Tuple prev) -> std::expected<Result, parse_error> {
//     //     auto result = parser(input);
//     //     if (result.has_value()) {
//     //         if (until(input)) {
//     //             return std::expected(std::tuple_cat(prev, std::make_tuple(*result)));
//     //         } else {
//     //             return iter(std::tuple_cat(prev, std::make_tuple(*result)));
//     //         }
//     //     }
//     //     return std::unexpected(result.error());
//     // };
//     auto iter = RepeatUntil(parser, until);
//     return iter(std::tuple<>(), input);
// }

template<typename T>
constexpr std::expected<T, parse_error> wrapped(Input& input, std::string_view open, std::expected<T, parse_error>(parse_func)(Input&), std::string_view close) {
    TRY_OR_RETURN(input.expect(open));
    auto result = parse_func(input);
    TRY_OR_RETURN(input.expect(close));
    return result;
}

template<typename Ret, typename Parser>
constexpr auto choice(Input& input, Parser parser) -> std::expected<Ret, parse_error>
{
    if constexpr (std::is_invocable_v<Parser, Input&>) {
        auto result = parser(input);
        if (result.has_value()) {
            if (result.value().has_value()) {
                return {result.value().value()};
            }
        } else {
            return std::unexpected(result.error());
        }
    } else if constexpr (std::is_convertible_v<Parser, Ret>) {
        return {parser};
    } else if constexpr (std::is_convertible_v<Parser, std::unexpected<parse_error>>) {
        // If the parser is a string, it's an error
        return std::unexpected(parser.error());
    }
    return std::unexpected(std::make_tuple("None of the choices matched", input));
}

template<typename Ret, typename Parser, typename... Parsers>
constexpr auto choice(Input& input, Parser parser, Parsers... rest) -> std::expected<Ret, parse_error>
{
    auto result = parser(input);
    if (result.has_value()) {
        if (result.value().has_value()) {
            return {result.value().value()};
        }
        return choice<Ret>(input, std::forward<Parsers>(rest)...);
    }
    return std::unexpected(result.error());
}



constexpr std::expected<std::string, parse_error> identifier(Input& input) {

    // identifiers prefixed with % are allowed to be keywords.
    bool raw = input.match("%");

    size_t len = input.first_non_ident();

    if (len == 0) {
        return std::unexpected(std::make_tuple("Expected identifier", input));
    }

    if (!raw && is_keyword(input.substr(0, len))) {
        return std::unexpected(std::make_tuple("Expected identifier, but found keyword", input));
    }

    return input.consume(len).transform([](auto sv) { return std::string(sv); });
}

constexpr std::expected<std::string_view, parse_error> keyword(Input& input) {
    size_t len = input.first_non_ident();

    if (len == 0 || !is_keyword(input.substr(0, len))) {
        return std::unexpected(std::make_tuple("Expected keyword", input));
    }

    return input.consume(len);
}

constexpr std::expected<Wit::SemVer, parse_error> semver(Input& input)
{
    unsigned values[3] = {0, 0, 0};

    for (size_t i = 0;;) {
        auto value = TRY_OR_RETURN(input.integer());
        values[i] = static_cast<unsigned>(value);
        if (++i == 3) {
            return Wit::SemVer{values[0], values[1], values[2]};
        }
        TRY_OR_RETURN(input.expect("."));
    }
}

// use-path ::= id
//            | id ':' id '/' id ('@' valid-semver)?
//           | ( id ':' )+ id ( '/' id )+ ('@' valid-semver)?
constexpr std::expected<Wit::Path, parse_error> path(Input& input, bool allow_single) {
    Wit::Path result;

    auto ident = TRY_OR_RETURN(identifier(input));
    while (input.match(":")) {
        result.namespaces.emplace_back(std::move(ident));
        ident = TRY_OR_RETURN(identifier(input));
    }

    if (result.namespaces.empty()) {
        // If there is no namespaces, we can allow a single identifier as a valid path.
        if (allow_single) {
            result.name = std::move(ident);
            return result;
        } else {
            return std::unexpected(std::make_tuple("Expected namespace", input));
        }
    }

    // Optional path
    while (input.match("/")) {
        result.path.emplace_back(std::move(ident));
        ident = TRY_OR_RETURN(identifier(input));
    }

    // then package name
    result.name = std::move(ident);

    // And optional semantic version: @1.2.3
    if (input.match("@")) {
        result.version = TRY_OR_RETURN(semver(input));
    }

    return result;
}

// matches package declaration, for example:
//      package namespace:package[@1.2.3]
// or
//      package namespace:namespace2:path/to/package[@1.2.3]
constexpr std::expected<std::optional<Wit::PackageDecl>, parse_error> package_decl(Input& input)
{
    if (!input.match("package"))
        return std::nullopt;

    return path(input, false).transform([](auto p) { return Wit::PackageDecl(p); });
}

// gate ::= gate-item*
// gate-item ::= unstable-gate
//             | since-gate
//             | deprecated-gate

// unstable-gate ::= '@unstable' '(' feature-field ')'
// since-gate ::= '@since' '(' version-field ')'
// deprecated-gate ::= '@deprecated' '(' version-field ')'

// feature-field ::= 'feature' '=' id
// version-field ::= 'version' '=' <valid semver>
constexpr std::expected<Wit::Gate, parse_error> gate(Input& input) {
    using Type = Wit::Gate::Type;
    if (input.match("@unstable")) {
        std::string feature_name = TRY_OR_RETURN(wrapped(input, "(", identifier, ")"));
        return Wit::Gate(Type::Unstable, feature_name);
    }
    if (input.match("@since")) {
        Wit::SemVer version = TRY_OR_RETURN(wrapped(input, "(", semver, ")"));
        return Wit::Gate(Type::Since, version);
    }
    if (input.match("@deprecated")) {
        Wit::SemVer version = TRY_OR_RETURN(wrapped(input, "(", semver, ")"));
        return Wit::Gate(Type::Deprecated, version);
    }

    return {};
}

// use-path ::= id
//            | id ':' id '/' id ('@' valid-semver)?
//            | ( id ':' )+ id ( '/' id )+ ('@' valid-semver)?
constexpr std::expected<Wit::UsePath, parse_error> use_path(Input& input) {
    return path(input, true).transform([](auto p) { return Wit::UsePath(p); });
}

// ty ::= 'u8' | 'u16' | 'u32' | 'u64'
//      | 's8' | 's16' | 's32' | 's64'
//      | 'f32' | 'f64'
//      | 'char'
//      | 'bool'
//      | 'string'
//      | tuple
//      | list
//      | option
//      | result
//      | map
//      | handle
//      | future
//      | stream
//      | id
constexpr std::expected<Wit::Ty, parse_error> ty(Input& input) {
    auto kw_result = keyword(input);
    if (!kw_result) {
        return identifier(input).transform([](auto id) {
            return Wit::Ty{Wit::Ty::Kind::Id, id};
        });
    }

    std::string_view kw = kw_result.value();

    using Kind = Wit::Ty::Kind;

    if (kw == "u8") return Wit::Ty{Kind::U8};
    if (kw == "u16") return Wit::Ty{Kind::U16};
    if (kw == "u32") return Wit::Ty{Kind::U32};
    if (kw == "u64") return Wit::Ty{Kind::U64};
    if (kw == "s8") return Wit::Ty{Kind::S8};
    if (kw == "s16") return Wit::Ty{Kind::S16};
    if (kw == "s32") return Wit::Ty{Kind::S32};
    if (kw == "s64") return Wit::Ty{Kind::S64};
    if (kw == "f32") return Wit::Ty{Kind::F32};
    if (kw == "f64") return Wit::Ty{Kind::F64};
    if (kw == "char") return Wit::Ty{Kind::Char};
    if (kw == "bool") return Wit::Ty{Kind::Bool};
    if (kw == "string") return Wit::Ty{Kind::String};

    // tuple ::= 'tuple' '<' tuple-list '>'
    // tuple-list ::= ty
    //              | ty ',' tuple-list?
    if (kw == "tuple") {
        TRY_OR_RETURN(input.expect("<"));
        std::vector<Wit::Ty> types;
        do {
            types.push_back(TRY_OR_RETURN(ty(input)));
            input.match(","); // Optional trailing comma
        } while (!input.match(">"));
        return Wit::Ty{Kind::Tuple, std::move(types)};
    }

    // list ::= 'list' '<' ty '>'
    //    | 'list' '<' ty ',' uint '>'
    if (kw == "list") {
        TRY_OR_RETURN(input.expect("<"));
        auto inner = TRY_OR_RETURN(ty(input));
        if (input.match(",")) {
            auto size = TRY_OR_RETURN(input.integer());
            (void)size;
            TRY_OR_RETURN(input.expect(">"));
            return Wit::Ty{Kind::FixedLengthList, {std::move(inner), Wit::Ty{Kind::U32}}};
        }
        TRY_OR_RETURN(input.expect(">"));
        return Wit::Ty{Kind::List, {std::move(inner)}};
    }

    // option ::= 'option' '<' ty '>'
    if (kw == "option") {
        return Wit::Ty{Kind::Option, {TRY_OR_RETURN(wrapped(input, "<", ty, ">"))}};
    }


    // result ::= 'result' '<' ty ',' ty '>'
    //          | 'result' '<' '_' ',' ty '>'
    //          | 'result' '<' ty '>'
    //          | 'result'
    if (kw == "result") {
        std::vector<Wit::Ty> params{ Wit::Ty{Kind::Ignore}, Wit::Ty{Kind::Ignore} };

        if (input.match("<")) {
            if (!input.match("_")) {
                params[0] = TRY_OR_RETURN(ty(input));
            }
            if (input.match(",")) {
                params[1] = TRY_OR_RETURN(ty(input));
            } else if (params[0].kind == Kind::Ignore) {
                return std::unexpected(std::make_tuple("result<_> is not valid, use result instead", input));
            }
            TRY_OR_RETURN(input.expect(">"));
        }

        return Wit::Ty{Kind::Result, std::move(params)};
    }

    // map ::= 'map' '<' kt ',' ty '>'
    // kt ::= 'u8' | 'u16' | 'u32' | 'u64'
    //  | 's8' | 's16' | 's32' | 's64'
    //  | 'char' | 'bool' | 'string'
    if (kw == "map") {
        TRY_OR_RETURN(input.expect("<"));
        auto key_pos = input;
        auto key = TRY_OR_RETURN(ty(input));
        if (key.kind != Kind::U8 && key.kind != Kind::U16 && key.kind != Kind::U32 && key.kind != Kind::U64 &&
            key.kind != Kind::S8 && key.kind != Kind::S16 && key.kind != Kind::S32 && key.kind != Kind::S64 &&
            key.kind != Kind::Char && key.kind != Kind::Bool && key.kind != Kind::String) {
            return std::unexpected(std::make_tuple("map key type must be a primitive type", key_pos));
        }
        TRY_OR_RETURN(input.expect(","));
        auto value = TRY_OR_RETURN(ty(input));
        TRY_OR_RETURN(input.expect(">"));
        return Wit::Ty{Kind::Map, {std::move(key), std::move(value)}};
    }

    if (kw == "borrow") {
        return Wit::Ty{Kind::Handle, {TRY_OR_RETURN(wrapped(input, "<", identifier, ">"))}};
    }

    // future ::= 'future' '<' ty '>'
    //          | 'future'
    if (kw == "future") {
        std::vector<Wit::Ty> params;
        if (input.match("<")) {
            params.emplace_back(TRY_OR_RETURN(ty(input)));
            TRY_OR_RETURN(input.expect(">"));
        }
        return Wit::Ty{Kind::Future, std::move(params)};
    }

    // stream ::= 'stream' '<' ty '>'
    //          | 'stream'
    if (kw == "stream") {
        std::vector<Wit::Ty> params;
        if (input.match("<")) {
            params.emplace_back(TRY_OR_RETURN(ty(input)));
            TRY_OR_RETURN(input.expect(">"));
        }
        return Wit::Ty{Kind::Stream, std::move(params)};
    }

    return std::unexpected(
        std::make_tuple("Expected type keyword",
        input
    ));
}

// param-list ::= '(' named-type-list ')'
// named-type-list ::= ϵ
//                  | named-type ( ',' named-type )*
// named-type ::= id ':' ty
constexpr std::expected<std::vector<Wit::NamedType>, parse_error> param_list(Input& input) {
    std::vector<Wit::NamedType> types;

    TRY_OR_RETURN(input.expect("("));
    if (input.match(")")) {
        // ϵ (Empty list)
        return types;
    }
    do {
        auto name = TRY_OR_RETURN(identifier(input));
        TRY_OR_RETURN(input.expect(":"));
        auto type = TRY_OR_RETURN(ty(input));
        types.push_back({std::move(name), std::move(type)});
        // Trailing comma is not allowed
    } while (input.match(","));
    TRY_OR_RETURN(input.expect(")"));
    return types;
}

// func-type ::= 'async'? 'func' param-list result-list
constexpr std::expected<Wit::FuncType, parse_error> func_type(Input& input) {
    Wit::FuncType func_type;
    func_type.async = input.match("async");

    TRY_OR_RETURN(input.expect("func"));

    func_type.params = TRY_OR_RETURN(param_list(input));
    if (input.match("->")) {
        func_type.results = {{TRY_OR_RETURN(ty(input))}};
    }

    return func_type;
}


// func-item ::= id ':' func-type ';'
constexpr std::expected<std::optional<Wit::Func>, parse_error> func_item(Input& input, Wit::Gate& gate) {
    Wit::Func func;
    func.gate = std::move(gate);

    func.name = TRY_OR_RETURN(identifier(input));
    func.ty = TRY_OR_RETURN(wrapped(input, ":", func_type, ";"));

    return func;
}

// resource-item ::= 'resource' id ';'
//                | 'resource' id '{' resource-method* '}'
// resource-method ::= func-item
//                  | id ':' 'static' func-type ';'
//                  | 'constructor' param-list ';'
constexpr std::expected<std::optional<Wit::Resource>, parse_error> resource_item(Input& input, Wit::Gate& gate) {
    Wit::Resource resource;

    if (!input.match("resource"))
        return std::nullopt;

    resource.gate = std::move(gate);
    resource.name = TRY_OR_RETURN(identifier(input));

    if (input.match(";")) {
        return resource;
    }

    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        if (auto method = TRY_OR_RETURN(func_item(input, resource.gate))) {
            resource.methods.push_back(std::move(*method));
        } else if (input.match("constructor")) {
            Wit::FuncType ty;
            ty.params = TRY_OR_RETURN(param_list(input));
            ty.results = {Wit::Ty{Wit::Ty::Kind::Id, resource.name}};
            resource.methods.push_back(
                Wit::Func{resource.gate, "", std::move(ty)});
            TRY_OR_RETURN(input.expect(";"));
        } else {
            auto name = TRY_OR_RETURN(identifier(input));
            TRY_OR_RETURN(input.expect(":"));
            TRY_OR_RETURN(input.expect("static"));
            Wit::FuncType ty = TRY_OR_RETURN(func_type(input));
            ty.static_ = true;
            resource.methods.push_back(
                Wit::Func{resource.gate, std::move(name), std::move(ty)});
            TRY_OR_RETURN(input.expect(";"));
        }
    }
    return resource;
}

// variant-items ::= 'variant' id '{' variant-cases '}'
// variant-cases ::= variant-case
//                 | variant-case ',' variant-cases?
// variant-case ::= id
//                | id '(' ty ')'
constexpr std::expected<std::optional<Wit::Varient>, parse_error> variant_item(Input& input, Wit::Gate& gate) {
    Wit::Varient variant;

    if (!input.match("variant"))
        return std::nullopt;

    variant.gate = std::move(gate);
    variant.name = TRY_OR_RETURN(identifier(input));

    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        auto name = TRY_OR_RETURN(identifier(input));
        if (input.match("(")) {
            auto type = TRY_OR_RETURN(ty(input));
            variant.cases.emplace_back(std::move(name), std::move(type));
            TRY_OR_RETURN(input.expect(")"));
        } else {
            variant.cases.emplace_back(std::move(name), Wit::Ty{Wit::Ty::Kind::Ignore});
        }
        input.match(","); // Optional trailing comma
    }
    return variant;
}

// record-item ::= 'record' id '{' record-fields '}'
// record-fields ::= record-field
//                 | record-field ',' record-fields?
// record-field ::= id ':' ty
constexpr std::expected<std::optional<Wit::Record>, parse_error> record_item(Input& input, Wit::Gate& gate) {
    Wit::Record record;

    if (!input.match("record"))
        return std::nullopt;

    record.gate = std::move(gate);
    record.name = TRY_OR_RETURN(identifier(input));

    TRY_OR_RETURN(input.expect("{"));
    do {
        auto name = TRY_OR_RETURN(identifier(input));
        TRY_OR_RETURN(input.expect(":"));
        auto type = TRY_OR_RETURN(ty(input));
        record.fields.emplace_back(std::move(name), std::move(type));
        input.match(","); // Optional trailing comma
    } while (!input.match("}"));
    return record;
}

// flags-items ::= 'flags' id '{' flags-fields '}'
// flags-fields ::= id
//                | id ',' flags-fields?
constexpr std::expected<std::optional<Wit::Flags>, parse_error> flags_item(Input& input, Wit::Gate& gate) {
    Wit::Flags flags;

    if (!input.match("flags"))
        return std::nullopt;

    flags.gate = std::move(gate);
    flags.name = TRY_OR_RETURN(identifier(input));

    TRY_OR_RETURN(input.expect("{"));
    do {
        auto name = TRY_OR_RETURN(identifier(input));
        flags.fields.push_back(std::move(name));
        input.match(","); // Optional trailing comma
    } while (!input.match("}"));
    return flags;
}

// enum-items ::= 'enum' id '{' enum-cases '}'
// enum-cases ::= id
//              | id ',' enum-cases?
constexpr std::expected<std::optional<Wit::Enum>, parse_error> enum_item(Input& input, Wit::Gate& gate) {
    Wit::Enum result;

    if (!input.match("enum"))
        return std::nullopt;

    result.gate = std::move(gate);
    result.name = TRY_OR_RETURN(identifier(input));

    TRY_OR_RETURN(input.expect("{"));
    do {
        auto name = TRY_OR_RETURN(identifier(input));
        result.cases.push_back(std::move(name));
        input.match(","); // Optional trailing comma
    } while (!input.match("}"));
    return result;
}

// type-item ::= 'type' id '=' ty ';'
// AKA type-alias
constexpr std::expected<std::optional<Wit::TypeAlias>, parse_error> type_item(Input& input, Wit::Gate& gate) {
    Wit::TypeAlias type;

    if (!input.match("type"))
        return std::nullopt;


    type.gate = std::move(gate);
    type.name = TRY_OR_RETURN(identifier(input));
    TRY_OR_RETURN(input.expect("="));
    type.ty = TRY_OR_RETURN(ty(input));
    TRY_OR_RETURN(input.expect(";"));

    return type;
}

// typedef-item ::= resource-item
//                | variant-items
//                | record-item
//                | flags-items
//                | enum-items
//                | type-item
template<typename T>
constexpr std::expected<std::optional<T>, parse_error> typedef_item(Input& input, Wit::Gate& gate) {
    return choice<std::optional<T>>(input,
        std::bind_back(resource_item, gate),
        std::bind_back(variant_item, gate),
        std::bind_back(record_item, gate),
        std::bind_back(flags_item, gate),
        std::bind_back(enum_item, gate),
        std::bind_back(type_item, gate),
        std::nullopt
    );
}


// use-item ::= 'use' use-path '.' '{' use-names-list '}' ';'
// use-names-list ::= use-names-item
//                  | use-names-item ',' use-names-list?
// use-names-item ::= id
//                  | id 'as' id
constexpr std::expected<std::optional<Wit::Use>, parse_error> use_item(Input& input, Wit::Gate& gate) {
    Wit::Use use;
    if (!input.match("use"))
        return std::nullopt;

    use.gate = std::move(gate);

    use.path = TRY_OR_RETURN(use_path(input));
    TRY_OR_RETURN(input.expect("."));
    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        Wit::UseMap map { TRY_OR_RETURN(identifier(input)) };

        if (input.match("as")) {
            map.alias = TRY_OR_RETURN(identifier(input));
        }

        TRY_OR_RETURN(input.expect(","));
    }
    TRY_OR_RETURN(input.expect(";"));

    return use;
}


// interface-item ::= gate 'interface' id '{' interface-items* '}'
// interface-items ::= gate interface-definition
constexpr std::expected<std::vector<Wit::InterfaceItem>, parse_error> interface_items(Input& input) {
    std::vector<Wit::InterfaceItem> items;

    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        Wit::Gate def_gate = TRY_OR_RETURN(gate(input));

        items.emplace_back(TRY_OR_RETURN(choice<Wit::InterfaceItem>(input,
            std::bind_back(typedef_item<Wit::InterfaceItem>, def_gate),
            std::bind_back(use_item, def_gate),
            std::bind_back(func_item, def_gate),
            std::unexpected(std::make_tuple("Expected interface item", input))
        )));
    }
    return items;
}

constexpr std::expected<std::optional<Wit::Interface>, parse_error> interface_item(Input& input) {
    Wit::Interface interface;
    interface.gate = TRY_OR_RETURN(gate(input));

    if (interface.gate.is_none()) {
        if (!input.match("interface"))
            return std::nullopt;
    } else {
        // If we have a gate, we must have an interface
        TRY_OR_RETURN(input.expect("interface"));
    }

    interface.name = TRY_OR_RETURN(identifier(input));
    interface.items = TRY_OR_RETURN(interface_items(input));

    return interface;
}


// extern-type ::= func-type ';'
//               | 'interface' '{' interface-items* '}'
//               | use-path ';'
constexpr std::expected<Wit::ExternType, parse_error> extern_type(Input& input, Wit::Gate gate) {
    // to prevent ambiguity, we first try to peek to see if we have a namespaced identifier without any whitespace.

    size_t ident_len = input.first_non_ident();
    if (ident_len && input.size() > ident_len + 2 && input[ident_len] == ':') {
        char next = input[ident_len + 1];
        if (is_ident_char(next)) {
            auto path = TRY_OR_RETURN(use_path(input));
            TRY_OR_RETURN(input.expect(";"));
            return Wit::Rename{std::move(gate), std::move(path), {}};
        }
    }

    auto name = TRY_OR_RETURN(identifier(input));

    if (input.match(";")) {
        return Wit::Rename{std::move(gate), {Wit::Path{.name = std::move(name)}}, {}};
    }

    TRY_OR_RETURN(input.expect(":"));

    if (input.starts_with("interface")) {
        auto items = TRY_OR_RETURN(interface_items(input));
        TRY_OR_RETURN(input.expect(";"));
        return Wit::Interface {
            .gate = std::move(gate),
            .name = std::move(name),
            .items = std::move(items),
        };
    } else if (input.starts_with("async") || input.starts_with("func")) {
        auto ty = TRY_OR_RETURN(func_type(input));
        TRY_OR_RETURN(input.expect(";"));
        return Wit::Func{
            .gate = std::move(gate),
            .name = std::move(name),
            .ty = std::move(ty),
        };
    } else {
        auto path = TRY_OR_RETURN(use_path(input));
        TRY_OR_RETURN(input.expect(";"));
        return Wit::Rename{
            .gate = std::move(gate),
            .path = std::move(path),
            .name = std::move(name),
        };
    }
}


// export-item ::= 'export' id ':' extern-type
//               | 'export' use-path ';'
constexpr std::expected<std::optional<Wit::Export>, parse_error> export_item(Input& input, Wit::Gate& gate) {
    Wit::Export export_item;
    export_item.gate = std::move(gate);
    if (!input.match("export"))
        return std::nullopt;

    return extern_type(input, export_item.gate).transform([&](auto item) {
        export_item.item = std::move(item);
        return export_item;
    });
}

// import-item ::= 'import' id ':' extern-type
//               | 'import' use-path ';'
constexpr std::expected<std::optional<Wit::Import>, parse_error> import_item(Input& input, Wit::Gate& gate) {
    Wit::Import import_item;
    import_item.gate = std::move(gate);
    if (!input.match("import"))
        return std::nullopt;

    return extern_type(input, import_item.gate).transform([&](auto item) {
        import_item.item = std::move(item);
        return import_item;
    });
}

// world-item ::= gate 'world' id '{' world-items* '}'
// world-items ::= gate world-definition
// world-definition ::= export-item
//                    | import-item
//                    | use-item
//                    | typedef-item
//                    | include-item

constexpr std::expected<std::optional<Wit::World>, parse_error> world_item(Input& input) {
    Wit::World world;
    world.gate = TRY_OR_RETURN(gate(input));

    if (world.gate.is_none()) {
        if (!input.match("world"))
            return std::nullopt;
    } else {
        // If we have a gate, we must have a world
        TRY_OR_RETURN(input.expect("world"));
    }

    world.name = TRY_OR_RETURN(identifier(input));

    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        Wit::Gate def_gate = TRY_OR_RETURN(gate(input));

        auto result = choice<Wit::WorldDefinition>(input,
            std::bind_back(export_item, def_gate),
            std::bind_back(import_item, def_gate),
            std::bind_back(use_item, def_gate),
            std::bind_back(typedef_item<Wit::WorldDefinition>, def_gate),
            std::unexpected(std::make_tuple("Expected world item", input))
        );

        if (!result)
            return std::unexpected(result.error());

        world.items.emplace_back(std::move(result.value()));
    }

    return world;
}

// package-items ::= toplevel-use-item | interface-item | world-item
constexpr std::expected<Wit::PackageItem, parse_error> package_items(Input& input) {
    // TODO: toplevel-use-item
    std::expected<Wit::PackageItem, parse_error> item = choice<Wit::PackageItem>(input,
        interface_item,
        world_item,
        std::unexpected(std::make_tuple("Expected package item", input))
    );
    return item;
}

// nested-package-definition ::= package-decl '{' package-items* '}'
constexpr std::expected<std::optional<Wit::Nested>, parse_error> nested_package_definition(Input& input) {
    Wit::Nested nested;
    auto decl = TRY_OR_RETURN(package_decl(input));
    if (!decl) {
        return std::nullopt;
    }

    TRY_OR_RETURN(input.expect("{"));
    while (!input.match("}")) {
        nested.items.push_back(TRY_OR_RETURN(package_items(input)));
    }
    return nested;
}


// wit-file ::= (package-decl ';')? (package-items | nested-package-definition)*
//template<typename Result>
constexpr std::expected<std::vector<Wit::PackageItem>, parse_error> wit_file(Input& input) {
    TRY_OR_RETURN(package_decl(input));
    TRY_OR_RETURN(input.expect(";"));

    std::vector<Wit::PackageItem> items;

    // return repeat(input, package_items, [](Input& input) { return input.empty(); });

    while (!input.empty()) {
        // if (auto nested = TRY_OR_RETURN(nested_package_definition(input))) {
        //     // Nested package blocks currently flatten into top-level items.
        //     for (auto& nested_item : nested->items) {
        //         items.push_back(std::move(nested_item));
        //     }
        // }
        // else {
            items.push_back(TRY_OR_RETURN(package_items(input)));
        // }
    }

    return items;
}

void print_error(parse_error error, const std::string_view input) {
    auto [error_message, remaining_input] = error;
    fmt::print(stderr, "Error parsing dolphin.wit\n");
    size_t error_pos = input.size() - remaining_input.size();
    size_t error_line_num = std::count(input.begin(), input.begin() + error_pos, '\n') + 1;
    size_t start_of_line = input.rfind('\n', error_pos);
    if (start_of_line == std::string_view::npos)
        start_of_line = 0;
    else
        start_of_line += 1;
    size_t end_of_line = input.find('\n', error_pos);
    if (end_of_line == std::string_view::npos)
        end_of_line = input.size();
    size_t line_offset = error_pos - start_of_line;
    auto lines = input.substr(0, end_of_line);
    fmt::print(stderr, "Error at line {}:\n{}\n", error_line_num, lines);

    fmt::print(stderr, "{:>{}}^\n", "", line_offset);
    fmt::print(stderr, "{:>{}}{}\n", "", line_offset, error_message);
}



static constexpr std::string_view get_embedded_wit()
{
    static constexpr char embedded_wit[] = {
#ifdef __cpp_pp_embed
    #embed "dolphin.wit"
#else
    #include "dolphin_wit.h"
#endif
    };
    return std::string_view(embedded_wit, sizeof(embedded_wit));
}

// static consteval std::optional<std::vector<Wit::PackageItem>> parse_embedded_wit() {
//     Input input(get_embedded_wit());
//     auto result = wit_file(input);
//     if (input.empty() && result.has_value()) {
//         return { result.value() };
//     }
//     return std::nullopt;
// }
// static_assert(parse_embedded_wit().has_value(), "Failed to parse dolphin.wit");

// static consteval std::vector<Wit::PackageItem> parse_wit() {
//     auto parser =  [] consteval -> std::optional<std::vector<Wit::PackageItem>> {
//         Input input(get_embedded_wit());
//         auto result = wit_file(input);
//         if (input.empty() && result.has_value()) {
//             return { result.value() };
//         }
//         return std::nullopt;
//     };

//     static_assert(parser().has_value(), "Failed to parse dolphin.wit");
//     return parser().value();
// }
