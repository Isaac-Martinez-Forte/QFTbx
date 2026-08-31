// Characterisation tests for the loop-shaping support structures (phase
// 8b.0 safety net): the live-node ordered list that drives every branch &
// bound, and the expression tree that implements the HC4-style contractor
// of the MR (Rambabu/ICSP) algorithm. They pin CURRENT behaviour; known
// defects are marked // BUG: and will be fixed consciously in 8b.1.

#include <gtest/gtest.h>

#include <QMap>
#include <string>

#include "Modelo/LoopShaping/EstructuraDatos/listaordenada.h"
#include "Modelo/LoopShaping/EstructuraDatos/arbol_exp.h"

#include <interval.hpp>

namespace {

using cxsc::interval;

N* node(qreal index)
{
    return new N(index);
}

TEST(OrderedList, AscendingInsertsKeepTheOrder)
{
    ListaOrdenada lista;

    lista.insertar(node(1));
    lista.insertar(node(3));
    lista.insertar(node(5));
    lista.insertar(node(8));
    lista.insertar(node(9));

    EXPECT_EQ(lista.recuperarPrimero()->getIndex(), 1);
    EXPECT_EQ(lista.recuperarUltimo()->getIndex(), 9);
}

TEST(OrderedList, SmallerThanFirstGoesToTheFront)
{
    ListaOrdenada lista;
    lista.insertar(node(5));
    lista.insertar(node(2));

    EXPECT_EQ(lista.recuperarPrimero()->getIndex(), 2);
}

TEST(OrderedList, MiddleInsertBreaksTheOrder)
{
    // BUG: insertar() places the element at i-1 instead of i, so a value
    // that belongs between two existing nodes lands BEFORE its smaller
    // neighbour. The live-node list of every branch & bound relies on this
    // order for the "first solution is the optimum" guarantee.
    ListaOrdenada lista;
    lista.insertar(node(1));
    lista.insertar(node(3));
    lista.insertar(node(5));

    lista.insertar(node(4)); // belongs between 3 and 5

    lista.borrarPrimero();   // 1
    // Current behaviour: 4 sits before 3.
    EXPECT_EQ(lista.recuperarPrimero()->getIndex(), 4);
    lista.borrarPrimero();
    EXPECT_EQ(lista.recuperarPrimero()->getIndex(), 3);
}

TEST(OrderedList, FirstRetrieveAndDeleteWork)
{
    ListaOrdenada lista;
    EXPECT_TRUE(lista.esVacia());

    lista.insertar(node(7));
    EXPECT_FALSE(lista.esVacia());

    N* primero = lista.recuperarPrimeroBorrar();
    EXPECT_EQ(primero->getIndex(), 7);
    delete primero;

    EXPECT_TRUE(lista.esVacia());
}

//---------------------------------------------------------------- exp_tree

TEST(ExpressionTree, ScalarEvaluationWithVariables)
{
    alg::exp_tree tree("1");
    tree.setFunc(std::string("2*x+3"));

    QMap<std::string, qreal> variables;
    variables.insert("x", 5.0);

    EXPECT_DOUBLE_EQ(tree.eval(&variables), 13.0);
}

TEST(ExpressionTree, ScalarEvaluationWithFunctionsAndConstants)
{
    alg::exp_tree tree("1");
    tree.setFunc(std::string("cos(0)+sqrt(9)"));

    EXPECT_DOUBLE_EQ(tree.eval(static_cast<QMap<std::string, qreal> *>(nullptr)), 4.0);
}

TEST(ExpressionTree, IntervalEvaluationEnclosesTheRange)
{
    alg::exp_tree tree("1");
    tree.setFunc(std::string("2*x+3"));

    QMap<std::string, interval> variables;
    variables.insert("x", interval(1.0, 4.0));

    const interval result = tree.eval(&variables);
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(result)), 5.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(result)), 11.0);
}

TEST(ExpressionTree, ContractionNarrowsAnInconsistentDomain)
{
    // recorrer() is the HC4-style contractor of the MR algorithm: it must
    // shrink the variable domains to the part consistent with the
    // constraint (expression >= threshold by default in the MR usage).
    alg::exp_tree tree("1");
    tree.setFunc(std::string("x-2"), 0.0, alg::MAYORIGUAL);

    QMap<std::string, interval> variables;
    variables.insert("x", interval(0.0, 10.0));

    const bool consistent = tree.recorrer(&variables);

    EXPECT_TRUE(consistent);
    EXPECT_DOUBLE_EQ(cxsc::_double(Inf(variables.value("x"))), 2.0);
    EXPECT_DOUBLE_EQ(cxsc::_double(Sup(variables.value("x"))), 10.0);
}

TEST(ExpressionTree, ContractionDetectsAnEmptyDomain)
{
    alg::exp_tree tree("1");
    tree.setFunc(std::string("x-20"), 0.0, alg::MAYORIGUAL);

    QMap<std::string, interval> variables;
    variables.insert("x", interval(0.0, 10.0));

    EXPECT_FALSE(tree.recorrer(&variables));
}

} // namespace
