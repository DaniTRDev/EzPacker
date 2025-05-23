#include "Semantics/NormalTable.h"

NormalTable::NormalTable()
{
}

NormalTable::~NormalTable()
{
    m_table.clear();
}

bool NormalTable::addElement(const std::string &elem)
{
    if (doesElemExist(elem))
        return false;

    m_table.insert({elem, m_table.size() + 1});
    return true;
}

bool NormalTable::doesElemExist(const std::string &elem) const
{
    return m_table.contains(elem);
}

size_t NormalTable::getElemId(const std::string &elem) const
{
    if (doesElemExist(elem))
        return m_table.at(elem);

    return 0;
}

void NormalTable::clear()
{
    m_table.clear();
}

const std::unordered_map<std::string, size_t> &NormalTable::getTable() const
{
    return m_table;
}
