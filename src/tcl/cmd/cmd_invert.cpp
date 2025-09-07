#include <sstream>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

static int cmd_invert(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip, Tcl_NewStringObj("usage: hdl invert <sub> [args...]", -1));
        return TCL_ERROR;
    }
    std::string sub = a[0];
    Console::Args args(a.begin() + 1, a.end());
    auto plan = c.computeReversePlan(sub, args, c.selection());
    if (plan.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("<none>", -1));
        return TCL_OK;
    }
    std::ostringstream oss;
    for (auto& l : plan)
        oss << l << "\n";
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_invert(Console& c) {
    c.registerCommand("invert",
                      "Show reverse command(s): hdl invert <sub> [args...]",
                      &cmd_invert);
}
}