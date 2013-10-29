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

#include "asplite/asplite.h"

#include <string>
#include <vector>

bool NameValueCollection::Add(const std::string &name,
                              const std::string &value)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end()) {
        keys_[name] = values_.size();
        values_.push_back(Pair(name, value));
        return true;
    }
    else {
        values_[iter->second].values.push_back(value);
        return false;
    }
}


std::string NameValueCollection::Get(size_t index) const
{
    const Pair &pair = values_[index];
    std::string str;
    for (value_list_type::const_iterator iter = pair.values.begin();
            iter != pair.values.end(); ++iter) {
        str += *iter;
        if (iter + 1 != pair.values.end())
            str += ',';
    }

    return str;
}


// TODO: Distinguish between not found and null.
std::string NameValueCollection::Get(const std::string &name) const
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter != keys_.end())
        return Get(iter->second);
    else
        return std::string();
}


std::string NameValueCollection::GetKey(size_t index) const
{
    return values_[index].key;
}


NameValueCollection::value_list_type NameValueCollection::GetValues(
        size_t index) const
{
    return values_[index].values;
}


NameValueCollection::value_list_type NameValueCollection::GetValues(
        const std::string &name)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter != keys_.end())
        return GetValues(iter->second);
    else
        return NameValueCollection::value_list_type();
}


void NameValueCollection::Clear()
{
    keys_.clear();
    values_.clear();
}


NameValueCollection::value_list_type NameValueCollection::AllKeys() const
{
    NameValueCollection::value_list_type keys;
    for (key_map_type::const_iterator iter = keys_.begin();
            iter != keys_.end(); ++iter) {
        keys.push_back(iter->first);
    }
    return keys;
}


bool NameValueCollection::Set(const std::string &name,
                              const std::string &value)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end()) {
        keys_[name] = values_.size();
        values_.push_back(Pair(name, value));
        return true;
    }
    else {
        values_[iter->second].values.clear();
        values_[iter->second].values.push_back(value);
        return false;
    }
}


bool NameValueCollection::Remove(const std::string &name)
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter == keys_.end())
        return false;

    values_.erase(values_.begin() + iter->second);
    keys_.erase(iter);
    return true;
}
