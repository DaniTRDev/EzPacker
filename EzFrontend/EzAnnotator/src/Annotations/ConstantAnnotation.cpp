#include "Annotations/ConstantAnnotation.h"

ConstantAnnotation::ConstantAnnotation(size_t typeId, float _float) :
    m_isFloat(true), m_isInteger(false), m_isString(false), m_float(_float)
{
    setTypeId(typeId);
}

ConstantAnnotation::ConstantAnnotation(size_t typeId, const std::string &str) :
    m_isFloat(false), m_isInteger(false), m_isString(true), m_string(str)
{
    setTypeId(typeId);
}

ConstantAnnotation::ConstantAnnotation(size_t typeId, const std::shared_ptr<mp_int> &integer) :
    m_isFloat(false), m_isInteger(true), m_isString(false), m_integer(integer)
{
    setTypeId(typeId);
}

ConstantAnnotation::~ConstantAnnotation()
{
    if (m_isInteger)
        mp_clear(m_integer.get());
}

bool ConstantAnnotation::isFloat() const { return m_isFloat; }

bool ConstantAnnotation::isInteger() const { return m_isInteger; }

bool ConstantAnnotation::isString() const { return m_isString; }

float ConstantAnnotation::getFloat() const { return m_float; }

const char *ConstantAnnotation::getAnnotationName() { return "Constant"; }

const std::shared_ptr<mp_int> &ConstantAnnotation::getInt() const { return m_integer; }

const std::string &ConstantAnnotation::getString() const { return m_string; }
