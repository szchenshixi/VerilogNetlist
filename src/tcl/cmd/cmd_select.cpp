#include "hdl/tcl/console.hpp"
#include "hdl/util/id_string.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

// select-module <name> [PARAM=VALUE ...]
static int cmd_select_module(Console& c, Tcl_Interp* ip,
                             const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: select-module <name> [PARAM=VALUE ...]",
                           -1));
        return TCL_ERROR;
    }
    auto name = hdl::IdString::tryLookup(a[0]);
    auto env = Console::parseParamTokens(a, 1, &std::cerr);
    hdl::IdString key;
    if (!c.getOrElabByName(name.str(), env, &key)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown module", -1));
        return TCL_ERROR;
    }
    if (!c.selection().hasModuleKey(key)) c.selection().addModuleKey(key);
    c.selection().mPrimaryKey = key;
    Tcl_SetObjResult(ip, Tcl_NewStringObj(key.str().c_str(), -1));
    return TCL_OK;
}
static std::vector<std::string> rev_select_module(Console& c,
                                                  const std::string&,
                                                  const Console::Args& a,
                                                  const Selection& pre) {
    std::vector<std::string> inv;
    if (a.empty()) return inv;
    auto name = hdl::IdString::tryLookup(a[0]);
    auto env = Console::parseParamTokens(a, 1, &std::cerr);
    auto key =
      hdl::IdString::tryLookup(hdl::elab::makeModuleKey(name.str(), env));
    if (!pre.hasModuleKey(key)) inv.push_back("unselect-module " + key.str());
    if (pre.mPrimaryKey.valid()) {
        inv.push_back("set-primary " + pre.mPrimaryKey.str());
    }
    return inv;
}
static std::vector<std::string>
compl_select_module(Console& c, const Console::Args& toks) {
    // tokens: ["select-module", "<modulePartial>", "PARAM=...", ...]
    if (toks.size() <= 2) {
        std::string pref = toks.size() >= 2 ? toks[1] : "";
        return c.completeModules(pref);
    }
    auto modName = hdl::IdString::tryLookup(toks[1]);
    return c.completeParams(modName.str(), toks.back());
}

// select-spec <specKey>
static int cmd_select_spec(Console& c, Tcl_Interp* ip,
                           const Console::Args& a) {
    if (a.size() != 1) {
        Tcl_SetObjResult(ip,
                         Tcl_NewStringObj("usage: select-spec <specKey>", -1));
        return TCL_ERROR;
    }
    auto key = hdl::IdString::tryLookup(a[0]);
    if (!c.getSpecByKey(key.str())) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown specKey", -1));
        return TCL_ERROR;
    }
    if (!c.selection().hasModuleKey(key)) c.selection().addModuleKey(key);
    c.selection().mPrimaryKey = key;
    Tcl_SetObjResult(ip, Tcl_NewStringObj(key.str().c_str(), -1));
    return TCL_OK;
}
static std::vector<std::string> rev_select_spec(Console&, const std::string&,
                                                const Console::Args& a,
                                                const Selection& pre) {
    std::vector<std::string> inv;
    if (a.size() != 1) return inv;
    auto key = hdl::IdString::tryLookup(a[0]);
    if (!pre.hasModuleKey(key)) inv.push_back("unselect-module " + key.str());
    if (pre.mPrimaryKey.valid())
        inv.push_back("set-primary " + pre.mPrimaryKey.str());
    return inv;
}
static std::vector<std::string> compl_select_spec(Console& c,
                                                  const Console::Args& toks) {
    std::string pref = toks.size() >= 2 ? toks[1] : "";
    return c.completeSpecKeys(pref);
}

// set-primary <specKey>
static int cmd_set_primary(Console& c, Tcl_Interp* ip,
                           const Console::Args& a) {
    if (a.size() != 1) {
        Tcl_SetObjResult(ip,
                         Tcl_NewStringObj("usage: set-primary <specKey>", -1));
        return TCL_ERROR;
    }
    auto key = hdl::IdString::tryLookup(a[0]);
    if (!c.selection().hasModuleKey(key)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("specKey not in selection", -1));
        return TCL_ERROR;
    }
    c.selection().mPrimaryKey = key;
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> rev_set_primary(Console&, const std::string&,
                                                const Console::Args& a,
                                                const Selection& pre) {
    std::vector<std::string> inv;
    if (a.size() != 1) return inv;
    if (pre.mPrimaryKey.valid()) {
        inv.push_back("hdl set-primary " + pre.mPrimaryKey.str());
    }
    return inv;
}
static std::vector<std::string> compl_set_primary(Console& c,
                                                  const Console::Args& toks) {
    std::string pref = toks.size() >= 2 ? toks[1] : "";
    return c.completeSpecKeys(pref);
}

// unselect-module <specKey>
static int cmd_unselect_module(Console& c, Tcl_Interp* ip,
                               const Console::Args& a) {
    if (a.size() != 1) {
        Tcl_SetObjResult(
          ip, Tcl_NewStringObj("usage: unselect-module <specKey>", -1));
        return TCL_ERROR;
    }
    auto key = hdl::IdString::tryLookup(a[0]);
    if (!c.selection().hasModuleKey(key)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("module not in selection", -1));
        return TCL_ERROR;
    }
    c.selection().removeModuleKey(key);
    Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
    return TCL_OK;
}
static std::vector<std::string> rev_unselect_module(Console& c,
                                                    const std::string&,
                                                    const Console::Args& a,
                                                    const Selection& pre) {
    std::vector<std::string> inv;
    if (a.size() != 1) return inv;
    auto key = hdl::IdString::tryLookup(a[0]);
    inv.push_back("select-spec " + key.str());
    for (auto& r : pre.mPorts)
        if (r.mSpecKey == key)
            inv.push_back("select-port " + r.mName.str() + " " + key.str());
    for (auto& r : pre.mWires)
        if (r.mSpecKey == key)
            inv.push_back("select-wire " + r.mName.str() + " " + key.str());
    if (pre.mPrimaryKey.valid())
        inv.push_back("set-primary " + pre.mPrimaryKey.str());
    return inv;
}
static std::vector<std::string>
compl_unselect_module(Console& c, const Console::Args& toks) {
    std::string pref = toks.size() >= 2 ? toks[1] : "";
    return c.completeSpecKeys(pref);
}

namespace hdl::tcl {
void register_cmd_select(Console& c) {
    c.registerCommand("select-module",
                      "Add specialization by module+params and set primary: "
                      "select-module <name> [PARAM=VALUE ...]",
                      &cmd_select_module,
                      &compl_select_module,
                      &rev_select_module);
    c.registerCommand(
      "select-spec",
      "Add specialization by specKey and set primary: select-spec <specKey>",
      &cmd_select_spec,
      &compl_select_spec,
      &rev_select_spec);
    c.registerCommand("set-primary",
                      "Set the primary module context: set-primary <specKey>",
                      &cmd_set_primary,
                      &compl_set_primary,
                      &rev_set_primary);
    c.registerCommand(
      "unselect-module",
      "Remove specialization from selection: unselect-module <specKey>",
      &cmd_unselect_module,
      &compl_unselect_module,
      &rev_unselect_module);
}
} // namespace hdl::tcl
