#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

static int cmd_elab(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    if (a.empty()) {
        Tcl_SetObjResult(
          ip,
          Tcl_NewStringObj("usage: hdl elab <name> [PARAM=VALUE ...]", -1));
        return TCL_ERROR;
    }
    hdl::IdString name(a[0]);
    auto env = Console::parseParamTokens(a, 1, std::cerr);
    hdl::IdString key;
    if (!c.getOrElabByName(name, env, &key)) {
        Tcl_SetObjResult(ip, Tcl_NewStringObj("unknown module name", -1));
        return TCL_ERROR;
    }
    if (!c.selection().hasModuleKey(key)) c.selection().addModuleKey(key);
    c.selection().mPrimaryKey = key;
    std::string msg = "selected " + key.str();
    Tcl_SetObjResult(ip, Tcl_NewStringObj(msg.c_str(), -1));
    return TCL_OK;
}

static std::vector<std::string> rev_elab(Console& c, const std::string&,
                                         const Console::Args& args,
                                         const Selection& pre) {
    std::vector<std::string> inv;
    if (args.empty()) return inv;
    hdl::IdString name(args[0]);
    auto env = Console::parseParamTokens(args, 1, std::cerr);
    hdl::IdString key(hdl::elab::makeModuleKey(name.str(), env));
    if (!pre.hasModuleKey(key))
        inv.push_back("hdl unselect-module " + key.str());
    if (!(pre.mPrimaryKey == hdl::IdString()))
        inv.push_back("hdl set-primary " + pre.mPrimaryKey.str());
    return inv;
}

static std::vector<std::string> compl_elab(Console& c,
                                           const Console::Args& toks) {
    if (toks.size() <= 3) {
        std::string pref = toks.size() >= 3 ? toks[2] : "";
        return c.completeModules(pref);
    }
    hdl::IdString modName(toks[2]);
    std::string last = toks.back();
    return c.completeParams(modName, last);
}

namespace hdl::tcl {
void register_cmd_elab(Console& c) {
    c.registerCommand(
      "elab",
      "Elaborate module specialization and select it as primary",
      &cmd_elab,
      &compl_elab,
      &rev_elab);
}
} // namespace hdl::tcl