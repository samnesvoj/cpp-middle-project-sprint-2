#pragma once

#include <optional>
#include <tuple>
#include <utility>

#include "parse.hpp"
#include "types.hpp"

namespace stdx {

template <details::scannable... Ts>
[[nodiscard]] std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input,
                                                                                   std::string_view format) {
    auto parts = details::parse_sources<Ts...>(input, format);
    if (!parts) {
        return std::unexpected(parts.error());
    }

    const auto &[format_parts, input_parts] = *parts;
    if (format_parts.size() != sizeof...(Ts) || input_parts.size() != sizeof...(Ts)) {
        return std::unexpected(details::scan_error{"Number of placeholders does not match number of types"});
    }

    return
        [&]<size_t... I>(std::index_sequence<I...>) -> std::expected<details::scan_result<Ts...>, details::scan_error> {
            std::tuple results{details::parse_value_with_format<Ts>(input_parts[I], format_parts[I])...};

            std::optional<details::scan_error> first_error;
            auto remember_first_error = [&](const auto &result) {
                if (!result && !first_error) {
                    first_error = result.error();
                }
            };
            (remember_first_error(std::get<I>(results)), ...);
            if (first_error) {
                return std::unexpected(*first_error);
            }

            return details::scan_result<Ts...>{std::tuple<Ts...>{*std::move(std::get<I>(results))...}};
        }(std::index_sequence_for<Ts...>{});
}

}  // namespace stdx
