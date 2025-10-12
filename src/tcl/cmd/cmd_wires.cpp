#include <sstream>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

static int cmd_select_wire(Console& c, Tcl_Interp* ip,
                           const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: select-wire <name|index> [specKey]", -1));
        return TCL_ERROR;
    }
    hdl::IdString key = (a.size() >= 2) ? hdl::IdString::tryLookup(a[1])
                                        : c.selection().mPrimaryKey;
    if (!key.valid()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no module context", -1));
        return TCL_ERROR;
    }
    auto* s = c.getSpecByKey(key.str());
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    hdl::IdString wname;
    if (!c.resolveWireName(*s, a[0], wname)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no such wire", -1));
        return TCL_ERROR;
    }
    hdl::tcl::SelRef r{key, wname};
    if (!c.selection().hasWire(r)) c.selection().addWire(r);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> compl_select_wire(Console& c,
                                                  const Console::Args& toks) {
    // tokens: ["select-wire", "<namePartial>", "[specKey]"]
    if (toks.size() <= 2) {
        std::string key = c.selection().mPrimaryKey.str();
        if (key.empty())
            return c.completeSpecKeys(toks.size() == 2 ? toks[1] : "");
        return c.completeWiresForKey(key, toks.size() == 2 ? toks[1] : "");
    }
    if (toks.size() == 3) { return c.completeSpecKeys(toks[2]); }
    return {};
}
static std::vector<std::string> rev_select_wire(Console& c, const std::string&,
                                                const Console::Args& a,
                                                const Selection& pre) {
    std::vector<std::string> inv;
    if (a.empty()) return inv;
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString::tryLookup(a[1]) : pre.mPrimaryKey;
    if (!key.valid()) return inv;
    auto* s = c.getSpecByKey(key.str());
    if (!s) return inv;
    hdl::IdString wname;
    if (!c.resolveWireName(*s, a[0], wname)) return inv;
    bool was = false;
    for (auto& r : pre.mWires)
        if (r.mSpecKey == key && r.mName == wname) {
            was = true;
            break;
        }
    if (!was) inv.push_back("unselect-wire " + wname.str() + " " + key.str());
    return inv;
}

static int cmd_unselect_wire(Console& c, Tcl_Interp* ip,
                             const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: unselect-wire <name|index> [specKey]", -1));
        return TCL_ERROR;
    }
    hdl::IdString key = (a.size() >= 2) ? hdl::IdString::tryLookup(a[1])
                                        : c.selection().mPrimaryKey;
    auto* s = c.getSpecByKey(key.str());
    if (!s) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    hdl::IdString wname;
    if (!c.resolveWireName(*s, a[0], wname)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no such wire", -1));
        return TCL_ERROR;
    }
    hdl::tcl::SelRef r{key, wname};
    if (c.selection().hasWire(r)) c.selection().removeWire(r);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> rev_unselect_wire(Console& c,
                                                  const std::string&,
                                                  const Console::Args& a,
                                                  const Selection& pre) {
    std::vector<std::string> inv;
    if (a.empty()) return inv;
    hdl::IdString key =
      (a.size() >= 2) ? hdl::IdString::tryLookup(a[1]) : pre.mPrimaryKey;
    if (!key.valid()) return inv;
    auto* s = c.getSpecByKey(key.str());
    if (!s) return inv;
    hdl::IdString wname;
    if (!c.resolveWireName(*s, a[0], wname)) return inv;
    for (auto& r : pre.mWires)
        if (r.mSpecKey == key && r.mName == wname) {
            inv.push_back("select-wire " + wname.str() + " " + key.str());
            break;
        }
    return inv;
}

static int cmd_list_wires(Console& c, Tcl_Interp* ip, const Console::Args&) {
    if (c.selection().mModuleKeys.empty()) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("no modules selected", -1));
        return TCL_ERROR;
    }
    std::ostringstream oss;
    for (auto& key : c.selection().mModuleKeys) {
        auto* s = c.getSpecByKey(key.str());
        if (!s) continue;
        oss << "Module " << key.str() << ":\n";
        for (size_t i = 0; i < s->mWires.size(); ++i) {
            auto& w = s->mWires[i];
            oss << "  [" << i << "] " << w.mName.str() << " [" << w.mNet.mMsb
                << ":" << w.mNet.mLsb << "]\n";
        }
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_wires(Console& c) {
    c.registerCommand("select-wire",
                      "Select a wire: select-wire <name|index> [specKey]",
                      &cmd_select_wire,
                      &compl_select_wire,
                      &rev_select_wire);
    c.registerCommand("unselect-wire",
                      "Unselect a wire: unselect-wire <name|index> [specKey]",
                      &cmd_unselect_wire,
                      nullptr,
                      &rev_unselect_wire);
    c.registerCommand("list-wires",
                      "List wires of selected modules: list-wires",
                      &cmd_list_wires);
}
} // namespace hdl::tcl