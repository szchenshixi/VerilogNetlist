#include "hdl/common.hpp"
#include "hdl/ast/expr.hpp"

namespace hdl {
const char* to_string(PortDirection d) {
    switch (d) {
    case PortDirection::In: return "In";
    case PortDirection::Out: return "Out";
    case PortDirection::InOut: return "InOut";
    }
    return "?";
}

int64_t width_from_range(const ast::Expr& msbE, const ast::Expr& lsbE) {
    int64_t msb = ast::exprToInt64(msbE);
    int64_t lsb = ast::exprToInt64(lsbE);
    return width_from_range(msb, lsb);
}
int64_t width_from_range(int64_t msb, int64_t lsb) {
    return static_cast<int64_t>((msb >= lsb) ? (msb - lsb + 1)
                                              : (lsb - msb + 1));
}

std::ostream& operator<<(std::ostream& os, const Indent& i) {
    for (int k = 0; k < i.mN; ++k)
        os << ' ';
    return os;
}
} // namespace hdl