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

#ifndef XDOMAIN_H
#define XDOMAIN_H

#include "XCSP3Pool.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace XCSP3Core {

    class XIntegerEntity {
    public:
        virtual int width() = 0;

        virtual int minimum() = 0;

        virtual int maximum() = 0;

        virtual void print(std::ostream& O) const = 0;

        friend std::ostream& operator<<(std::ostream& f, const XIntegerEntity& ie);

        virtual bool equals(XIntegerEntity* arg) = 0;

        virtual ~XIntegerEntity() {}
    };

    class XIntegerValue : public XIntegerEntity {
    public:
        int value;

        XIntegerValue(int v) : value(v) {}

        int width() override { return 1; }

        int minimum() override { return value; }

        int maximum() override { return value; }

        void print(std::ostream& O) const override { O << value << " "; }

        bool equals(XIntegerEntity* arg) override {
            XIntegerValue* xiv;
            if ((xiv = dynamic_cast<XIntegerValue*>(arg)) == NULL)
                return false;
            return value == xiv->value;
        }

        virtual ~XIntegerValue() {}
    };

    class XIntegerInterval : public XIntegerEntity {
    public:
        int min, max;

        XIntegerInterval(int inf, int sup) : min(inf), max(sup) {}

        int width() override { return max - min + 1; }

        int minimum() override { return min; }

        int maximum() override { return max; }

        void print(std::ostream& O) const override { O << min << ".." << max << " "; }

        bool equals(XIntegerEntity* arg) override {
            XIntegerInterval* xii;
            if ((xii = dynamic_cast<XIntegerInterval*>(arg)) == NULL)
                return false;
            return min == xii->min && max == xii->max;
        }

        virtual ~XIntegerInterval() {}
    };

    class XDomain {
        public:
        virtual ~XDomain() { }
    };

    class XDomainInteger : public XDomain {
        //        friend class XMLParser;
    protected:
        int size;
        int top{std::numeric_limits<int>::min()};

        void addEntity(XIntegerEntity* e) {
            values.push_back(e);
            size += e->width();
        }

    public:
        std::vector<XIntegerEntity*> values;

        XDomainInteger() : size(0) {}

        virtual ~XDomainInteger() {}

        int nbValues() const {
            return size;
        }

        int minimum() {
            return values[0]->minimum();
        }

        int maximum() {
            return values[values.size() - 1]->maximum();
        }

        int isInterval() {
            return size == maximum() - minimum() + 1;
        }

        void addValue(int v) {
            if (v <= top)
                throw std::runtime_error{"not sequence domain"};
            addEntity(DataPool::IntegerEntityPool.make<XIntegerValue>(top = v));
        }

        void addInterval(int min, int max) {
            if (min >= max || min <= top)
                throw std::runtime_error{"not sequence domain"};
            addEntity(DataPool::IntegerEntityPool.make<XIntegerInterval>(min, top = max));
        }

        friend std::ostream& operator<<(std::ostream& f, const XDomainInteger& d);

        bool equals(XDomainInteger* arg) const {
            if (nbValues() != arg->nbValues())
                return false;
            if (values.size() != arg->values.size())
                return false;
            for (unsigned int i = 0; i < arg->values.size(); i++) {
                if (values[i]->equals(arg->values[i]) == false)
                    return false;
            }
            return true;
        }

    };
} // namespace XCSP3Core

#endif /* XDOMAIN_H */
