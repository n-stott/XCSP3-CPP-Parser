/*=============================================================================
 * parser for CSP instances represented in XCSP3 Format
 *
 * Copyright (c) 2015 xcsp.org (contact <at> xcsp.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *=============================================================================
 */

#include <map>

#include "XCSP3Tree.h"
#include "XCSP3TreeNode.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <vector>

using namespace XCSP3Core;

bool XCSP3Core::isSymmetricOperator(Expr type) {
    return type == Expr::ADD || type == Expr::MUL || type == Expr::MIN || type == Expr::MAX || type == Expr::DIST || type == Expr::NE || type == Expr::EQ || type == Expr::SET || type == Expr::AND || type == Expr::OR || type == Expr::XOR || type == Expr::IFF ||
           type == Expr::UNION || type == Expr::INTER || type == Expr::DJOINT;
}

bool XCSP3Core::isNonSymmetricRelationalOperator(Expr type) {
    return type == Expr::LT || type == Expr::LE || type == Expr::GE || type == Expr::GT;
}

bool XCSP3Core::isPredicateOperator(Expr type) {
    return isRelationalOperator(type) || type == Expr::NOT || type == Expr::IMP || type == Expr::AND || type == Expr::OR || type == Expr::AND || type == Expr::OR || type == Expr::XOR ||
           type == Expr::IFF || type == Expr::IN || type == Expr::NOTIN;
}

bool XCSP3Core::isRelationalOperator(Expr type) {
    return XCSP3Core::isNonSymmetricRelationalOperator(type) || type == Expr::NE || type == Expr::EQ;
}

Expr XCSP3Core::arithmeticInversion(Expr type) {
    return type == Expr::LT ? Expr::GT : type == Expr::LE ? Expr::GE : type == Expr::GE ? Expr::LE : type == Expr::GT ? Expr::LT : type; // no change for NE and EQ
}

std::string XCSP3Core::operatorToString(Expr op) {
    if (op == Expr::NEG)
        return "neg";
    if (op == Expr::ABS)
        return "abs";

    if (op == Expr::ADD)
        return "add";
    if (op == Expr::SUB)
        return "sub";
    if (op == Expr::MUL)
        return "mul";
    if (op == Expr::DIV)
        return "div";
    if (op == Expr::MOD)
        return "mod";

    if (op == Expr::SQR)
        return "sqr";
    if (op == Expr::POW)
        return "pow";

    if (op == Expr::MIN)
        return "min";
    if (op == Expr::MAX)
        return "max";

    if (op == Expr::DIST)
        return "dist";

    if (op == Expr::LE)
        return "le";
    if (op == Expr::LT)
        return "lt";
    if (op == Expr::GE)
        return "ge";
    if (op == Expr::GT)
        return "gt";

    if (op == Expr::NE)
        return "ne";
    if (op == Expr::EQ)
        return "eq";

    if (op == Expr::NOT)
        return "not";
    if (op == Expr::AND)
        return "and";
    if (op == Expr::OR)
        return "or";
    if (op == Expr::XOR)
        return "xor";
    if (op == Expr::IMP)
        return "imp";
    if (op == Expr::IF)
        return "if";
    if (op == Expr::IFF)
        return "iff";

    if (op == Expr::IN)
        return "in";
    if (op == Expr::NOTIN)
        return "notin";
    if (op == Expr::SET)
        return "set";
    //assert(false);
    return "oundef";
}

