

#include "Plugins/WitFile.h"
#include "Plugins/WitLexy.h"

#include "BindingCheck.h"
#include "BindingGen.h"

#include <lexy/input/string_input.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/action/validate.hpp>
#include <lexy_ext/report_error.hpp>

#include <string_view>
#include <type_traits>

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

    std::vector<const WitLexy::Resource*> all_resources;
    all_resources.resize(get_resource_count(), nullptr);


    for (const auto& interface : witfile.interfaces) {
        for (const auto& resource : interface.resources) {
            if (auto index = get_resource_idx(resource.id)) {
                all_resources[*index] = &resource;
            }
        }
    }

    Bindings bindings = get_bindings();

    bool bindings_valid = true;

    // Check resource bindings
    for (size_t i = 0; i < bindings.resources.size(); ++i) {
        auto& resource = bindings.resources[i];
        auto loc = resource.binding_location;
        auto name = resource.name;

        unsigned index = get_resource_idx(name).value();
        if (index != i) {
            fmt::print(stderr, "{}: error: resource {} bound multiple times\n", loc, name);
            fmt::print(stderr, "{}: info: first binding of {} was here\n", bindings.resources[index].binding_location, name);
            bindings_valid = false;
            continue;
        }

        unsigned type_index = get_resource_idx(resource.resource_type).value();
        if (type_index != i) {
            fmt::print(stderr, "{}: error: struct/class for resource {} bound to multiple resources\n", loc, name);
            auto first_resource = bindings.resources[type_index];
            fmt::print(stderr, "{}: info: struct/class first bound to {}\n", first_resource.binding_location, first_resource.name);
            bindings_valid = false;
        }

        if (all_resources[i] == nullptr) {
            fmt::print(stderr, "{}: error: resource {} is not defined in dolphin.wit\n", loc, name);
            bindings_valid = false;
        }
    }

    for (const auto& method : bindings.methods) {
        const std::source_location& loc = method.binding_location;
        const std::string_view name = method.name;

        auto resource_idx = get_resource_idx(method.resource_type);

        if (resource_idx.has_value() == false) {
            fmt::print(stderr, "{}: error: resource for method '{}' is not bound\n", loc, name);
            bindings_valid = false;
            continue;
        }

        const auto* resource = all_resources[resource_idx.value()];

        if (resource == nullptr) {
            std::string_view resource_name = get_resource_name(method.resource_type).value();
            fmt::print(stderr, "{}: error: resource '{}' for method '{}' is not defined in dolphin.wit\n", loc, resource_name, name);
            bindings_valid = false;
            continue;
        }

        auto it = std::find_if(resource->methods.begin(), resource->methods.end(),
                               [&](const auto& m) { return m.id == method.name; });
        if (it == resource->methods.end()) {
            fmt::print(stderr, "{}: error: Binding '{}' is not defined in dolphin.wit\n", loc, name);
            continue;
        }

        size_t num_params = std::max(method.arg_types.size(), it->type.num_params);
        bool method_valid = false;

        for (unsigned i = 0; i < num_params; i++) {
            if (i >= it->type.params.size()) {
                fmt::print(stderr, "{}: error: method '{}' has extra argument of type {} at position {}\n", loc, name, method.arg_types[i], i);
                continue;
            }
            auto arg_name = it->type.params[i].id;
            auto arg_type = it->type.params[i].type;

            if (i >= method.arg_types.size()) {
                fmt::print(stderr, "{}: error: method '{}' is missing argument {} '{}: {}'\n", loc, name, i, arg_name, arg_type);
                continue;
            }
            if (method.arg_types[i].type_tree != it->type.params[i].type.type_tree) {
                fmt::print(stderr, "{}: error: method '{}' argument {} type mismatch: expected '{}: {}', found ': {}'\n", loc, name, i, arg_name, arg_type, method.arg_types[i]);
            } else if (i == num_params - 1) {
                method_valid = true;
            }
        }

        if (it->type.result.type_tree != method.return_type.type_tree) {
            fmt::print(stderr, "{}: error: method '{}' result type mismatch: expected {}, found {}\n", loc, name, it->type.result, method.return_type);
            method_valid = false;
        }

        // If there were any errors, print the expected signature for the method.
        if (!method_valid) {
            bindings_valid = false;
            fmt::print(stderr, "{}: info: expected method {}(", loc, name);
            for (size_t i = 0; i < it->type.num_params; ++i) {
                if (i > 0) {
                    fmt::print(stderr, ", ");
                }
                fmt::print(stderr, "{}: {}", it->type.params[i].id, it->type.params[i].type);
            }
            fmt::print(stderr, ") -> {}\n", it->type.result);
        }
    }

    return bindings_valid;
}