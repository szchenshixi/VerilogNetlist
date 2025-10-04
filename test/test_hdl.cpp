#include "hdl/ast/decl.hpp"
#include "hdl/ast/expr.hpp"
#include "hdl/elab/elaborate.hpp"
#include "hdl/elab/flatten.hpp"
#include "hdl/elab/spec.hpp"
#include "hdl/util/id_string.hpp"
#include <gtest/gtest.h>

using namespace hdl;
using namespace hdl::ast;
using namespace hdl::elab;

// Helpers
// Nets
static NetDecl n(int msb, int lsb) {
    return {Expr::number(msb), Expr::number(lsb)};
}

TEST(IdString, Basic) {
    IdString a1("foo");
    IdString a2("foo");
    IdString b("bar");
    EXPECT_TRUE(a1.valid());
    EXPECT_EQ(a1, a2);
    EXPECT_EQ(a1.id(), a2.id());
    EXPECT_NE(a1.id(), b.id());
    EXPECT_EQ(a1.str(), "foo");
}

TEST(Expr, WidthAndString) {
    IdString M("M");
    IdString x("x");
    IdString y("y");

    ModuleDecl md;
    md.mName = M;
    md.mPorts.push_back(PortDecl{x, PortDirection::In, n(7, 0)});
    md.mWires.push_back(WireDecl{y, n(3, 0)});

    ModuleSpec ms = elaborateModule(md);
    auto id_x = Expr::id(x);
    auto id_y = Expr::id(y);
    auto s = Expr::slice(x, 5, 2);
    auto c = Expr::concat({s, id_y}); // MSB: x[5:2] (4), LSB: y[3:0] (4) => 8
    EXPECT_EQ(exprBitWidth(id_x, ms), 8u);
    EXPECT_EQ(exprBitWidth(s, ms), 4u);
    EXPECT_EQ(exprBitWidth(c, ms), 8u);

    std::string s_str = exprToString(s);
    EXPECT_EQ(s_str, "x[5:2]");
    std::string c_str = exprToString(c);
    EXPECT_EQ(c_str, "{x[5:2], y}");
}

TEST(BitMap, AllocationAndReverse) {
    IdString M("M");
    IdString p("p");
    IdString q("q");
    IdString w("w");

    ModuleDecl md;
    md.mName = M;
    md.mPorts.push_back(PortDecl{p, PortDirection::In, n(3, 0)});  // 4 bits
    md.mPorts.push_back(PortDecl{q, PortDirection::Out, n(1, 0)}); // 2 bits
    md.mWires.push_back(WireDecl{w, n(7, 0)});                     // 8 bits

    ModuleSpec spec = elaborateModule(md);
    EXPECT_EQ(spec.mPorts.size(), 2u);
    EXPECT_EQ(spec.mWires.size(), 1u);

    // Check BitIds ranges: port[0]=0..3, port[1]=4..5, wire[0]=6..13
    EXPECT_EQ(spec.mBitMap.portBit(0, 0), 0u);
    EXPECT_EQ(spec.mBitMap.portBit(0, 3), 3u);
    EXPECT_EQ(spec.mBitMap.portBit(1, 1), 5u);
    EXPECT_EQ(spec.mBitMap.wireBit(0, 0), 6u);
    EXPECT_EQ(spec.mBitMap.wireBit(0, 7), 13u);
    // Check BitIds ranges with helper functions
    EXPECT_EQ(spec.portBit(p, 0), 0u);
    EXPECT_EQ(spec.portBit(p, 3), 3u);
    EXPECT_EQ(spec.portBit(q, 1), 5u);
    EXPECT_EQ(spec.wireBit(w, 0), 6u);
    EXPECT_EQ(spec.wireBit(w, 7), 13u);

    // Reverse render
    EXPECT_EQ(spec.renderBit(0), "port " + p.str() + "[0]");
    EXPECT_EQ(spec.renderBit(13), "wire " + w.str() + "[7]");
}

TEST(Connectivity, AliasAndNetId) {
    IdString M("M");
    IdString a("a");
    IdString b("b");

    ModuleDecl md;
    md.mName = M;
    md.mWires.push_back(WireDecl{a, n(1, 0)}); // 2b
    md.mWires.push_back(WireDecl{b, n(1, 0)}); // 2b
    ModuleSpec spec = elaborateModule(md);

    auto a0 = spec.wireBit(a, 0);
    auto b1 = spec.wireBit(b, 1);
    spec.mBitMap.alias(a0, b1);

    EXPECT_EQ(spec.mBitMap.netId(a0), spec.mBitMap.netId(b1));
    EXPECT_NE(spec.mBitMap.netId(a0),
              spec.mBitMap.netId(spec.mBitMap.wireBit(0, 1)));
}

TEST(Flatten, IdSliceConcat) {
    IdString M("M");
    IdString x("x");
    IdString y("y");

    ModuleDecl md;
    md.mName = M;
    md.mPorts.push_back(PortDecl{x, PortDirection::In, n(7, 0)});
    md.mWires.push_back(WireDecl{y, n(3, 0)});
    ModuleSpec spec = elaborateModule(md);
    FlattenContext fc(spec, nullptr);

    auto v_id = fc.flattenExpr(Expr::id(x));
    EXPECT_EQ(v_id.size(), 8u);
    EXPECT_EQ(v_id.front().mKind, BitAtomKind::PortBit);

    auto v_slice = fc.flattenExpr(Expr::slice(x, 5, 2));
    EXPECT_EQ(v_slice.size(), 4u);
    EXPECT_EQ(v_slice.front().mBitIndex, 2u); // LSB-first; first is x[2]

    auto v_concat =
      fc.flattenExpr(Expr::concat({Expr::slice(x, 5, 2), Expr::id(y)}));
    EXPECT_EQ(v_concat.size(), 8u);
    EXPECT_EQ(v_concat.front().mKind,
              BitAtomKind::WireBit); // LSB comes from y
    EXPECT_EQ(v_concat.back().mKind, BitAtomKind::PortBit);
}

