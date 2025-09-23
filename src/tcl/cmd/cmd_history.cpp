#include <cstdio>

#include <readline/history.h>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_history(Console& c, Tcl_Interp* ip, const Console::Args&) {
    HIST_ENTRY **list = history_list();
    if (list) {
        for (int i = 0; list[i]; i++) {
            printf("%d: %s\n", i + history_base, list[i]->line);
        }
    }
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_history(Console& c) {
    c.registerCommand("history", "List the command histories", cmd_history);
}
} // namespace hdl::tcl