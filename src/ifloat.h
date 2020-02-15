#pragma once
#ifndef IFLOAT_H
#define IFLOAT_H

#include <QDebug>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
using boost::multiprecision::number;
using boost::multiprecision::cpp_dec_float;
using boost::multiprecision::cpp_dec_float_50;
using boost::numeric_cast;
using std::cout;

typedef cpp_dec_float_50 float50;

struct iFloatException
{
    std::string m_msg;
    iFloatException( const char*const msg ) : m_msg(msg) {}
    iFloatException( const iFloatException& ve ) : m_msg(ve.m_msg) {}
};

//----------------      TUTORIAL ON BOOST C++ FLOAT
// Download: https://stackoverflow.com/questions/18562603/how-to-configure-qt-creator-to-use-boost-in-windows
// https://www.boost.org/doc/libs/1_65_0/libs/multiprecision/doc/html/boost_multiprecision/tut/input_output.html
// https://www.boost.org/doc/libs/1_66_0/libs/math/doc/html/math_toolkit/high_precision/use_multiprecision.html
// https://www.boost.org/doc/libs/1_34_1/libs/numeric/conversion/doc/numeric_cast.html
//----------------------------------------------------------------------------------------------------------------------------------


// iFloat is the wrapper class for Boost C++ class cpp_dec_float_50 (float50), which is float that has 50 decimal places precision
// We need a wrapper class because float50 can use a lot of memory. In a function, it will cause stack overflow very quickly.
// Using a wrapper class, the memory is on the heap instead, so there's no problem.

// Usage:
// Use this class just like the normal numeric class: float. Operator =, +, -, *, /, and conversion to other numeric types are supported.

class iFloat
{
private:
    float50 *value;

    void cleanup() {
        if (value==nullptr)
            throw iFloatException("NullPointerException: Somehow object is destructed wrongly");
        delete value;
    }

    void copy(const iFloat& v) {
        value = new float50;
        *value = *(v.value);
    }

public:
    //--------------------  CANONICAL FORM FUNCTIONS
    iFloat() {
        value = new float50;
        *value = 0;
    };


    iFloat(double x) {
        value = new float50;
        *value = x;
    }


    iFloat(const iFloat& v) {
        copy(v);
    }

    ~iFloat() {
        cleanup();
    }

    iFloat& operator = (const iFloat& v) {
        if (this!=&v) {
            // usually we need to cleanup(), but Boost C++ class float50 does not cause memory leak
            *value = *(v.value);
        }
        return *this;
    }


    //--------------------------------- CAST OPERATORS
    /*
    operator int() const {return (*value).convert_to<int>();}

    operator long long() const {return (*value).convert_to<long long>();}

    operator float() const {return (*value).convert_to<float>();}
    */
    operator double() const {return (*value).convert_to<double>();}


    //-------------------------------- CALCULATION OPERATORS
    iFloat operator + (const iFloat& v) const {
        iFloat res;
        (*res.value) = (*value) + (*v.value);
        return res;
    }

    iFloat operator - (const iFloat& v) const {
        iFloat res;
        (*res.value) = (*value) - (*v.value);
        return res;
    }

    iFloat operator * (const iFloat& v) const {
        iFloat res;
        (*res.value) = (*value) * (*v.value);
        return res;
    }

    iFloat operator / (const iFloat& v) const {
        iFloat res;
        (*res.value) = (*value) / (*v.value);
        return res;
    }

    iFloat operator - () const {
        iFloat res;
        (*res.value) = -(*value);
        return res;
    }


    //-------------------------------- SCALAR CALCULATION OPERATORS
    template <class dtype2>
    iFloat operator + (const dtype2& v) const {
        qDebug() << "operator + template is used\n";
        return (*this) + iFloat(v);
    }

    template <class dtype2>
    iFloat operator - (const dtype2& v) const {
        return (*this) - iFloat(v);
    }

    template <class dtype2>
    iFloat operator * (const dtype2& v) const {
        return (*this) * iFloat(v);
    }

    template <class dtype2>
    iFloat operator / (const dtype2& v) const {
        return (*this) / iFloat(v);
    }
};


//-------------------------------- SCALAR CALCULATION OPERATORS, opposite order
template<class dtype2>
iFloat operator + (const dtype2& v, const iFloat& num) {
    return num + v;
}

template<class dtype2>
iFloat operator - (const dtype2& v, const iFloat& num) {
    return num - v;
}

template<class dtype2>
iFloat operator * (const dtype2& v, const iFloat& num) {
    return num * v;
}

template<class dtype2>
iFloat operator / (const dtype2& v, const iFloat& num) {
    return num / v;
}

#endif // IFLOAT_H