TEST(Elab, AssignWiring) {
    IdString A("A");
    IdString in("in");
    IdString out("out");

    ModuleDecl md;
    md.mName = A;
    md.mPorts.push_back(PortDecl{in, PortDirection::In, n(7, 0)});
    md.mPorts.push_back(PortDecl{out, PortDirection::Out, n(7, 0)});

    // assign out = {in[3:0], in[7:4]}
    auto lhs = Expr::id(out);
    auto rhs = Expr::concat({Expr::slice(in, 3, 0), Expr::slice(in, 7, 4)});
    md.mAssigns.push_back(AssignStmt{lhs, rhs});

    ModuleSpec spec = elaborateModule(md);
    wireAssigns(spec);

    // out[0] == in[4], out[7] == in[3]
    auto outIdx = spec.findPortIndex(out);
    auto inIdx = spec.findPortIndex(in);
    auto out0 = spec.mBitMap.portBit(outIdx, 0);
    auto in4 = spec.mBitMap.portBit(inIdx, 4);
    auto out7 = spec.mBitMap.portBit(outIdx, 7);
    auto in3 = spec.mBitMap.portBit(inIdx, 3);
    EXPECT_EQ(spec.mBitMap.netId(out0), spec.mBitMap.netId(in4));
    EXPECT_EQ(spec.mBitMap.netId(out7), spec.mBitMap.netId(in3));
}

TEST(Generate, IfAndFor) {
    IdString Top("Top");
    IdString A("A");
    IdString p_in("p_in");
    IdString p_out("p_out");
    IdString w0("w0");
    IdString w1("w1");
    IdString DO_EXTRA("DO_EXTRA");
    IdString REPL("REPL");
    IdString uA("uA");
    IdString uA2("uA2");
    IdString g_if("g_if");
    IdString g_for("g_for");

    // Callee A
    ModuleDecl modA;
    modA.mName = A;
    modA.mPorts.push_back(PortDecl{p_in, PortDirection::In, n(7, 0)});
    modA.mPorts.push_back(PortDecl{p_out, PortDirection::Out, n(7, 0)});

    // Top
    ModuleDecl top;
    top.mName = Top;
    top.mParams.emplace(DO_EXTRA, 1);
    top.mParams.emplace(REPL, 3);
    top.mWires.push_back(WireDecl{w0, n(7, 0)});
    top.mWires.push_back(WireDecl{w1, n(7, 0)});
    top.mInstances.push_back(InstanceDecl{
      uA,
      A,
      {},
      {ConnDecl{p_in, Expr::id(w0)}, ConnDecl{p_out, Expr::id(w1)}}});
    // if (DO_EXTRA) uA2
    {
        GenIfDecl gi;
        gi.mLabel = g_if;
        gi.mCond = ast::Expr(ast::IdExpr{DO_EXTRA});
        InstanceDecl x{
          uA2,
          A,
          {},
          {ConnDecl{p_in, Expr::id(w0)}, ConnDecl{p_out, Expr::id(w1)}}};
        gi.mThenBlks.push_back(std::move(x));
        top.mGenBlks.push_back(std::move(gi));
    }
    // for i in [0,REPL)
    {
        GenForDecl gf;
        gf.mLabel = g_for;
        gf.mLoopVar = IdString("i");
        gf.mStart = ast::Expr::number(0);
        gf.mLimit = ast::Expr::id(REPL);
        gf.mStep = ast::Expr::number(1);
        InstanceDecl t{
          IdString("U"),
          A,
          {},
          {ConnDecl{p_in, Expr::id(w0)}, ConnDecl{p_out, Expr::id(w1)}}};
        gf.mBlks.push_back(std::move(t));
        top.mGenBlks.push_back(std::move(gf));
    }

    // Library
    ModuleLibrary lib;
    ASTIndex idx;
    idx.emplace(Top, std::cref(top));
    idx.emplace(A, std::cref(modA));

    // Get callee spec first
    ModuleSpec& A_s = getOrCreateSpec(modA, {}, lib);

    // Top spec with params
    ModuleSpec& Top_s = getOrCreateSpec(top, {{DO_EXTRA, 1}, {REPL, 3}}, lib);

    // Link instances
    linkInstances(Top_s, idx, lib, std::cerr);
    EXPECT_EQ(Top_s.mInstances.size(), 1u /*base*/ + 1u /*if*/ + 3u /*for*/);

    // Check one binding width
    ASSERT_FALSE(Top_s.mInstances.empty());
    const auto& inst0 = Top_s.mInstances[0];
    ASSERT_FALSE(inst0.mConnections.empty());
    const auto& b0 = inst0.mConnections[0];
    EXPECT_EQ(b0.mActual.size(), 8u);
}

TEST(ModuleKey, MakeKey) {
    ParamSpec params{
      {IdString("DO_EXTRA"), 1}, {IdString("REPL"), 2}};
    std::string key = makeModuleKey("Top", params);
    // Deterministic order: DO_EXTRA,REPL
    EXPECT_EQ(key, "Top#DO_EXTRA=1,REPL=2");
}