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

#ifndef ASPLITE_NAME_VALUE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_NAME_VALUE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include <map>
#include <string>
#include <vector>

class NameValueCollection {
public:
    typedef std::string key_type;
    typedef std::string value_type;
    typedef std::vector<key_type> key_list_type;
    typedef std::vector<value_type> value_list_type;

    size_t GetCount() const {
        return values_.size();
    }

    bool Add(const key_type &name, const value_type &value) {
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

    value_type Get(size_t index) const {
        const Pair &pair = values_[index];
        value_type str;
        for (value_list_type::const_iterator iter = pair.values.begin();
                iter != pair.values.end(); ++iter) {
            str += *iter;
            if (iter + 1 != pair.values.end())
                str += ',';
        }

        return str;
    }

    // TODO: Distinguish between not found and null.
    value_type Get(const key_type &name) const {
        key_map_type::const_iterator iter = keys_.find(name);
        if (iter != keys_.end())
            return Get(iter->second);
        else
            return value_type();
    }

    key_type GetKey(size_t index) const {
        return values_[index].key;
    }

    const value_list_type &GetValues(size_t index) const {
        return values_[index].values;
    }

    value_list_type GetValues(const key_type &name) const {
        key_map_type::const_iterator iter = keys_.find(name);
        if (iter != keys_.end())
            return GetValues(iter->second);
        else
            return value_list_type();
    }

    void Clear() {
        keys_.clear();
        values_.clear();
    }

    value_list_type AllKeys() const {
        value_list_type keys;
        for (key_map_type::const_iterator iter = keys_.begin();
                iter != keys_.end(); ++iter) {
            keys.push_back(iter->first);
        }
        return keys;
    }

    bool Set(const key_type &name, const value_type &value) {
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

    bool Remove(const key_type &name) {
        key_map_type::const_iterator iter = keys_.find(name);
        if (iter == keys_.end())
            return false;

        values_.erase(values_.begin() + iter->second);
        keys_.erase(iter);
        return true;
    }

private:
    struct Pair {
        Pair(const key_type &key, const value_type &value)
        : key(key) {
            values.push_back(value);
        }

        key_type key;
        value_list_type values;
    };

    typedef std::vector<Pair> pair_list_type;
    typedef std::map<key_type, typename pair_list_type::size_type> key_map_type;

    key_map_type keys_;
    pair_list_type values_;
};


// http://www.whatwg.org/specs/web-apps/current-work/
// multipage/association-of-controls-and-forms.html
// #application/x-www-form-urlencoded-encoding-algorithm

// TODO: Add CreateXWwwFormString
std::string CreateQueryString(const NameValueCollection &coll);


#endif // ASPLITE_NAME_VALUE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