NodeOperator* createNodeOperator(std::string op) {
    NodeOperator* tmp = nullptr;
    if (op == "neg")
        tmp = new NodeNeg();
    if (op == "abs")
        tmp = new NodeAbs();

    if (op == "add")
        tmp = new NodeAdd();
    if (op == "sub")
        tmp = new NodeSub();
    if (op == "mul")
        tmp = new NodeMult();
    if (op == "div")
        tmp = new NodeDiv();
    if (op == "mod")
        tmp = new NodeMod();

    if (op == "sqr")
        tmp = new NodeSquare();
    if (op == "pow")
        tmp = new NodePow();

    if (op == "min")
        tmp = new NodeMin();
    if (op == "max")
        tmp = new NodeMax();
    if (op == "dist")
        tmp = new NodeDist();

    if (op == "le")
        tmp = new NodeLE();
    if (op == "lt")
        tmp = new NodeLT();
    if (op == "ge")
        tmp = new NodeGE();
    if (op == "gt")
        tmp = new NodeGT();

    if (op == "ne")
        tmp = new NodeNE();
    if (op == "eq")
        tmp = new NodeEQ();

    if (op == "not")
        tmp = new NodeNot();
    if (op == "and")
        tmp = new NodeAnd();
    if (op == "or")
        tmp = new NodeOr();
    if (op == "xor")
        tmp = new NodeXor();
    if (op == "imp")
        tmp = new NodeImp();
    if (op == "if")
        tmp = new NodeIf();
    if (op == "iff")
        tmp = new NodeIff();

    if (op == "in")
        tmp = new NodeIn();
    if (op == "notin")
        tmp = new NodeNotIn();
    if (op == "set")
        tmp = new NodeSet();

    assert(tmp != nullptr);

    return tmp;
}

Expr XCSP3Core::logicalInversion(Expr type) {
    return type == Expr::LT ? Expr::GE
                            : type == Expr::LE ? Expr::GT
                                               : type == Expr::GE ? Expr::LT
                                                                  : type == Expr::GT ? Expr::LE
                                                                                     : type == Expr::NE ? Expr::EQ
                                                                                                        : type == Expr::EQ ? Expr::NE
                                                                                                                           : type == Expr::IN ? Expr::NOTIN
                                                                                                                                              : type ==
                                                                                                                                                        Expr::NOTIN
                                                                                                                                                    ? Expr::IN
                                                                                                                                                    : type ==
                                                                                                                                                              Expr::SUBSET
                                                                                                                                                          ? Expr::SUPSEQ
                                                                                                                                                          : type ==
                                                                                                                                                                    Expr::SUBSEQ
                                                                                                                                                                ? Expr::SUPSET
                                                                                                                                                                : type ==
                                                                                                                                                                          Expr::SUPSEQ
                                                                                                                                                                      ? Expr::SUBSET
                                                                                                                                                                      : type ==
                                                                                                                                                                                Expr::SUPSET
                                                                                                                                                                            ? Expr::SUBSEQ
                                                                                                                                                                            : Expr::UNDEF;
}

int equalNodes(Node* a, Node* b) { // return -1 if a<0, 0 if a=b, +1 si a>b
    if (a->type != b->type)
        return static_cast<int>(a->type) - static_cast<int>(b->type);

    NodeConstant *c1 = dynamic_cast<NodeConstant*>(a), *c2 = dynamic_cast<NodeConstant*>(b);
    if (c1 != nullptr)
        return c1->val - c2->val;

    NodeVariable *v1 = dynamic_cast<NodeVariable*>(a), *v2 = dynamic_cast<NodeVariable*>(b);
    if (v1 != nullptr)
        return v1->var.compare(v2->var);

    NodeOperator *o1 = dynamic_cast<NodeOperator*>(a), *o2 = dynamic_cast<NodeOperator*>(b);
    if (o1->parameters.size() < o2->parameters.size())
        return -1;
    if (o1->parameters.size() > o2->parameters.size())
        return +1;

    for (unsigned int i = 0; i < o1->parameters.size() - 1; i++) {
        int cmp = equalNodes(o1->parameters[i], o2->parameters[i]);
        if (cmp != 0)
            return cmp;
    }
    return equalNodes(o1->parameters.back(), o2->parameters.back());
}

bool compareNodes(Node* a, Node* b) {
    return equalNodes(a, b) < 0;
}

