#pragma once
#include <iostream>

namespace log {
    void debug(auto ... ts) {
        #ifdef DEBUG
        std::cout (<< ts)... << '\n';
        #endif
    }

    void info (auto ... ts) {
        std::cout (<< ts)... << '\n';
    }

    void error(auto ... ts) {
        std::cerr (<< ts)... << '\n';
    }
};