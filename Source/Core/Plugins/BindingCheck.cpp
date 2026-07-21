

#include "Common/Logging/Log.h"
#include "Plugins/WitLexy.h"

#include "Binding.h"
#include "BindingCheck.h"
#include "FnTraits.h"


#include "Plugins/WitLexy.h"


#include <lexy/input/string_input.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/action/validate.hpp>
#include <lexy_ext/report_error.hpp>

#include <source_location>
#include <ranges>


// Custom formatter for source_location, prints file:line:column which many IDEs will extract.
template <>
struct fmt::formatter<std::source_location>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const std::source_location& loc, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}:{}:{}", loc.file_name(), loc.line(), loc.column());
    }
};


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

template <typename T, typename StrT = empty_type_string>
struct TypeTree {
    using Type = T;
    using Name = StrT;

    template <typename U>
    static constexpr void is_same() {
        static_assert(std::is_same_v<Type, U>, "Type mismatch with dolphin.wit");
    }

    template <typename NewStrT>
    static constexpr TypeTree<Type, NewStrT> named(NewStrT id) {
        return {};
    }
};


template<const WitLexy::TypeKind kind>
static constexpr auto typetree() {
    if constexpr (kind == WitLexy::TypeKind::U8) {
        return TypeTree<uint8_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U16) {
        return TypeTree<uint16_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U32) {
        return TypeTree<uint32_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::U64) {
        return TypeTree<uint64_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S8) {
        return TypeTree<int8_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S16) {
        return TypeTree<int16_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S32) {
        return TypeTree<int32_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::S64) {
        return TypeTree<int64_t>{};
    } else if constexpr (kind == WitLexy::TypeKind::F32) {
        return TypeTree<float>{};
    } else if constexpr (kind == WitLexy::TypeKind::F64) {
        return TypeTree<double>{};
    } else if constexpr (kind == WitLexy::TypeKind::Bool) {
        return TypeTree<bool>{};
    } else if constexpr (kind == WitLexy::TypeKind::CharType) {
        return TypeTree<char>{};
    } else if constexpr (kind == WitLexy::TypeKind::StringType) {
        return TypeTree<std::string>{};
    } else {
        static_assert(false, "Unhandled typetree kind in typetree");
    }
}

template<const WitLexy::TypeKind head, const WitLexy::TypeKind... kinds> requires(sizeof...(kinds) > 0)
static constexpr auto typetree() {
    constexpr WitLexy::TypeKind kinds_array[] = {kinds...};
    constexpr auto size = sizeof...(kinds);

    if constexpr (head == WitLexy::TypeKind::ListStart) {
        auto element_type = [&]<std::size_t... Is>(std::index_sequence<Is...> is) consteval {
            return typetree<kinds_array[Is]...>();
        }(std::make_index_sequence<size-1>{});

        return TypeTree<std::vector<typename decltype(element_type)::Type>>();
    } else {
        static_assert(false, "Unhandled typetree kind in typetree");
    }
}

struct MethodBinding {
    std::string_view name;
    std::source_location binding_location;
    TypeInfo return_type;
    std::vector<TypeInfo> arg_types;
};



struct BindingCounts {
    size_t methods = 0;
};

static constexpr BindingCounts s_counts = []() constexpr {
    BindingCounts counts;
    dolphin_bindings([&] (auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_method) {
            counts.methods++;
        } else {
            static_assert(false, "Unhandled type in binder");
        }
    });
    return counts;
}();



constexpr std::array<WasmtimeFn, s_counts.methods> s_methods = []() consteval {
    std::array<WasmtimeFn, s_counts.methods> methods{};
    size_t index = 0;
    dolphin_bindings([&](auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_method) {
            using Traits = decltype(item)::Traits;
            methods[index++] = Traits::wrapped;
        } else {
            static_assert(false, "Unhandled type in binder");
        }
    });
    return methods;
}();

template<typename Reader, typename Tag>
void error_reporter(const auto& context, const lexy::error<Reader, Tag>& error) {
    auto context_location = lexy::get_input_location(context.input(), context.position());
    auto location = lexy::get_input_location(context.input(), error.position(), context_location.anchor());

    lexy::stderr_output_iterator out;

    fmt::print(stderr, "{}:{}:{}: ", DOLPHIN_WIT_PATH, location.line_nr(), location.column_nr());

    lexy_ext::diagnostic_writer writer(context.input(), {lexy::visualization_flags::visualize_fancy});


    out = writer.write_message(out, lexy_ext::diagnostic_kind::error,
                            [&](auto out, lexy::visualization_options) {
                                out = lexy::_detail::write_str(out, "while parsing ");
                                out = lexy::_detail::write_str(out, context.production());
                                return out;
                            });

    // Write an annotation for the context.
    if (location.line_nr() != context_location.line_nr())
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::secondary, context_location,
                                      lexy::_detail::next(context.position()),
                                      [&](auto out, lexy::visualization_options) {
                                          return lexy::_detail::write_str(out, "beginning here");
                                      });
        out = writer.write_empty_annotation(out);
    }

    // Write the main annotation.
    if constexpr (std::is_same_v<Tag, lexy::expected_literal>)
    {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(),
                                                                                    error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.index() + 1,
                                      [&](auto out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    }
    else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>)
    {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(),
                                                                                    error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](auto out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected keyword '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    }
    else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>)
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, 1u,
                                      [&](auto out, lexy::visualization_options) {
                                          out = lexy::_detail::write_str(out, "expected ");
                                          out = lexy::_detail::write_str(out, error.name());
                                          return out;
                                      });
    }
    else
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](auto out, lexy::visualization_options) {
                                          return lexy::_detail::write_str(out, error.message());
                                      });
    }

}

template <>
struct fmt::formatter<TypeInfo>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const TypeInfo& type_info, FormatContext& ctx) const {
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

bool check_bindings() {
    auto literal = lexy::string_input(get_embedded_wit());
    auto result = lexy::parse<WitLexy::grammar::witfile>(literal, lexy::callback<void>([](auto context, auto error) {
        error_reporter(context, error);
    }));

    if (!result.has_value() || !result.is_success()) {
        fmt::print(stderr, "Failed to parse dolphin.wit\n");
        return false;
    }

    auto witfile = result.value();

    auto cpu_resource = witfile.interfaces[0].resources[0];

    std::array<MethodBinding, s_counts.methods> methods{};
    size_t index = 0;
    dolphin_bindings([&](auto item, std::source_location loc = std::source_location::current()) constexpr {
        if constexpr (item.is_method) {
            using Traits = decltype(item)::Traits;
            methods[index++] = {item.binding_name(), loc, {ToTypeTree<typename Traits::ReturnType>()}, Traits::arg_types()};
        } else {
            static_assert(false, "Unhandled type in binder");
        }
    });


    for (const auto& method : methods) {
        auto it = std::find_if(cpu_resource.methods.begin(), cpu_resource.methods.end(),
                               [&](const auto& m) { return m.id == method.name; });
        if (it == cpu_resource.methods.end()) {
            fmt::print(stderr, "{}: error: Binding '{}' is not defined in dolphin.wit\n", method.binding_location, method.name);
            continue;
        }

        fmt::print(stderr, "{} : {}(", method.name, method.return_type);
        for (size_t i = 0; i < method.arg_types.size(); ++i) {
            if (i > 0) {
                fmt::print(stderr, ", ");
            }
            fmt::print(stderr, "{}", method.arg_types[i]);
        }
        fmt::print(stderr, ")\n");
    }

    return false;
}