bool pattern(Node* node, std::string s,
             std::vector<Expr>& operators, std::vector<int>& constants, std::vector<std::string>& variables, bool fakeR = false) {
    constants.clear();
    variables.clear();
    operators.clear();
    Tree pattern(s);
    if (fakeR)
        pattern.root->type = Expr::FAKEOP;
    return Node::areSimilar(node, pattern.root, operators, constants, variables);
}

Node* NodeOperator::canonize() {
    std::vector<int> constants;
    std::vector<std::string> variables;
    std::vector<Expr> operators;

    std::vector<Node*> newParams;
    for (Node* n : parameters)
        newParams.push_back(n->canonize());

    if (isSymmetricOperator(type))
        std::sort(newParams.begin(), newParams.end(), compareNodes);

    Expr newType = type;

    // newParams are potentially sorted if the type corresponds to a non-symmetric binary relational operator (in that case, we swap newParams and
    // arithmetically
    // inverse the operator)
    if (newParams.size() == 2 && isNonSymmetricRelationalOperator(type) &&
        (static_cast<int>(arithmeticInversion(type)) < static_cast<int>(type) || (arithmeticInversion(type) == type && equalNodes(newParams[0], newParams[1]) > 0))) {
        newType = arithmeticInversion(type);
        Node* tmp = newParams[0];
        newParams[0] = newParams[1];
        newParams[1] = tmp;
        return createNodeOperator(operatorToString(newType))->addParameters(newParams)->canonize();
    }

    // Now, some specific reformulation rules are applied
    if (newType == Expr::LT && newParams[1]->type == Expr::DECIMAL) { // lt(x,k) becomes le(x,k-1)
        NodeConstant* c = dynamic_cast<NodeConstant*>(newParams[1]);
        c->val = c->val - 1;
        return (new NodeLE())->addParameter(newParams[0])->addParameter(newParams[1])->canonize();
    }
    if (newType == Expr::LT && newParams[0]->type == Expr::DECIMAL) { // lt(k,x) becomes le(k+1,x)
        NodeConstant* c = dynamic_cast<NodeConstant*>(newParams[0]);
        c->val = c->val + 1;
        return (new NodeLE())->addParameter(newParams[0])->addParameter(newParams[1])->canonize();
    }

    NodeOperator* tmp = dynamic_cast<NodeOperator*>(newParams[0]); // abs(sub becomes dist
    if (newType == Expr::ABS && newParams[0]->type == Expr::SUB)
        return (new NodeDist())->addParameters(tmp->parameters)->canonize();

    if (newType == Expr::NOT && newParams[0]->type == Expr::NOT) // NOT(NOT.. becomes ..
        return tmp->parameters[0]->canonize();

    if (newType == Expr::NEG && newParams[0]->type == Expr::NEG) // neg(neg(...)) becomes ...
        return tmp->parameters[0]->canonize();

    if (newType == Expr::NOT && logicalInversion(newParams[0]->type) != Expr::UNDEF) // not(lt(...)) becomes ge(...), not(eq(...)) becomes ne(...), and
        return createNodeOperator(operatorToString(logicalInversion(newParams[0]->type)))->addParameters(tmp->parameters)->canonize();

    if (newParams.size() == 1 && (newType == Expr::ADD || newType == Expr::MUL || newType == Expr::MIN || newType == Expr::MAX || newType == Expr::EQ || newType == Expr::AND || newType == Expr::OR || newType == Expr::XOR || newType == Expr::IFF)) // certainly can happen during the canonization process
        return newParams[0];

    if (newType == Expr::ADD || newType == Expr::MUL) { // we merge constant (similar operations possible for MUL, MIN, ...)
        // They are at the end of the add
        NodeConstant *c1, *c2;
        if (newParams.size() >= 2 && (c1 = dynamic_cast<NodeConstant*>(newParams[newParams.size() - 1])) != nullptr &&
            (c2 = dynamic_cast<NodeConstant*>(newParams[newParams.size() - 2])) != nullptr) {
            std::vector<Node*> l;
            l.insert(l.end(), newParams.begin(), newParams.end() - 2);
            l.push_back(newType == Expr::ADD ? new NodeConstant(c1->val + c2->val) : new NodeConstant(c1->val * c2->val));

            if (newType == Expr::ADD)
                return ((new NodeAdd())->addParameters(l))->canonize();
            else
                return ((new NodeMult())->addParameters(l))->canonize();
        }
    }

    //le(add(y[4],5),7) -> le(y[4],2)
    if (pattern(this, "le(add(y[4],5),7)", operators, constants, variables, true)) {
        if (newType == Expr::EQ || newType == Expr::NE || newType == Expr::LE || newType == Expr::LT)
            return createNodeOperator(operatorToString(newType))
                ->addParameter(new NodeVariable(variables[0]))
                ->addParameter(new NodeConstant(constants[1] - constants[0]))
                ->canonize();
    }

    //le(8,add(5,y[4])) -> le(3, y[4])
    if (pattern(this, "le(8,add(y[4],5))", operators, constants, variables, true)) {
        if (newType == Expr::EQ || newType == Expr::NE || newType == Expr::LE || newType == Expr::LT)
            return createNodeOperator(operatorToString(newType))
                ->addParameter(new NodeConstant(constants[0] - constants[1]))
                ->addParameter(new NodeVariable(variables[0]))
                ->canonize();
    }

    //le(8,add(5,y[4]))->le(3, y[4])
    if (pattern(this, "le(8,add(5,y[4]))", operators, constants, variables, true)) {
        if (newType == Expr::EQ || newType == Expr::NE || newType == Expr::LE || newType == Expr::LT)
            return createNodeOperator(operatorToString(newType))
                ->addParameter(new NodeConstant(constants[0] - constants[1]))
                ->addParameter(new NodeVariable(variables[0]))
                ->canonize();
    }

    // eq(mul(y[0],3),9) -> eq(y[0],3)
    if (pattern(this, "eq(mul(y[0],3),9)", operators, constants, variables) ||
        pattern(this, "eq(mul(3,x),6)", operators, constants, variables)) {
        if (constants[1] % constants[0] != 0)
            return new NodeConstant(0);
        return (new NodeEQ())->addParameter(new NodeVariable(variables[0]))->addParameter(new NodeConstant(constants[1] / constants[0]))->canonize();
    }

    //eq(9,mul(3,y[0]))
    if (pattern(this, "eq(9,mul(3,y[0]))", operators, constants, variables) ||
        pattern(this, "eq(9,mul(y[0],3))", operators, constants, variables)) {
        if (constants[0] % constants[1] != 0)
            return new NodeConstant(0);
        return (new NodeEQ())->addParameter(new NodeVariable(variables[0]))->addParameter(new NodeConstant(constants[0] / constants[1]))->canonize();
    }

    // Then, we merge operators when possible; for example add(add(x,y),z) becomes add(x,y,z)
    if (isSymmetricOperator(newType) && newType != Expr::EQ && newType != Expr::DIST && newType != Expr::DJOINT) {
        for (unsigned int i = 0; i < newParams.size(); i++) {
            NodeOperator* n;
            if ((n = dynamic_cast<NodeOperator*>(newParams[i])) != nullptr && n->type == newType) {
                std::vector<Node*> list;
                for (unsigned int j = 0; j < i; j++)
                    list.push_back(newParams[j]);

                list.insert(list.end(), n->parameters.begin(), n->parameters.end());

                for (unsigned int j = i + 1; j < newParams.size(); j++)
                    list.push_back(newParams[j]);
                return ((createNodeOperator(operatorToString(newType)))->addParameters(list))->canonize();
            }
        }
    }
    if (newParams.size() == 2 && isRelationalOperator(type)) {
        NodeOperator* n0 = dynamic_cast<NodeOperator*>(newParams[0]);
        NodeOperator* n1 = dynamic_cast<NodeOperator*>(newParams[1]);
        // First, we replace sub by add when possible
        if (newParams[0]->type == Expr::SUB && newParams[1]->type == Expr::SUB) {
            Node* a = (new NodeAdd())->addParameter(n0->parameters[0])->addParameter(n1->parameters[1]);
            Node* b = (new NodeAdd())->addParameter(n1->parameters[0])->addParameter(n0->parameters[1]);
            return (createNodeOperator(operatorToString(newType)))->addParameter(a)->addParameter(b)->canonize();
        } else if (newParams[1]->type == Expr::SUB) {
            Node* a = (new NodeAdd())->addParameter(newParams[0])->addParameter(n1->parameters[1]);
            Node* b = n1->parameters[0];
            return (createNodeOperator(operatorToString(newType)))->addParameter(a)->addParameter(b)->canonize();
        } else if (n0 != nullptr && n0->op == "sub") {
            Node* a = n0->parameters[0];
            Node* b = (new NodeAdd())->addParameter(newParams[1])->addParameter(n0->parameters[1]);
            return (createNodeOperator(operatorToString(newType)))->addParameter(a)->addParameter(b)->canonize();
        }

        // next, we remove some add when possible
        if (newParams[0]->type == Expr::ADD && newParams[1]->type == Expr::DECIMAL) {
            if (n0->parameters.size() == 2 && n0->parameters[0]->type == Expr::VAR && n0->parameters[1]->type == Expr::DECIMAL) {
                NodeConstant* c1 = dynamic_cast<NodeConstant*>(newParams[1]);
                NodeConstant* c2 = dynamic_cast<NodeConstant*>(n0->parameters[1]);
                return (createNodeOperator(operatorToString(newType)))->addParameter(n0->parameters[0])->addParameter(new NodeConstant(c1->val - c2->val))->canonize();
            }
        }

        if (n0 != nullptr && n1 != nullptr && n0->type == Expr::ADD && n1->type == Expr::ADD) {
            NodeConstant *c1, *c2;
            if (n0->parameters.size() == 2 && n1->parameters.size() == 2 &&
                (c1 = dynamic_cast<NodeConstant*>(n0->parameters[1])) != nullptr &&
                (c2 = dynamic_cast<NodeConstant*>(n1->parameters[1])) != nullptr) {
                c1->val = c1->val - c2->val;
                newParams[1] = n1->parameters[0];
                return (createNodeOperator(operatorToString(newType)))->addParameters(newParams)->canonize();
            }
        }
    }

    return (createNodeOperator(operatorToString(newType)))->addParameters(newParams);
}

