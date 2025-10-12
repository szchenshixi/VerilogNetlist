#pragma once

#include <cstdint>
#include <iostream>

#include "hdl/util/id_string.hpp"

namespace hdl {
namespace ast {
struct IntExpr;
using ParamDecl = std::unordered_map<IdString, IntExpr, IdString::Hash>;
} // namespace ast
namespace elab {
using ParamSpec = std::unordered_map<IdString, int64_t, IdString::Hash>;
} // namespace elab

enum class PortDirection { In, Out, InOut };

const char* to_string(PortDirection d);

int64_t widthFromRange(const ast::IntExpr& msbE, const ast::IntExpr& lsbE,
                       const elab::ParamSpec& env);
int64_t widthFromRange(int64_t msb, int64_t lsb);
struct Indent {
    int mN = 0;
    explicit Indent(int n)
        : mN(n) {}
};
std::ostream& operator<<(std::ostream& os, const Indent& i);

void warn(std::ostream* diag, const std::string& msg, int indent = 0);
void error(std::ostream* diag, const std::string& msg, int indent = 0);

void update(elab::ParamSpec& out, const elab::ParamSpec& overrides);
} // namespace hdl