#ifndef UNORDERED_SET_HPP
#define UNORDERED_SET_HPP

#include <set>
#include <list>
#include <algorithm>
#include <utility>

template<typename T>
class unordered_set
{
    private:
        std::list<T> order;   // garde l'ordre d'insertion
        std::set<T> lookup;   // garantit l'unicité

    public:
        typedef typename std::list<T>::iterator iterator;
        typedef typename std::list<T>::const_iterator const_iterator;

        std::pair<iterator, bool> insert(const T& value)
        {
            std::pair<typename std::set<T>::iterator, bool> result = lookup.insert(value);
            if (result.second)
            {
                order.push_back(value);
                return std::make_pair(--order.end(), true);
            }
            return std::make_pair(std::find(order.begin(), order.end(), value), false);
        }

        void erase(const T& value)
        {
            lookup.erase(value);
            order.remove(value);
        }

        void clear()
        {
            lookup.clear();
            order.clear();
        }

        bool count(const T& value) const
        {
            return lookup.count(value);
        }

        iterator find(const T& value)
        {
            typename std::set<T>::iterator it = lookup.find(value);
            if (it == lookup.end())
                return order.end();
            return std::find(order.begin(), order.end(), value);
        }

        const_iterator find(const T& value) const
        {
            typename std::set<T>::const_iterator it = lookup.find(value);
            if (it == lookup.end())
                return order.end();
            return std::find(order.begin(), order.end(), value);
        }

        bool empty() const { return order.empty(); }
        size_t size() const { return order.size(); }

        iterator begin()
        {
            return order.begin();
        }
        iterator end()
        {
            return order.end();
        }
        const_iterator begin() const
        {
            return order.begin();
        }
        const_iterator end() const
        {
            return order.end();
        }
};

#endif
