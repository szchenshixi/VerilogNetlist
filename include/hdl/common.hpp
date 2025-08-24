#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace hdl {

struct SourceLoc {
    std::string mFile = "<mem>";
    uint32_t mLine = 0;
    uint32_t mCol = 0;
};

enum class PortDirection { In, Out, InOut };

inline const char* to_string(PortDirection d) {
    switch (d) {
    case PortDirection::In: return "In";
    case PortDirection::Out: return "Out";
    case PortDirection::InOut: return "InOut";
    }
    return "?";
}

inline uint32_t width_from_range(int msb, int lsb) {
    return static_cast<uint32_t>((msb >= lsb) ? (msb - lsb + 1)
                                              : (lsb - msb + 1));
}

struct Indent {
    int mN = 0;
    explicit Indent(int n)
        : mN(n) {}
};
inline std::ostream& operator<<(std::ostream& os, const Indent& i) {
    for (int k = 0; k < i.mN; ++k)
        os << ' ';
    return os;
}

} // namespace hdl