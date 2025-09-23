#include "register_all.hpp"

namespace hdl::tcl {
void register_all_commands(Console& c) {
    register_cmd_help(c);
    register_cmd_specs(c);
    register_cmd_invert(c);
    register_cmd_elab(c);
    register_cmd_select(c);
    register_cmd_selection(c);
    register_cmd_ports(c);
    register_cmd_wires(c);
    register_cmd_dump(c);
    register_cmd_query(c);
    register_cmd_undo(c);
    register_cmd_history(c);
    // Hook for user-provided commands (see src/tcl/cmd/user/)
    register_user_commands(c);
}
} // namespace hdl::tcl