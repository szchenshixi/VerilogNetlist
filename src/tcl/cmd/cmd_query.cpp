#include "hdl/tcl/console.hpp"
#include <algorithm>
#include <sstream>

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
            "usage: hdl net-of [specKey] <port|wire> <name> <bitOff>", -1));
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
          ip, Tcl_NewStringObj("usage: hdl render-bit [specKey] <bitId>", -1));
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

// Completion for: net-of [specKey] <port|wire> <name> <bitOff>
static std::vector<std::string> compl_net_of(Console& c,
                                             const Console::Args& toks) {
    auto addIfPrefix = [](std::vector<std::string>& out,
                          const std::string& pref,
                          const std::string& s) {
        if (pref.empty() || s.rfind(pref, 0) == 0) out.push_back(s);
    };
    // tokens: ["hdl","net-of", ...]
    if (toks.size() < 2) return {};

    auto proposeKinds = [&](const std::string& pref) {
        std::vector<std::string> out;
        addIfPrefix(out, pref, "port");
        addIfPrefix(out, pref, "wire");
        return out;
    };
    auto proposeNames =
      [&](hdl::elab::ModuleSpec* s, bool ports, const std::string& pref) {
          std::vector<std::string> out;
          if (!s) return out;
          if (ports) {
              for (auto& p : s->mPorts) {
                  auto n = p.mName.str();
                  if (pref.empty() || n.rfind(pref, 0) == 0) out.push_back(n);
              }
          } else {
              for (auto& w : s->mWires) {
                  auto n = w.mName.str();
                  if (pref.empty() || n.rfind(pref, 0) == 0) out.push_back(n);
              }
          }
          std::sort(out.begin(), out.end());
          return out;
      };

    // Helper: figure out if specKey is present
    auto isKind = [](const std::string& s) {
        return s == "port" || s == "wire";
    };

    // Shapes:
    //  - ["hdl","net-of"]                          -> kinds + specKeys
    //  - ["hdl","net-of","<p>"]                    -> kinds + specKeys
    //  (filter)
    //  - ["hdl","net-of","port","<p>"]             -> names (primary spec)
    //  - ["hdl","net-of","<specKey>","<p>"]        -> kinds
    //  - ["hdl","net-of","<specKey>","port","<p>"] -> names (that spec)
    //  - ["hdl","net-of","...","...","<p>"]        -> bit offsets (if
    //  resolvable)

    // 1) No extra args yet -> kinds + spec keys
    if (toks.size() == 2) {
        auto out = proposeKinds("");
        auto keys = c.completeSpecKeys("");
        out.insert(out.end(), keys.begin(), keys.end());
        return out;
    }

    // 2) One partial arg: could be kind or specKey
    if (toks.size() == 3) {
        std::string pref = toks[2];
        auto out = proposeKinds(pref);
        auto keys = c.completeSpecKeys(pref);
        out.insert(out.end(), keys.begin(), keys.end());
        return out;
    }

    // With two or more args, disambiguate whether arg2 is kind or specKey
    bool arg2IsKind = isKind(toks[2]);
    if (arg2IsKind) {
        // Shape: net-of kind <name> [bitOff]
        bool ports = (toks[2] == "port");
        hdl::elab::ModuleSpec* s = c.currentPrimarySpec();
        if (toks.size() == 4) {
            std::string pref = toks[3];
            return proposeNames(s, ports, pref);
        }
        if (toks.size() == 5) {
            // propose a few example offsets based on a name match (if any)
            std::string name = toks[3];
            if (!s) return {};
            int idx = ports ? s->findPortIndex(hdl::IdString(name))
                            : s->findWireIndex(hdl::IdString(name));
            if (idx < 0) return {};
            uint32_t w = ports ? s->mPorts[(size_t)idx].width()
                               : s->mWires[(size_t)idx].width();
            std::vector<std::string> out;
            std::string pref = toks[4];
            for (uint32_t k = 0; k < std::min<uint32_t>(w, 32); ++k) {
                auto sidx = std::to_string(k);
                if (pref.empty() || sidx.rfind(pref, 0) == 0)
                    out.push_back(sidx);
            }
            return out;
        }
        return {};
    }

    // Arg2 is specKey: net-of <specKey> <kind> <name> [bitOff]
    hdl::IdString key(toks[2]);
    hdl::elab::ModuleSpec* s = c.getSpecByKey(key);
    if (!s) return {};

    if (toks.size() == 4) {
        // propose kinds
        return proposeKinds(toks[3]);
    }
    if (toks.size() == 5) {
        bool ports = (toks[3] == "port");
        return proposeNames(s, ports, toks[4]);
    }
    if (toks.size() == 6) {
        bool ports = (toks[3] == "port");
        std::string name = toks[4];
        int idx = ports ? s->findPortIndex(hdl::IdString(name))
                        : s->findWireIndex(hdl::IdString(name));
        if (idx < 0) return {};
        uint32_t w = ports ? s->mPorts[(size_t)idx].width()
                           : s->mWires[(size_t)idx].width();
        std::vector<std::string> out;
        std::string pref = toks[5];
        for (uint32_t k = 0; k < std::min<uint32_t>(w, 32); ++k) {
            auto sidx = std::to_string(k);
            if (pref.empty() || sidx.rfind(pref, 0) == 0) out.push_back(sidx);
        }
        return out;
    }
    return {};
}

// Completion for: render-bit [specKey] <bitId>
static std::vector<std::string> compl_render_bit(Console& c,
                                                 const Console::Args& toks) {
    if (toks.size() == 2) {
        // Offer spec keys
        return c.completeSpecKeys("");
    }
    if (toks.size() == 3) {
        // Could be specKey or bitId; offer both
        std::string pref = toks[2];
        auto keys = c.completeSpecKeys(pref);
        for (int i = 0; i < 64; ++i) {
            std::string s = std::to_string(i);
            if (pref.empty() || s.rfind(pref, 0) == 0) keys.push_back(s);
        }
        std::sort(keys.begin(), keys.end());
        keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
        return keys;
    }
    // With explicit specKey, suggest a few bitIds based on size
    if (toks.size() == 4) {
        hdl::IdString key(toks[2]);
        hdl::elab::ModuleSpec* s = c.getSpecByKey(key);
        if (!s) return {};
        uint32_t N = s->mBitMap.mConn.size();
        std::vector<std::string> out;
        std::string pref = toks[3];
        for (uint32_t i = 0; i < std::min<uint32_t>(N, 128); ++i) {
            std::string ss = std::to_string(i);
            if (pref.empty() || ss.rfind(pref, 0) == 0) out.push_back(ss);
        }
        return out;
    }
    return {};
}

namespace hdl::tcl {
void register_cmd_query(Console& c) {
    c.registerCommand(
      "net-of",
      "Return NetId: hdl net-of [specKey] <port|wire> <name> <bitOff>",
      &cmd_net_of,
      &compl_net_of);
    c.registerCommand(
      "render-bit",
      "Render a bit owner label: hdl render-bit [specKey] <bitId>",
      &cmd_render_bit,
      &compl_render_bit);
}
} // namespace hdl::tcl