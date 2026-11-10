/**
 * @file
 * @brief An expression tree flattened into a postfix program, to evaluate it
 * many times over complex values.
 *
 * compile() walks a bound tree in post order: each node becomes one step
 * that pops its operands from a stack and pushes its result, so evaluating
 * the program performs the tree's operations on the same operands. On the
 * processor it gives what ExpressionTree::evaluate gives: to the bit in a
 * build that does not fuse products and sums, and to the last bits in one
 * that does, since the compiler fuses the two differently. A power whose
 * exponent is a small whole constant is a step of its own, taken by
 * repeated squaring as the tree takes it. The program is plain data and
 * its layout is the same on the processor and on the GPU, which interprets
 * it; depth is the stack a run needs. compile() throws
 * std::invalid_argument for a variable the tree has not bound.
 */

#ifndef QFTBX_EXPRESSION_PROGRAM_H
#define QFTBX_EXPRESSION_PROGRAM_H

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qftbx {

class ExpressionTree;

enum class ProgramOp : std::int32_t {
    Constant, Variable,
    Add, Subtract, Multiply, Divide, WholePower, Power,
    Sin, Cos, Tan, Atan, Sinh, Cosh, Tanh, Asin, Acos,
    Exp, Abs, Ln, Lg, Log2, Sqrt
};

struct ProgramStep
{
    ProgramOp op = ProgramOp::Constant;
    std::int32_t operand = 0;
    double constant = 0.0;
};

struct ExpressionProgram
{
    std::vector<ProgramStep> steps;
    std::size_t depth = 0;

    static ExpressionProgram compile(const ExpressionTree & tree);

    std::complex<double> evaluate(const std::vector<std::complex<double>> & values) const;
};

}

#endif
