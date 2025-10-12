#include <sstream>

#include "hdl/hier/instance.hpp"
#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_dump_layout(Console& c, Tcl_Interp* ip, const Console::Args&) {
    if (c.selection().mModuleKeys.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no modules selected", -1));
        return TCL_ERROR;
    }
    std::ostringstream oss;
    for (auto& key : c.selection().mModuleKeys) {
        if (auto* s = c.getSpecByKey(key.str())) {
            oss << "=== " << key.str() << " ===\n";
            s->dumpLayout(oss);
        }
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}
static int cmd_dump_connectivity(Console& c, Tcl_Interp* ip,
                                 const Console::Args&) {
    if (c.selection().mModuleKeys.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no modules selected", -1));
        return TCL_ERROR;
    }
    std::ostringstream oss;
    for (auto& key : c.selection().mModuleKeys) {
        if (auto* s = c.getSpecByKey(key.str())) {
            oss << "=== " << key.str() << " ===\n";
            s->dumpConnectivity(oss);
        }
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}
static int cmd_dump_hierarchy(Console& c, Tcl_Interp* ip,
                              const Console::Args&) {
    if (c.selection().mModuleKeys.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no modules selected", -1));
        return TCL_ERROR;
    }
    std::ostringstream oss;
    for (auto& key : c.selection().mModuleKeys) {
        auto* s = c.getSpecByKey(key.str());
        if (!s) continue;
        oss << "=== " << key.str() << " ===\n";
        hdl::elab::hier::dumpInstanceTree(*s, oss);
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_dump(Console& c) {
    c.registerCommand("dump-layout",
                      "Print port/wire layout for selected modules: dump-layout",
                      &cmd_dump_layout);
    c.registerCommand("dump-connectivity",
                      "Print connectivity groups for selected modules: dump-connectivity",
                      &cmd_dump_connectivity);
    c.registerCommand("dump-hierarchy",
                      "Print instance hierarchy for selected modules: dump-hierarchy",
                      &cmd_dump_hierarchy);
}
} // namespace hdl::tcl