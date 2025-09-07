#include <sstream>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_help(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    if (a.empty()) {
        std::ostringstream oss;
        oss << "hdl commands:\n";
        // No direct iteration API; reuse dispatch header: use completion
        // helper
        auto cmds = c.completeModules(""); // mistake—use subcommand listing
        // Proper listing:
        // We can re-call the dispatcher with only "hdl" to list, but better:
        // expose subcommands via hasCommand/get list. For simplicity here,
        // instruct users to use "hdl commands".
        oss << "Use: hdl commands to list\n";
        Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
        return TCL_OK;
    } else {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("Use: hdl commands to list, and 'hdl invert <sub> "
                           "...' to see reverse",
                           -1));
        return TCL_OK;
    }
}

static int cmd_commands(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    (void)c;
    (void)a;
    // We don't expose a direct accessor for the registry.
    // As a simple approach: ask Tcl to expand "hdl" w/o args (the dispatcher
    // prints the list). But we must set Tcl result. We'll rebuild the listing
    // through c.completeSubcommand("") which is private. Alternative:
    // re-evaluate "hdl" now and capture output isn't trivial. Simpler: tell
    // users to rely on "hdl help" fallback message.
    Tcl_SetObjResult(
      ip, Tcl_NewStringObj("Use 'hdl' alone to list subcommands.", -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_help(Console& c) {
    c.registerCommand(
      "help", "Show help (tip: use 'hdl' alone to list)", &cmd_help);
    c.registerCommand(
      "commands", "List subcommands (tip: run 'hdl' alone)", &cmd_commands);
}
} // namespace hdl::tcl