// -----------------------------------------
// Compare Two trees in order to find primitives
// -----------------------------------------

bool Node::areSimilar(Node* canonized, Node* pattern, std::vector<Expr>& operators, std::vector<int>& constants, std::vector<std::string>& variables) {
    if (pattern->type == Expr::FAKEOP) {
        operators.push_back(canonized->type);
    } else {
        if (pattern->type != canonized->type)
            return false;

        if (pattern->type == Expr::DECIMAL) {
            NodeConstant* c = dynamic_cast<NodeConstant*>(canonized);
            constants.push_back(c->val);
            return true;
        }

        if (pattern->type == Expr::VAR) {
            NodeVariable* v = dynamic_cast<NodeVariable*>(canonized);
            variables.push_back(v->var);
            return true;
        }
    }
    if (pattern->type == Expr::SET) {
        for (Node* n : canonized->parameters)
            if (n->type != Expr::DECIMAL)
                return false;
        return true;
    }

    NodeOperator* nc = dynamic_cast<NodeOperator*>(canonized);
    NodeOperator* np = dynamic_cast<NodeOperator*>(pattern);
    if (nc->parameters.size() != np->parameters.size())
        return false;

    for (unsigned int i = 0; i < nc->parameters.size(); i++) {
        if (Node::areSimilar(nc->parameters[i], np->parameters[i], operators, constants, variables) == false)
            return false;
    }
    return true;
}
