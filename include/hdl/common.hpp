#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace hdl {
namespace ast {
struct Expr;
}
// struct SourceLoc {
//     std::string mFile = "<mem>";
//     uint32_t mLine = 0;
//     uint32_t mCol = 0;
// };

enum class PortDirection { In, Out, InOut };

const char* to_string(PortDirection d);

int64_t width_from_range(const ast::Expr& msbE, const ast::Expr& lsbE);
int64_t width_from_range(int64_t msb, int64_t lsb);
struct Indent {
    int mN = 0;
    explicit Indent(int n)
        : mN(n) {}
};
std::ostream& operator<<(std::ostream& os, const Indent& i);
} // namespace hdl