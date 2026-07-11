
#include "WitParserImpl.h"


// consteval bool parse_wit_consteval()
// {
//     Input input(get_embedded_wit());
//     auto result = wit_file(input);
//     return result.has_value() && input.empty();
// }

// static_assert(parse_wit_consteval(), "Failed to parse dolphin.wit at compile time");

// std::vector<Wit::PackageItem> parse_wit() {

//     //constexpr ;
//     const std::string_view dolphin_wit = get_embedded_wit();
//     Input input(dolphin_wit);
//     auto result = wit_file(input);

//     if (!result) {
//         print_error(result.error(), dolphin_wit);
//         return {};
//     }

//     auto items = result.value();
//     for (const auto& item : items) {
//         if (std::holds_alternative<Wit::World>(item)) {
//             const auto& world = std::get<Wit::World>(item);
//             fmt::print("Parsed world: {}\n", world.name);
//         } else if (std::holds_alternative<Wit::Interface>(item)) {
//             const auto& interface = std::get<Wit::Interface>(item);
//             fmt::print("Parsed interface: {}\n", interface.name);
//         }
//     }

//     return items;
// }