#include "hdl/tcl/console.hpp"

#include <algorithm>
#include <sstream>

using hdl::tcl::Console;

// Simple Levenshtein distance for "did you mean" suggestions
static size_t lev(const std::string& a, const std::string& b) {
    const size_t n = a.size(), m = b.size();
    std::vector<size_t> prev(m + 1), cur(m + 1);
    for (size_t j = 0; j <= m; ++j)
        prev[j] = j;
    for (size_t i = 1; i <= n; ++i) {
        cur[0] = i;
        for (size_t j = 1; j <= m; ++j) {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] =
              std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, cur);
    }
    return prev[m];
}

static void render_list(Console& c, Tcl_Interp* ip) {
    auto list = c.listCommands();
    std::ostringstream oss;
    oss << "commands:\n";
    size_t w = 0;
    for (auto& p : list) {
        w = std::max(w, p.first.size());
    }
    for (auto& p : list) {
        oss << "  " << p.first;
        if (p.first.size() < w) oss << std::string(w - p.first.size(), ' ');
        oss << " - " << p.second << "\n";
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
}

static int cmd_help(Console& c, Tcl_Interp* ip, const Console::Args& a) {
    if (a.empty()) {
        render_list(c, ip);
        return TCL_OK;
    }
    const std::string sub = a[0];
    std::string help;
    if (c.getCommandHelp(sub, help)) {
        std::ostringstream oss;
        oss << sub << " - " << help;
        Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
        return TCL_OK;
    }
    // Unknown: suggest nearest commands
    auto list = c.listCommands();
    std::vector<std::pair<size_t, std::string>> cand;
    cand.reserve(list.size());
    for (auto& p : list) {
        cand.emplace_back(lev(sub, p.first), p.first);
    }
    std::sort(cand.begin(), cand.end(), [](auto& x, auto& y) {
        return x.first < y.first;
    });
    std::ostringstream oss;
    oss << "unknown subcommand: " << sub << "\n";
    oss << "did you mean:";
    int shown = 0;
    for (auto& kv : cand) {
        if (kv.first <= 3) {
            oss << "\n  " << kv.second;
            if (++shown >= 5) break;
        }
    }
    if (shown == 0) {
        // fallback to prefix suggestions
        for (auto& p : list) {
            if (p.first.rfind(sub, 0) == 0) {
                oss << "\n  " << p.first;
                ++shown;
                if (shown >= 5) break;
            }
        }
        if (shown == 0) oss << "  (no close matches)";
    }
    Tcl_SetObjResult(ip, Tcl_NewStringObj(oss.str().c_str(), -1));
    return TCL_ERROR;
}

static std::vector<std::string> compl_help(Console& c,
                                           const Console::Args& toks) {
    // tokens: ["hdl","help","<partial>"]
    if (toks.size() != 3) return {};
    std::vector<std::string> names;
    for (auto& p : c.listCommands())
        names.push_back(p.first);
    std::vector<std::string> out;
    const std::string pref = toks[2];
    for (auto& n : names)
        if (pref.empty() || n.rfind(pref, 0) == 0) out.push_back(n);
    std::sort(out.begin(), out.end());
    return out;
}

static int cmd_commands(Console& c, Tcl_Interp* ip, const Console::Args&) {
    render_list(c, ip);
    return TCL_OK;
}

namespace hdl::tcl {
void register_cmd_help(Console& c) {
    c.registerCommand("help",
                      "Show help or help for a subcommand: hdl help [name]",
                      &cmd_help,
                      &compl_help);
    c.registerCommand(
      "commands", "List subcommands with one-line help", &cmd_commands);
}
} // namespace hdl::tcl