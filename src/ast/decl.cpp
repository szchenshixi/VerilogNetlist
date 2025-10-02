#include "hdl/ast/decl.hpp"

namespace hdl::ast {
int ModuleDecl::findPortIndex(IdString n) const {
    auto it = std::find_if(
      mPorts.begin(), mPorts.end(), [n](auto& p) { return p.mName == n; });
    return (it != mPorts.end()) ? std::distance(mPorts.begin(), it) : -1;
}
int ModuleDecl::findWireIndex(IdString n) const {
    auto it = std::find_if(
      mWires.begin(), mWires.end(), [n](auto& p) { return p.mName == n; });
    return (it != mWires.end()) ? std::distance(mWires.begin(), it) : -1;
}
} // namespace hdl::ast