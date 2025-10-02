#include <sstream>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

static int cmd_select_port(Console& c, Tcl_Interp* ip,
                           const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: select-port <name|index> [specKey]", -1));
        return TCL_ERROR;
    }
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString(a[1]) : c.selection().mPrimaryKey;
    if (key == hdl::IdString()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no module context", -1));
        return TCL_ERROR;
    }
    auto* s = c.getSpecByKey(key);
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    hdl::IdString pname;
    if (!c.resolvePortName(*s, a[0], pname)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no such port", -1));
        return TCL_ERROR;
    }
    hdl::tcl::SelRef r{key, pname};
    if (!c.selection().hasPort(r)) c.selection().addPort(r);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> compl_select_port(Console& c,
                                                  const Console::Args& toks) {
    if (toks.size() <= 2) {
        std::string key = c.selection().mPrimaryKey.str();
        if (key.empty())
            return c.completeSpecKeys(toks.size() == 2 ? toks[1] : "");
        return c.completePortsForKey(key, toks.size() == 2 ? toks[1] : "");
    }
    if (toks.size() == 3) { return c.completeSpecKeys(toks[2]); }
    return {};
}
static std::vector<std::string> rev_select_port(Console& c, const std::string&,
                                                const Console::Args& a,
                                                const Selection& pre) {
    std::vector<std::string> inv;
    if (a.empty()) return inv;
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString(a[1]) : pre.mPrimaryKey;
    if (key == hdl::IdString()) return inv;
    auto* s = c.getSpecByKey(key);
    if (!s) return inv;
    hdl::IdString pname;
    if (!c.resolvePortName(*s, a[0], pname)) return inv;
    bool was = false;
    for (auto& r : pre.mPorts)
        if (r.mSpecKey == key && r.mName == pname) {
            was = true;
            break;
        }
    if (!was)
        inv.push_back("unselect-port " + pname.str() + " " + key.str());
    return inv;
}

static int cmd_unselect_port(Console& c, Tcl_Interp* ip,
                             const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: unselect-port <name|index> [specKey]",
                           -1));
        return TCL_ERROR;
    }
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString(a[1]) : c.selection().mPrimaryKey;
    auto* s = c.getSpecByKey(key);
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    hdl::IdString pname;
    if (!c.resolvePortName(*s, a[0], pname)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no such port", -1));
        return TCL_ERROR;
    }
    hdl::tcl::SelRef r{key, pname};
    if (c.selection().hasPort(r)) c.selection().removePort(r);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> rev_unselect_port(Console& c,
                                                  const std::string&,
                                                  const Console::Args& a,
                                                  const Selection& pre) {
    std::vector<std::string> inv;
    if (a.empty()) return inv;
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString(a[1]) : pre.mPrimaryKey;
    if (key == hdl::IdString()) return inv;
    auto* s = c.getSpecByKey(key);
    if (!s) return inv;
    hdl::IdString pname;
    if (!c.resolvePortName(*s, a[0], pname)) return inv;
    for (auto& r : pre.mPorts)
        if (r.mSpecKey == key && r.mName == pname) {
            inv.push_back("select-port " + pname.str() + " " + key.str());
            break;
        }
    return inv;
}

static int cmd_list_ports(Console& c, Tcl_Interp* ip, const Console::Args&) {
    if (c.selection().mModuleKeys.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no modules selected", -1));
        return TCL_ERROR;
    }
    std::ostringstream oss;
    for (auto& key : c.selection().mModuleKeys) {
        auto* s = c.getSpecByKey(key);
        if (!s) continue;
        oss << "Module " << key.str() << ":\n";
        for (size_t i = 0; i < s->mPorts.size(); ++i) {
            auto& p = s->mPorts[i];
            oss << "  [" << i << "] " << p.mName.str()
                << " dir=" << to_string(p.mDir) << " [" << p.mNet.mMsb << ":"
                << p.mNet.mLsb << "]\n";
        }
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_ports(Console& c) {
    c.registerCommand("select-port",
                      "Select a port: select-port <name|index> [specKey]",
                      &cmd_select_port,
                      &compl_select_port,
                      &rev_select_port);
    c.registerCommand("unselect-port",
                      "Unselect a port: unselect-port <name|index> [specKey]",
                      &cmd_unselect_port,
                      nullptr,
                      &rev_unselect_port);
    c.registerCommand(
      "list-ports", "List ports of selected modules: list-ports", &cmd_list_ports);
}
} // namespace hdl::tcl