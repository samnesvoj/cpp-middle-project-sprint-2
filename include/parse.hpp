#pragma once

#include <charconv>
#include <concepts>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

template <typename T>
    requires(std::integral<T> || std::floating_point<T>)
std::expected<T, scan_error> parse_number(std::string_view input) {
    const char *end = input.data() + input.size();
    T value{};
    auto [ptr, ec] = std::from_chars(input.data(), end, value);
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected(scan_error{"Number out of range"});
    }
    if (ec != std::errc{} || ptr != end) {
        return std::unexpected(scan_error{"Invalid number"});
    }
    return value;
}

template <scannable T>
std::expected<T, scan_error> parse_value(std::string_view input) {
    if constexpr (std::same_as<T, std::string_view>) {
        return input;
    } else if constexpr (std::same_as<T, std::string>) {
        return std::string(input);
    } else {
        return parse_number<T>(input);
    }
}

// Функция для парсинга значения с учетом спецификатора формата
template <scannable T>
[[nodiscard]] std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    using U = std::remove_cv_t<T>;

    const bool known = fmt.empty() || fmt == "%d" || fmt == "%u" || fmt == "%f" || fmt == "%s";
    if (!known) {
        return std::unexpected(scan_error{"Unsupported format specifier"});
    }

    const bool matches = fmt.empty() || (fmt == "%d" && supported_signed<U>) ||
                         (fmt == "%u" && supported_unsigned<U>) || (fmt == "%f" && supported_floating<U>) ||
                         (fmt == "%s" && supported_string<U>);
    if (!matches) {
        return std::unexpected(scan_error{"Format specifier does not match target type"});
    }

    return parse_value<U>(input);
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
[[nodiscard]] std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos || (start == 0 && pos != 0)) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        if (!input.ends_with(remaining_format)) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, input.size() - remaining_format.size()));
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{format_parts, input_parts};
}

}  // namespace stdx::details