/*=============================================================================
 * parser for CSP instances represented in XCSP3 Format
 *
 * Copyright (c) 2015 xcsp.org (contact <at> xcsp.org)
 * Copyright (c) 2008 Olivier ROUSSEL (olivier.roussel <at> cril.univ-artois.fr)
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

// definition of different functions coming from XCSP3Constraint, XCSPVariables, XCS3Domain
#include "XCSP3Constraint.h"
#include "XCSP3Domain.h"
#include "XCSP3Tree.h"
#include "XCSP3Variable.h"
#include "XCSP3Objective.h"
#include <assert.h>

using namespace XCSP3Core;

namespace XCSP3Core {
    // Special global vars...
    std::vector<XTransition> tr; // Not beautiful but remove code to fixed data in group constraint.
    std::string st;
    std::vector<std::string> fi;
    std::vector<int> _except;
    OrderType _op;
    std::vector<int> _values;

    int XParameterVariable::max;

    //------------------------------------------------------------------------------------------
    //  XCSP3Domain.h functions
    //------------------------------------------------------------------------------------------
    std::ostream& operator<<(std::ostream& O, const XIntegerEntity& ie) {
        ie.print(O);
        return O;
    }

    std::ostream& operator<<(std::ostream& f, const XDomainInteger& d) {
        for (XIntegerEntity* xi : d.values)
            f << *xi;
        return f;
    }
} // namespace XCSP3Core

//------------------------------------------------------------------------------------------
//  XCSP3Variable.h functions
//------------------------------------------------------------------------------------------

XEntity::XEntity() : id("") {}

XEntity::~XEntity() {}

XEntity::XEntity(std::string lid) { id = lid; }

XVariable::XVariable(std::string idd, XDomainInteger* dom) : XEntity(idd), domain(dom) {}

XVariable::XVariable(std::string idd, XDomainInteger* dom, std::vector<int> indexes) {
    domain = dom;
    std::stringstream oss;
    oss << idd;

    for (unsigned int i = 0; i < indexes.size(); i++)
        oss << "[" << indexes[i] << "]";

    id = oss.str();
}

XVariable::~XVariable() {}

XParameterVariable::XParameterVariable(std::string lid) : XVariable(lid, NULL) {
    if (id[1] == '.')
        number = -1;
    else
        number = std::stoi(id.substr(1));
    if (max < number)
        max = number;
}

namespace XCSP3Core {
    std::ostream& operator<<(std::ostream& O, const XVariable& x) {
        O << x.id;
        return O;
    }

    std::ostream& operator<<(std::ostream& O, const XInterval& it) {
        O << "[" << it.min << "," << it.max << "]";
        return O;
    }
} // namespace XCSP3Core

// Check if a XEntity is an integer
// If yes, the value is set to its integer
bool XCSP3Core::isInteger(XEntity* xe, int& value) {
    XInteger* xi;
    if ((xi = dynamic_cast<XInteger*>(xe)) != NULL) {
        value = xi->value;
        return true;
    }
    return false;
}

// Check if a XEntity is an integer
// If yes, the value is set to its integer
bool XCSP3Core::isInterval(XEntity* xe, int& min, int& max) {
    XEInterval* xi;
    if ((xi = dynamic_cast<XEInterval*>(xe)) != NULL) {
        min = xi->min;
        max = xi->max;
        return true;
    }
    return false;
}

bool XCSP3Core::isVariable(XEntity* xe, XVariable*& v) {
    XVariable* x;
    if ((x = dynamic_cast<XVariable*>(xe)) != NULL) {
        v = x;
        return true;
    }
    return false;
}

XVariableArray::XVariableArray(std::string id, std::vector<int> szs) : XEntity(id), sizes(szs.begin(), szs.end()) {
    int nb = 1;
    for (int sz : sizes)
        nb *= sz;
    variables.assign(nb, NULL);
}

XVariableArray::XVariableArray(std::string idd, XVariableArray* as) : sizes(as->sizes.begin(), as->sizes.end()) {
    std::vector<int> indexes;
    indexes.assign(as->sizes.size(), 0);
    variables.assign(as->variables.size(), NULL);
    id = idd;
    for (unsigned int i = 0; i < variables.size(); i++) {
        variables[i] = DataPool::EntityPool.make<XVariable>(idd, as->variables[i]->domain, indexes);
        for (int j = sizes.size() - 1; j >= 0; j--)
            if (++indexes[j] == sizes[j])
                indexes[j] = 0;
            else
                break;
    }
}

XVariableArray::~XVariableArray() {}

void XVariableArray::indexesFor(int flatIndex, std::vector<int>& indexes) {
    indexes.resize(sizes.size());
    for (int i = indexes.size() - 1; i > 0; i--) {
        indexes[i] = flatIndex % sizes[i];
        flatIndex = flatIndex / sizes[i];
    }
    indexes[0] = flatIndex;
}

bool XVariableArray::incrementIndexes(std::vector<int>& indexes, std::vector<XIntegerEntity*>& ranges) {
    int j = indexes.size() - 1;
    for (; j >= 0; j--)
        if (ranges[j]->width() == 1)
            continue;
        else if (++indexes[j] > ranges[j]->maximum())
            indexes[j] = ranges[j]->minimum();
        else
            break;
    return j >= 0;
}

void XVariableArray::getVarsFor(std::vector<XVariable*>& list, std::string compactForm, std::vector<int>* flatIndexes, bool storeIndexes) {
    std::vector<XIntegerEntity*> ranges;
    std::string tmp;
    // Compute the different ranges for all dimension
    for (unsigned int i = 0; i < sizes.size(); i++) {
        int pos = compactForm.find(']');
        tmp = compactForm.substr(1, pos - 1);
        compactForm = compactForm.substr(pos + 1);
        if (tmp.size() == 0) {
            ranges.push_back(DataPool::IntegerEntityPool.make<XIntegerInterval>(0, sizes[i] - 1));
        } else {
            size_t dot = tmp.find("..");
            if (dot == std::string::npos)
                ranges.push_back(DataPool::IntegerEntityPool.make<XIntegerValue>(std::stoi(tmp)));
            else {
                int first = std::stoi(tmp.substr(0, dot));
                int last = std::stoi(tmp.substr(dot + 2));
                ranges.push_back(DataPool::IntegerEntityPool.make<XIntegerInterval>(first, last));
            }
        }
    }
    // Compute the first one
    std::vector<int> indexes;
    for (unsigned int i = 0; i < sizes.size(); i++)
        indexes.push_back(ranges[i]->minimum());

    // Compute all necessary variables
    do {
        if (storeIndexes)
            flatIndexes->push_back(flatIndexFor(indexes));
        else {
            if (variables[flatIndexFor(indexes)] != nullptr)
                list.push_back(variables[flatIndexFor(indexes)]);
        }
    } while (incrementIndexes(indexes, ranges));
}

void XVariableArray::buildVarsWith(XDomainInteger* domain) {
    std::vector<int> indexes;
    indexes.assign(sizes.size(), 0);

    for (unsigned int i = 0; i < variables.size(); i++) {
        if (variables[i] == NULL) // We need to create a variable
            variables[i] = DataPool::EntityPool.make<XVariable>(id, domain, indexes);
        for (int j = sizes.size() - 1; j >= 0; j--)
            if (++indexes[j] == sizes[j])
                indexes[j] = 0;
            else
                break;
    }
}

int XVariableArray::flatIndexFor(std::vector<int> indexes) {
    int sum = 0;
    for (int i = indexes.size() - 1, nb = 1; i >= 0; i--) {
        sum += indexes[i] * nb;
        nb *= sizes[i];
    }
    return sum;
}

//------------------------------------------------------------------------------------------
//  XCSP3Variable.h functions
//------------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& O, const XInterval& it) {
    O << "[" << it.min << "," << it.max << "]";
    return O;
}

//------------------------------------------------------------------------------------------
//  XCSP3Constraint.h functions
//------------------------------------------------------------------------------------------

void XConstraint::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    group->unfoldVector(list, arguments, original->list);
}

void XConstraintGroup::unfoldVector(std::vector<XVariable*>& toUnfold, std::vector<XVariable*>& args, std::vector<XVariable*>& initial) {
    XParameterVariable* xp;
    if (initial.size() == 0)
        return;
    if ((xp = dynamic_cast<XParameterVariable*>(initial[0])) == NULL) { // non parametrized vector
        toUnfold.assign(initial.begin(), initial.end());
        return;
    }
    if (xp->number == -1) { // %...
        toUnfold.assign(args.begin() + (XParameterVariable::max == -1 ? 0 : XParameterVariable::max + 1), args.end());
        return;
    }
    for (XVariable* xv : initial) {
        xp = dynamic_cast<XParameterVariable*>(xv);
        toUnfold.push_back(args[xp->number]);
    }
}

void XConstraintGroup::unfoldString(std::string& toUnfold, std::vector<XVariable*>& args) {
    for (int i = args.size() - 1; i >= 0; i--) {
        std::string param = "%" + std::to_string(i);
        ReplaceStringInPlace(toUnfold, param, args[i]->id);
    }
}

namespace XCSP3Core {
    std::ostream& operator<<(std::ostream& O, const XCondition& xc) {
        std::string sep;
        if (xc.op == OrderType::LT)
            sep = " < ";
        if (xc.op == OrderType::LE)
            sep = " <= ";
        if (xc.op == OrderType::GT)
            sep = " > ";
        if (xc.op == OrderType::GE)
            sep = " >= ";
        if (xc.op == OrderType::EQ)
            sep = " = ";

        if (xc.operandType == OperandType::INTEGER)
            O << sep << xc.val;

        if (xc.operandType == OperandType::INTERVAL)
            O << sep << "in [" << xc.min << "," << xc.max << "]";

        if (xc.operandType == OperandType::VARIABLE)
            O << sep << xc.var;
        return O;
    }
} // namespace XCSP3Core

void XInitialCondition::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XInitialCondition* xi = dynamic_cast<XInitialCondition*>(original);
    condition = xi->condition;
    group->unfoldString(condition, arguments);
}

void XInitialCondition::extractCondition(XCondition& xc) { // Create the op and the operand (which can be a value, an interval or a XVariable)
    std::regex const rglt(R"(\(.*(le|lt|ge|gt|in|eq|ne),(.*)\).*)");
    std::smatch match;
    std::regex_match(condition, match, rglt);

    if (match.size() != 3)
        throw std::runtime_error("condition is malformed\n");

    xc.val = xc.min = xc.max = 0;
    xc.var = "";
    std::string tmp0 = match[1].str();
    std::string tmp1 = match[2].str();

    if (tmp0 == "le")
        xc.op = OrderType::LE;
    if (tmp0 == "lt")
        xc.op = OrderType::LT;
    if (tmp0 == "ge")
        xc.op = OrderType::GE;
    if (tmp0 == "gt")
        xc.op = OrderType::GT;
    if (tmp0 == "in")
        xc.op = OrderType::IN;
    if (tmp0 == "eq")
        xc.op = OrderType::EQ;
    if (tmp0 == "ne")
        xc.op = OrderType::NE;
    //std::cout << condition <<": "<< tmp0 << " " <<tmp1 << std::endl;
    //printf("%d %d\n",' ',condition[condition.length()-1]);
    size_t dotdot = tmp1.find('.');
    if (dotdot != std::string::npos) { // Normal variable
        xc.operandType = OperandType::INTERVAL;
        xc.min = stoi(tmp1.substr(0, dotdot));
        xc.max = stoi(tmp1.substr(dotdot + 2));
        return;
    }
    try {
        xc.val = stoi(tmp1);
        xc.operandType = OperandType::INTEGER;
    } catch (const std::invalid_argument& e) {
        xc.var = tmp1;
        xc.operandType = OperandType::VARIABLE;
    }
}

void XValues::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XValues* xv = dynamic_cast<XValues*>(original);
    group->unfoldVector(values, arguments, xv->values);
}

void XValue::unfoldParameters(XConstraintGroup*, std::vector<XVariable*>& arguments, XConstraint* original) {
    XParameterVariable* xp;
    XValue* xv = dynamic_cast<XValue*>(original);
    if ((xp = dynamic_cast<XParameterVariable*>(xv->value)) == NULL) {
        value = xv->value;
    } else
        value = arguments[xp->number == -1 ? 0 : xp->number];
}

void XIndex::unfoldParameters(XConstraintGroup*, std::vector<XVariable*>& arguments, XConstraint* original) {
    XParameterVariable* xp;
    XIndex* xi = dynamic_cast<XIndex*>(original);
    if (xi->index == NULL)
        return;
    if ((xp = dynamic_cast<XParameterVariable*>(xi->index)) == NULL)
        index = xi->index;
    else
        index = arguments[xp->number == -1 ? 0 : xp->number];
}

void XLengths::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XLengths* xl = dynamic_cast<XLengths*>(original);
    group->unfoldVector(lengths, arguments, xl->lengths);
}

void XConstraintExtension::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XConstraintExtension* xe = dynamic_cast<XConstraintExtension*>(original);
    isSupport = xe->isSupport;
    containsStar = xe->containsStar;
}

void XConstraintIntension::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintIntension* xi = dynamic_cast<XConstraintIntension*>(original);
    function = xi->function;
    group->unfoldString(function, arguments);
}

void XConstraintGroup::unfoldArgumentNumber(int i, XConstraint* builtConstraint) {
    builtConstraint->unfoldParameters(this, arguments[i], constraint);
    return;
}

void XConstraintAllDiffMatrix::unfoldParameters(XConstraintGroup*, std::vector<XVariable*>&, XConstraint*) {
    throw std::runtime_error("Group Alldiff Matrix and list is not yet supported");
}

void XConstraintOrdered::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XLengths::unfoldParameters(group, arguments, original);
}

void XConstraintLex::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintLex* xc = dynamic_cast<XConstraintLex*>(original);
    for (unsigned int i = 0; i < lists.size(); i++)
        group->unfoldVector(lists[i], arguments, xc->lists[i]);
}

void XConstraintLexMatrix::unfoldParameters(XConstraintGroup*, std::vector<XVariable*>&, XConstraint*) {
    throw std::runtime_error("Group lex Matrix  is not yet supported");
}

void XConstraintSum::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XValues::unfoldParameters(group, arguments, original);
    XInitialCondition::unfoldParameters(group, arguments, original);
    assert(values.size() == list.size() || values.size() == 0);
}

void XConstraintNValues::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XInitialCondition::unfoldParameters(group, arguments, original);
}

void XConstraintCardinality::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintCardinality* xc = dynamic_cast<XConstraintCardinality*>(original);
    closed = xc->closed;
    XConstraint::unfoldParameters(group, arguments, original);
    XValues::unfoldParameters(group, arguments, original);
    group->unfoldVector(occurs, arguments, xc->occurs);
}

void XConstraintCount::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XValues::unfoldParameters(group, arguments, original);
    XInitialCondition::unfoldParameters(group, arguments, original);
}

void XConstraintMaximum::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintMaximum* xc = dynamic_cast<XConstraintMaximum*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XIndex::unfoldParameters(group, arguments, original);
    XInitialCondition::unfoldParameters(group, arguments, original);
    startIndex = xc->startIndex;
    rank = xc->rank;
}

void XConstraintElement::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintElement* xc = dynamic_cast<XConstraintElement*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XIndex::unfoldParameters(group, arguments, original);
    XValue::unfoldParameters(group, arguments, original);
    startIndex = xc->startIndex;
    rank = xc->rank;
}

void XConstraintElementMatrix::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintElementMatrix* xc = dynamic_cast<XConstraintElementMatrix*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XIndex::unfoldParameters(group, arguments, original);
    XValue::unfoldParameters(group, arguments, original);
    startColIndex = xc->startColIndex;
    startRowIndex = xc->startRowIndex;
    XParameterVariable* xp;
    if ((xp = dynamic_cast<XParameterVariable*>(xc->index2)) == nullptr)
        index2 = xc->index2;
    else
        index2 = arguments[xp->number == -1 ? 0 : xp->number];

    matrix = xc->matrix;
}

void XConstraintChannel::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintChannel* xc = dynamic_cast<XConstraintChannel*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XValue::unfoldParameters(group, arguments, original);
    group->unfoldVector(secondList, arguments, xc->secondList);
    startIndex1 = xc->startIndex1;
    startIndex2 = xc->startIndex2;
}

void XConstraintNoOverlap::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraint::unfoldParameters(group, arguments, original);
    XLengths::unfoldParameters(group, arguments, original);
    XConstraintNoOverlap* xc = dynamic_cast<XConstraintNoOverlap*>(original);
    zeroIgnored = xc->zeroIgnored;
}

void XConstraintCumulative::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintCumulative* xc = dynamic_cast<XConstraintCumulative*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XLengths::unfoldParameters(group, arguments, original);
    XInitialCondition::unfoldParameters(group, arguments, original);
    group->unfoldVector(origins, arguments, xc->origins);
    group->unfoldVector(ends, arguments, xc->ends);
    group->unfoldVector(heights, arguments, xc->heights);
}

void XConstraintStretch::unfoldParameters(XConstraintGroup*, std::vector<XVariable*>&, XConstraint*) {
    throw std::runtime_error("group is not yet allowed with stretch constraint");
}

void XConstraintCircuit::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    XConstraintCircuit* xc = dynamic_cast<XConstraintCircuit*>(original);
    XConstraint::unfoldParameters(group, arguments, original);
    XValue::unfoldParameters(group, arguments, original);
    startIndex = xc->startIndex;
}

void XConstraintClause::unfoldParameters(XConstraintGroup* group, std::vector<XVariable*>& arguments, XConstraint* original) {
    (void)group;
    (void)original;
    for (XVariable* xv : arguments) {
        if (dynamic_cast<XTree*>(xv) != nullptr) { // not
            if (xv->id.rfind("not(", 0) != 0)
                throw std::runtime_error("a clause is malformed in a group: " + xv->id);
            std::string name = xv->id.substr(4, xv->id.length() - 5);
            negative.push_back(DataPool::EntityPool.make<XVariable>(name, nullptr)); // TODO: improvements needed here
        } else {
            positive.push_back(xv);
        }
    }
}

//------------------------------------------------------------------------------------------
//  XCSP3Utils.h functions
//------------------------------------------------------------------------------------------
std::vector<std::string>& XCSP3Core::split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> XCSP3Core::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

template <typename Base, typename T>
inline bool XCSP3Core:: instanceof (const T*) {
    return std::is_base_of<Base, T>::value;
}

void XCSP3Core::ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::string& XCSP3Core::ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun(std::iswspace))));
    return s;
}

std::string& XCSP3Core::rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun(std::iswspace))).base(), s.end());
    return s;
}

std::string& XCSP3Core::removeChar(std::string& s, char c) {
    std::string::size_type begin = s.find_first_not_of(c);
    std::string::size_type end = s.find_last_not_of(c);
    s = s.substr(begin, end - begin + 1);
    return s;
}

std::string& XCSP3Core::trim(std::string& s) {
    return ltrim(rtrim(s));
    s = removeChar(s, '\n');
    s = removeChar(s, '\r');
    s = removeChar(s, '\f');
    s = removeChar(s, '\t');
    s = removeChar(s, ' ');
    s = removeChar(s, 1);
    return s;
}
