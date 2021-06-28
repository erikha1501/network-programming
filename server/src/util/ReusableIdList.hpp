#pragma once

#include <stack>
#include <unordered_map>
#include <utility>

namespace util
{

template <typename TElement>
class ReusableIdList
{
public:
    using rid_t = uint;
    using iterator = typename std::unordered_map<rid_t, TElement>::iterator;
    using const_iterator = typename std::unordered_map<rid_t, TElement>::const_iterator;

    ReusableIdList(uint maxElementCount = -1) : m_maxElementCount(maxElementCount)
    {
    }

    bool isFull() const
    {
        return m_list.size() >= m_maxElementCount;
    }

    std::pair<iterator, bool> add(TElement item)
    {
        rid_t id = 0;

        // Exceed limit
        if (m_list.size() >= m_maxElementCount)
        {
            return std::make_pair(m_list.end(), false);
        }

        if (!m_freeIdList.empty())
        {
            id = m_freeIdList.top();
            m_freeIdList.pop();
        }
        else
        {
            id = m_list.size();
        }

        return m_list.emplace(id, std::move(item));
    }

    bool remove(rid_t id)
    {
        auto iter = m_list.find(id);
        if (iter == m_list.end())
        {
            return false;
        }

        m_freeIdList.push(iter->first);
        m_list.erase(iter);

        return true;
    }

    TElement* get(rid_t id)
    {
        auto iter = m_list.find(id);
        if (iter == m_list.end())
        {
            return nullptr;
        }

        return &iter->second;
    }
    const TElement* get(rid_t id) const
    {
        auto iter = m_list.find(id);
        if (iter == m_list.end())
        {
            return nullptr;
        }

        return &iter->second;
    }

    size_t size() const
    {
        return m_list.size();
    }

    iterator begin()
    {
        return m_list.begin();
    }
    iterator end()
    {
        return m_list.end();
    }
    const_iterator begin() const
    {
        return m_list.begin();
    }
    const_iterator end() const
    {
        return m_list.end();
    }

private:
    uint m_maxElementCount;

    std::unordered_map<rid_t, TElement> m_list;
    std::stack<rid_t> m_freeIdList;
};

} // namespace util