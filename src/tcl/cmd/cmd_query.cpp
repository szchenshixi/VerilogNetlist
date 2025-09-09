#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;

static int cmd_net_of(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    size_t idx = 0;
    hdl::IdString key;
    if (a.size() == 4) {
        key = hdl::IdString(a[0]);
        idx = 1;
    } else if (a.size() == 3) {
        key = c.selection().mPrimaryKey;
        idx = 0;
    } else {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj(
            "usage: net-of [specKey] <port|wire> <name> <bitOff>", -1));
        return TCL_ERROR;
    }
    if (key == hdl::IdString()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no module context", -1));
        return TCL_ERROR;
    }
    auto* s = c.getSpecByKey(key);
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    const std::string& kind = a[idx + 0];
    hdl::IdString name(a[idx + 1]);
    uint32_t bitOff = 0;
    try {
        bitOff = (uint32_t)std::stoul(a[idx + 2]);
    } catch (...) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("invalid bitOff", -1));
        return TCL_ERROR;
    }
    hdl::net::BitId b = UINT32_MAX;
    if (kind == "port") b = s->portBit(name, bitOff);
    else if (kind == "wire") b = s->wireBit(name, bitOff);
    else {
        Tcl_SetObjResult(
          ip, Tcl_NewStringObj("first arg must be 'port' or 'wire'", -1));
        return TCL_ERROR;
    }
    if (b == UINT32_MAX) {
        Tcl_SetObjResult(
          ip, Tcl_NewStringObj("bit out of range or unknown name", -1));
        return TCL_ERROR;
    }
    auto n = s->mBitMap.netId(b);
    Tcl_SetObjResult(ip, Tcl_NewIntObj((int)n));
    return TCL_OK;
}

static int cmd_render_bit(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    hdl::IdString key;
    size_t idx = 0;
    if (a.size() == 2) {
        key = hdl::IdString(a[0]);
        idx = 1;
    } else if (a.size() == 1) {
        key = c.selection().mPrimaryKey;
        idx = 0;
    } else {
        Tcl_SetObjResult(
          ip, Tcl_NewStringObj("usage: render-bit [specKey] <bitId>", -1));
        return TCL_ERROR;
    }
    auto* s = c.getSpecByKey(key);
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    uint32_t bit = 0;
    try {
        bit = (uint32_t)std::stoul(a[idx]);
    } catch (...) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("invalid bitId", -1));
        return TCL_ERROR;
    }
    std::string out = s->renderBit(bit);
    Tcl_SetObjResult(ip, Tcl_NewStringObj(out.c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_query(Console& c) {
    c.registerCommand(
      "net-of",
      "Return NetId: net-of [specKey] <port|wire> <name> <bitOff>",
      &cmd_net_of);
    c.registerCommand(
      "render-bit",
      "Render a bit owner label: render-bit [specKey] <bitId>",
      &cmd_render_bit);
}
}