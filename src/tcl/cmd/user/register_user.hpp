#pragma once
#include "hdl/tcl/console.hpp"

// User command registration hook.
//
// How to use:
// - Drop your command implementations under src/tcl/cmd/user/, for example:
//     src/tcl/cmd/user/cmd_myfeature.cpp
//   In that file, define a function:
//     void register_cmd_myfeature(hdl::tcl::Console& c) { ...
//     c.registerCommand(...); }
//
// - In register_user_commands, call your registration function(s).
//
// Build system:
// - CMakeLists.txt automatically glob-adds all *.cpp under src/tcl/cmd/user/
// to
//   the hdl_tcl target. You only need to add files; no further changes
//   required.
//
// Example command:
//   static int cmd_echo(Console& c, Tcl_Interp* ip, const Console::Args& a) {
//     std::string out;
//     for (size_t i=0;i<a.size();++i) { if (i) out += " "; out += a[i]; }
//     Tcl_SetObjResult(ip, Tcl_NewStringObj(out.c_str(), -1));
//     return TCL_OK;
//   }
//   void register_cmd_myfeature(Console& c) {
//     c.registerCommand("echo", "Echo arguments: echo <args...>", &cmd_echo);
//   }
//
// Then register it from register_user_commands below.

namespace hdl::tcl {
void register_user_commands(Console& c);
}