/**
 * @file
 * @brief The compilation of a tree into its postfix program, and the
 * program's evaluation on the processor.
 *
 * A small whole number is one of at most 64 in magnitude; a whole power is
 * taken by repeated squaring and a negative one is the reciprocal of the
 * positive, the tree's own rule and arithmetic.
 */

#include "src/core/math/expression_program.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

#include "src/core/math/constants.h"
#include "src/core/math/expression_tree.h"

namespace qftbx {

namespace {

bool isSmallWholeNumber(double value)
{
    return value == std::floor(value) && std::abs(value) <= 64.0;
}

std::complex<double> wholePower(std::complex<double> base, int exponent)
{
    unsigned count = exponent < 0 ? 0u - static_cast<unsigned>(exponent) : static_cast<unsigned>(exponent);
    std::complex<double> power = count % 2 != 0 ? base : std::complex<double>(1.0, 0.0);
    while ((count >>= 1) != 0) {
        base = base * base;
        if (count % 2 != 0) {
            power = power * base;
        }
    }
    return exponent < 0 ? std::complex<double>(1.0, 0.0) / power : power;
}

ProgramOp unaryOp(type_node type)
{
    switch (type) {
    case SIN: return ProgramOp::Sin;
    case COS: return ProgramOp::Cos;
    case TAN: return ProgramOp::Tan;
    case ATAN: return ProgramOp::Atan;
    case SINH: return ProgramOp::Sinh;
    case COSH: return ProgramOp::Cosh;
    case TANH: return ProgramOp::Tanh;
    case ASIN: return ProgramOp::Asin;
    case ACOS: return ProgramOp::Acos;
    case EXP: return ProgramOp::Exp;
    case ABS: return ProgramOp::Abs;
    case LN: return ProgramOp::Ln;
    case LG: return ProgramOp::Lg;
    case LOG2: return ProgramOp::Log2;
    case SQRT: return ProgramOp::Sqrt;
    default:
        throw std::invalid_argument("ExpressionProgram: a node the compiler does not know.");
    }
}

std::size_t emit(const exp_node * node, std::vector<ProgramStep> & steps)
{
    switch (node->type) {
    case CONSTANT:
        steps.push_back({ProgramOp::Constant, 0, node->c_const});
        return 1;
    case E:
        steps.push_back({ProgramOp::Constant, 0, math::kE});
        return 1;
    case PI:
        steps.push_back({ProgramOp::Constant, 0, math::kPi});
        return 1;
    case VAR:
        if (node->index < 0) {
            throw std::invalid_argument("ExpressionProgram: the variable '" + node->var
                                        + "' is not bound to a value (bind() first).");
        }
        steps.push_back({ProgramOp::Variable, node->index, 0.0});
        return 1;
    case ADD:
    case SUBTRACT:
    case MULTIPLY:
    case DIVIDE: {
        const std::size_t left = emit(node->left.get(), steps);
        const std::size_t right = emit(node->right.get(), steps);
        const ProgramOp op = node->type == ADD ? ProgramOp::Add
                           : node->type == SUBTRACT ? ProgramOp::Subtract
                           : node->type == MULTIPLY ? ProgramOp::Multiply : ProgramOp::Divide;
        steps.push_back({op, 0, 0.0});
        return std::max(left, right + 1);
    }
    case POWER: {
        const exp_node * exponent = node->right.get();
        if (exponent->type == CONSTANT && isSmallWholeNumber(exponent->c_const)) {
            const std::size_t base = emit(node->left.get(), steps);
            steps.push_back({ProgramOp::WholePower, static_cast<std::int32_t>(exponent->c_const), 0.0});
            return base;
        }
        const std::size_t left = emit(node->left.get(), steps);
        const std::size_t right = emit(exponent, steps);
        steps.push_back({ProgramOp::Power, 0, 0.0});
        return std::max(left, right + 1);
    }
    default: {
        const std::size_t operand = emit(node->left.get(), steps);
        steps.push_back({unaryOp(node->type), 0, 0.0});
        return operand;
    }
    }
}

}

ExpressionProgram ExpressionProgram::compile(const ExpressionTree & tree)
{
    ExpressionProgram program;
    if (tree.tree() == nullptr) {
        throw std::invalid_argument("ExpressionProgram: the tree is empty.");
    }
    program.depth = emit(tree.tree(), program.steps);
    return program;
}

std::complex<double> ExpressionProgram::evaluate(const std::vector<std::complex<double>> & values) const
{
    using Complex = std::complex<double>;
    std::vector<Complex> stack;
    stack.reserve(depth);

    const auto pop = [&stack]() {
        const Complex top = stack.back();
        stack.pop_back();
        return top;
    };

    for (const ProgramStep & step : steps) {
        switch (step.op) {
        case ProgramOp::Constant: stack.push_back(Complex(step.constant, 0.0)); break;
        case ProgramOp::Variable:
            if (static_cast<std::size_t>(step.operand) >= values.size()) {
                throw std::invalid_argument("ExpressionProgram: a variable without a value.");
            }
            stack.push_back(values[static_cast<std::size_t>(step.operand)]);
            break;
        case ProgramOp::Add: { const Complex b = pop(); const Complex a = pop(); stack.push_back(a + b); break; }
        case ProgramOp::Subtract: { const Complex b = pop(); const Complex a = pop(); stack.push_back(a - b); break; }
        case ProgramOp::Multiply: { const Complex b = pop(); const Complex a = pop(); stack.push_back(a * b); break; }
        case ProgramOp::Divide: { const Complex b = pop(); const Complex a = pop(); stack.push_back(a / b); break; }
        case ProgramOp::Power: { const Complex b = pop(); const Complex a = pop(); stack.push_back(std::pow(a, b)); break; }
        case ProgramOp::WholePower: stack.push_back(wholePower(pop(), step.operand)); break;
        case ProgramOp::Sin: stack.push_back(std::sin(pop())); break;
        case ProgramOp::Cos: stack.push_back(std::cos(pop())); break;
        case ProgramOp::Tan: stack.push_back(std::tan(pop())); break;
        case ProgramOp::Atan: stack.push_back(std::atan(pop())); break;
        case ProgramOp::Sinh: stack.push_back(std::sinh(pop())); break;
        case ProgramOp::Cosh: stack.push_back(std::cosh(pop())); break;
        case ProgramOp::Tanh: stack.push_back(std::tanh(pop())); break;
        case ProgramOp::Asin: stack.push_back(std::asin(pop())); break;
        case ProgramOp::Acos: stack.push_back(std::acos(pop())); break;
        case ProgramOp::Exp: stack.push_back(std::exp(pop())); break;
        case ProgramOp::Abs: stack.push_back(Complex(std::abs(pop()), 0.0)); break;
        case ProgramOp::Ln: stack.push_back(std::log(pop())); break;
        case ProgramOp::Lg: stack.push_back(std::log10(pop())); break;
        case ProgramOp::Log2: stack.push_back(std::log(pop()) / std::log(2.0)); break;
        case ProgramOp::Sqrt: stack.push_back(std::sqrt(pop())); break;
        }
    }
    return stack.back();
}

}
