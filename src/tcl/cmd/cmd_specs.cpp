#include <sstream>

#include "hdl/ast/decl.hpp"
#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_modules(Console& c, Tcl_Interp* ip, const Console::Args&) {
    std::ostringstream oss;
    for (auto& kv : c.astIndex())
        oss << kv.first.str() << "\n";
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

static int cmd_specs(Console& c, Tcl_Interp* ip, const Console::Args&) {
    std::ostringstream oss;
    for (auto& kv : c.lib())
        oss << kv.first << "\n";
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_specs(Console& c) {
    c.registerCommand("modules", "List AST modules", &cmd_modules);
    c.registerCommand(
      "specs", "List elaborated ModuleSpecs in library", &cmd_specs);
}
} // namespace hdl::tcl