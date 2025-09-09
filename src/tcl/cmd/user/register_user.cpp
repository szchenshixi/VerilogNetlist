#include "register_user.hpp"

// This file is the user entry point to register custom commands.
// By default it does nothing. You can add your own command sources under
// src/tcl/cmd/user/ and call their registration functions here.
//
// Example:
//   void register_cmd_myfeature(hdl::tcl::Console& c);
//   namespace hdl::tcl {
//   void register_user_commands(Console& c) {
//       register_cmd_myfeature(c);
//   }
//   }

namespace hdl::tcl {

void register_user_commands(Console& /*c*/) {
    // No-op by default.
    // Add calls to your own register_cmd_* functions here.
}

} // namespace hdl::tcl