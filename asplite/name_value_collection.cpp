// The MIT License (MIT)
//
//Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "asplite/name_value_collection.h"

#include <string>


bool NameValueCollection::Add(const key_type &name, const value_type &value)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end()) {
        keys_[name] = values_.size();
        value_list_type values;
        values.push_back(value);
        values_.push_back(pair_type(name, values));
        return true;
    }
    else {
        values_[iter->second].second.push_back(value);
        return false;
    }
}


bool NameValueCollection::Get(size_t index, value_type *value) const
{
    if (index >= values_.size())
        return false;

    const pair_type &pair = values_[index];

    for (value_list_type::const_iterator iter = pair.second.begin();
            iter != pair.second.end(); ++iter) {
        value->append(*iter);
        if (iter + 1 != pair.second.end())
            value->append(',', 1);
    }

    return true;
}


bool NameValueCollection::Get(const key_type &name, value_type *value) const
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter != keys_.end())
        return Get(iter->second, value);
    else
        return false;
}


bool NameValueCollection::GetKey(size_t index, key_type *key) const
{
    if (index >= values_.size())
        return false;

    *key = values_[index].first;
    return true;
}


bool NameValueCollection::GetValues(size_t index,
                                    const value_list_type **value) const
{
    if (index >= values_.size())
        return false;

    *value = &values_[index].second;
    return true;
}


bool NameValueCollection::GetValues(const key_type &name,
                                    const value_list_type **values) const
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end())
        return false;
    else
        return GetValues(iter->second, values);
}


NameValueCollection::value_list_type NameValueCollection::AllKeys() const
{
    value_list_type keys;
    for (key_map_type::const_iterator iter = keys_.begin();
            iter != keys_.end(); ++iter) {
        keys.push_back(iter->first);
    }
    return keys;
}


bool NameValueCollection::Set(const key_type &name, const value_type &value)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end()) {
        keys_[name] = values_.size();
        value_list_type values;
        values.push_back(value);
        values_.push_back(pair_type(name, values));
        return true;
    }
    else {
        values_[iter->second].second.clear();
        values_[iter->second].second.push_back(value);
        return false;
    }
}


bool NameValueCollection::Remove(const key_type &name)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end())
        return false;

    values_.erase(values_.begin() + iter->second);
    keys_.erase(iter);
    return true;
}


// TODO: Add CreateXWwwFormString
std::string CreateQueryString(const NameValueCollection &coll)
{
    std::string query_string;

    for (size_t i = 0; i < coll.GetCount(); i++) {
        std::string key;

        if (!coll.GetKey(i, &key))
            continue;

        const NameValueCollection::value_list_type *items;
        if (!coll.GetValues(i, &items))
            continue;

        // TODO: Not sure what to do with keys that do not have
        // values. According to HTML/4.01 successful control must provide
        // name/value pair.
        // Quote from HTML spec:
        //      "If a control doesn't have a current value when the form is submitted,
        //      user agents are not required to treat it as a successful control."
        if (items->size() > 0) {
            for (NameValueCollection::value_list_type::const_iterator iter =
                    items->begin(); iter != items->end(); ++iter) {
                // TODO: mitigate null keys/values
                // TODO: URL encode values.
                query_string += key;
                query_string += '=';
                query_string += *iter;
                if (iter != items->end() - 1)
                    query_string += '&';
            }

            if (i < coll.GetCount() - 1)
                query_string += '&';
        }
    }
    return query_string;
}
