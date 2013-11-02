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

#include "asplite/file_collection.h"


HttpFileCollection::key_list_type HttpFileCollection::AllKeys() const
{
    key_list_type keys;
    for (key_map_type::const_iterator iter = keys_.begin();
            iter != keys_.end(); ++iter) {
        keys.push_back(iter->first);
    }
    return keys;
}


const HttpFileCollection::value_type &HttpFileCollection::Get(size_t index) const
{
    const Pair &pair = values_[index];
    return pair.values[0];
}


const HttpFileCollection::value_type &HttpFileCollection::Get(
        const std::string &name) const
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter != keys_.end())
        return Get(iter->second);
    else
        return value_type();
}


const HttpFileCollection::value_list_type &HttpFileCollection::GetMultiple(
        const std::string &name) const
{
    key_map_type::const_iterator iter = keys_.find(name);
    if (iter != keys_.end())
        return values_[iter->second].values;
    else
        return value_list_type();
}


bool HttpFileCollection::Add(const HttpFileCollection::key_type &name,
                             const HttpFileCollection::value_type &value)
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


