#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_undo(Console& c, Tcl_Interp* ip, const Console::Args&) {
    return c.doUndo(ip);
}
static int cmd_redo(Console& c, Tcl_Interp* ip, const Console::Args&) {
    return c.doRedo(ip);
}

namespace hdl::tcl {
void register_cmd_undo(Console& c) {
    c.registerCommand("undo", "Undo last command (if undoable): undo", &cmd_undo);
    c.registerCommand("redo", "Redo last undone command: redo", &cmd_redo);
}
} // namespace hdl::tcl