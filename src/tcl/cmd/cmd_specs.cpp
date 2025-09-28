#include "hdl/ast/decl.hpp"
#include "hdl/tcl/console.hpp"

#include <sstream>

using hdl::tcl::Console;

static int cmd_modules(Console& c, Tcl_Interp* ip, const Console::Args&) {
    std::ostringstream oss;
    for (auto& kv : c.astIndex()) {
        oss << kv.first.str() << "\n";
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

// Completion for "modules"/aliases: list AST modules
static std::vector<std::string> compl_modules(Console& c,
                                              const Console::Args& toks) {
    // tokens like: ["modules","<module1>", "<partial>"]
    std::string pref = (toks.size() >= 3) ? toks.back() : "";
    return c.completeModules(pref);
}

static int cmd_specs(Console& c, Tcl_Interp* ip, const Console::Args&) {
    std::ostringstream oss;
    for (auto& kv : c.lib())
        oss << kv.first << "\n";
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

// Completion for "specs": list library specialization keys
static std::vector<std::string> compl_specs(Console& c,
                                            const Console::Args& toks) {
    std::string pref = (toks.size() >= 3) ? toks[2] : "";
    return c.completeSpecKeys(pref);
}

namespace hdl::tcl {
void register_cmd_specs(Console& c) {
    // Base names
    c.registerCommand(
      "modules", "List AST modules", &cmd_modules, &compl_modules);
    c.registerCommand("specs",
                      "List elaborated ModuleSpecs in library",
                      &cmd_specs,
                      &compl_specs);
    // Usability aliases
    c.registerCommand(
      "list-modules", "Alias: modules", &cmd_modules, &compl_modules);
    c.registerCommand(
      "list-module", "Alias: modules", &cmd_modules, &compl_modules);
    c.registerCommand("list-specs", "Alias: specs", &cmd_specs, &compl_specs);
}
} // namespace hdl::tcl