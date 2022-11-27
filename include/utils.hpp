#pragma once
#include <iostream>

namespace log {
    void impl_anyprint (auto&) {}
    void impl_anyprint (auto& output, auto& out, auto& ... rest) {
        output << out << ' ';
        impl_anyprint(output, rest...);
    }

    void debug(auto ... ts) {
        #ifndef NDEBUG
        impl_anyprint(std::cout, ts...);
        std::cout << '\n';
        #endif
    }

    void info (auto ... ts) {
        impl_anyprint(std::cout, ts...);
        std::cout << '\n';
    }

    void error(auto ... ts) {
        impl_anyprint(std::cerr, ts...);
        std::cerr << '\n';
    }
};