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

int64_t widthFromRange(const ast::IntExpr& msbE, const ast::IntExpr& lsbE,
                       const elab::ParamSpec& env) {
    int64_t msb = ast::evalIntExpr(msbE, env);
    int64_t lsb = ast::evalIntExpr(lsbE, env);
    return widthFromRange(msb, lsb);
}
int64_t widthFromRange(int64_t msb, int64_t lsb) {
    return static_cast<int64_t>((msb >= lsb) ? (msb - lsb + 1)
                                             : (lsb - msb + 1));
}

std::ostream& operator<<(std::ostream& os, const Indent& i) {
    for (int k = 0; k < i.mN; ++k)
        os << ' ';
    return os;
}

void warn(std::ostream* diag, const std::string& msg, int indent) {
    if (diag) *diag << Indent(indent) << "WARN: " << msg << "\n";
}
void error(std::ostream* diag, const std::string& msg, int indent) {
    if (diag) *diag << Indent(indent) << "ERROR: " << msg << "\n";
}

void update(elab::ParamSpec& out, const elab::ParamSpec& overrides) {
    for (auto& [key, val] : out) {
        if (auto it = out.find(key); it == out.end()) continue;
        out[key] = val;
    }
}
} // namespace hdl