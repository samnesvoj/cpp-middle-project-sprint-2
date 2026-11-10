#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

template <typename T, typename... Types>
concept one_of = (std::same_as<std::remove_cv_t<T>, Types> || ...);

template <typename T>
concept supported_signed = one_of<T, int8_t, int16_t, int32_t, int64_t>;

template <typename T>
concept supported_unsigned = one_of<T, uint8_t, uint16_t, uint32_t, uint64_t>;

template <typename T>
concept supported_floating = one_of<T, float, double>;

template <typename T>
concept supported_string = one_of<T, std::string, std::string_view>;

template <typename T>
concept scannable = supported_signed<T> || supported_unsigned<T> || supported_floating<T> || supported_string<T>;

// Шаблонный класс для хранения результатов успешного сканирования

template <typename... Ts>
struct scan_result {
    std::tuple<Ts...> scanned_values;

    const std::tuple<Ts...> &values() const { return scanned_values; }
    std::tuple<Ts...> &values() { return scanned_values; }
};

}  // namespace stdx::details
