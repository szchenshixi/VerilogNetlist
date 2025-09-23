#pragma once
#include "hdl/tcl/console.hpp"
#include "user/register_user.hpp"

// Declarations of per-file registration
namespace hdl::tcl {
void register_cmd_help(Console& c);
void register_cmd_specs(Console& c);
void register_cmd_invert(Console& c);
void register_cmd_elab(Console& c);
// select-module/select-spec/set-primary/unselect-module
void register_cmd_select(Console& c);
void register_cmd_selection(Console& c); // selection show/summary/clear
void register_cmd_ports(Console& c);   // select-port/unselect-port/list-ports
void register_cmd_wires(Console& c);   // select-wire/unselect-wire/list-wires
void register_cmd_dump(Console& c);    // dump-layout/connectivity/hierarchy
void register_cmd_query(Console& c);   // net-of/render-bit
void register_cmd_undo(Console& c);    // undo/redo
void register_cmd_history(Console& c); // history

void register_all_commands(Console& c);
// external user commands (stubbed by default; see src/tcl/cmd/user/)
void register_user_commands(Console& c);
} // namespace hdl::tcl