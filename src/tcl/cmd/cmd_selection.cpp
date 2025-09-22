#include <sstream>
#include <algorithm>

#include "hdl/tcl/console.hpp"

using hdl::tcl::Console;
using hdl::tcl::Selection;

static int cmd_selection(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    if (a.empty() || a[0] == "show") {
        std::ostringstream oss;
        // Render selection
        oss << "Selection:\n";
        if (c.selection().mModuleKeys.empty()) oss << "  modules: <none>\n";
        else {
            oss << "  modules:\n";
            for (auto& k : c.selection().mModuleKeys) {
                oss << "    " << (k == c.selection().mPrimaryKey ? "* " : "  ")
                    << k.str() << "\n";
            }
        }
        oss << "  ports:\n";
        if (c.selection().mPorts.empty()) oss << "    <none>\n";
        else
            for (auto& r : c.selection().mPorts)
                oss << "    " << r.mSpecKey.str() << "." << r.mName.str()
                    << "\n";
        oss << "  wires:\n";
        if (c.selection().mWires.empty()) oss << "    <none>\n";
        else
            for (auto& r : c.selection().mWires)
                oss << "    " << r.mSpecKey.str() << "." << r.mName.str()
                    << "\n";
        Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
        return TCL_OK;
    } else if (a[0] == "summary") {
        std::ostringstream oss;
        size_t modN = c.selection().mModuleKeys.size();
        size_t portN = c.selection().mPorts.size();
        size_t wireN = c.selection().mWires.size();
        uint64_t portBits = 0, wireBits = 0;
        for (auto& r : c.selection().mPorts) {
            if (auto* s = c.getSpecByKey(r.mSpecKey)) {
                int idx = s->findPortIndex(r.mName);
                if (idx >= 0) portBits += s->mPorts[(size_t)idx].width();
            }
        }
        for (auto& r : c.selection().mWires) {
            if (auto* s = c.getSpecByKey(r.mSpecKey)) {
                int idx = s->findWireIndex(r.mName);
                if (idx >= 0) wireBits += s->mWires[(size_t)idx].width();
            }
        }
        oss << "Summary:\n  modules: " << modN
            << "\n  selected ports: " << portN << " (" << portBits
            << " bits)\n  selected wires: " << wireN << " (" << wireBits
            << " bits)\n";
        Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
        return TCL_OK;
    } else if (a[0] == "clear") {
        c.selection().clearAll();
        Tcl_SetObjResult(ip, Tcl_NewStringObj("OK", -1));
        return TCL_OK;
    }
    Tcl_SetObjResult(
      ip, Tcl_NewStringObj("usage: hdl selection show|summary|clear", -1));
    return TCL_ERROR;
}

static std::vector<std::string> rev_selection(Console&, const std::string&,
                                              const Console::Args&,
                                              const Selection& pre) {
    std::vector<std::string> inv;
    for (auto& k : pre.mModuleKeys)
        inv.push_back("hdl select-spec " + k.str());
    for (auto& r : pre.mPorts)
        inv.push_back("hdl select-port " + r.mName.str() + " " +
                      r.mSpecKey.str());
    for (auto& r : pre.mWires)
        inv.push_back("hdl select-wire " + r.mName.str() + " " +
                      r.mSpecKey.str());
    if (!(pre.mPrimaryKey == hdl::IdString()))
        inv.push_back("hdl set-primary " + pre.mPrimaryKey.str());
    return inv;
}

// Completion: selection sub-options
static std::vector<std::string> compl_selection(Console&, const Console::Args& toks) {
    // tokens like: ["hdl","selection","<partial>"]
    std::vector<std::string> opts = {"show", "summary", "clear"};
    if (toks.size() < 2) return {};
    std::string pref;
    if (toks.size() >= 3) pref = toks[2];
    std::vector<std::string> out;
    for (const auto& s : opts) {
        if (pref.empty() || s.rfind(pref, 0) == 0) out.push_back(s);
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

namespace hdl::tcl {
void register_cmd_selection(Console& c) {
    c.registerCommand("selection",
                      "Selection management: show|summary|clear",
                      &cmd_selection,
                      &compl_selection,
                      &rev_selection);
}
} // namespace hdl::tcl