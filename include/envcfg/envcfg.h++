
#ifndef ENV_CFG_H
#define ENV_CFG_H
#include <concepts>
#include <string_view>
#include <system_error>
#include <charconv>
#include <cstdlib>

namespace envcfg {
    template<typename T>
    struct Parser;

    template<typename T>
        requires ((std::integral<T> && !std::same_as<T, bool>) || std::floating_point<T>)
    struct Parser<T> {
        static T parse(std::string_view str) {
            T value{};

            auto [ptr, ec] = std::from_chars(
                str.data(),
                str.data() + str.size(),
                value
                );

            if (ec != std::errc{} || ptr != str.data() + str.size()) { throw std::invalid_argument("Invalid Number"); }

            return value;
        }
    };

    // boolean
    template<>
    struct Parser<bool> {
        static bool parse(std::string_view str) {
            if (str == "true" || str == "1" || str == "on") return true;
            if (str == "false" || str == "0" || str == "off") return false;

            throw std::invalid_argument("Invalid Boolean");
        }
    };

    template<>
    struct Parser<std::string> {
        static std::string parse(std::string_view str) {
            return std::string{str};
        }
    };


    template<typename T>
    T get(const char* name, T fallback) {
        const char* raw = std::getenv(name);

        if (raw == nullptr) { return fallback; }

        try {
            return Parser<T>::parse(raw);
        } catch (const std::invalid_argument& error) {
            throw std::invalid_argument(std::string{name} + ": " + error.what());
        }
    }
}

#endif //ENV_CFG_H
