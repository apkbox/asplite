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


// TODO: Add CreateXWwwFormString
std::string CreateQueryString(const NameValueCollection &coll)
{
    std::string query_string;

    for (size_t i = 0; i < coll.GetCount(); i++) {
        std::string key = coll.GetKey(i);
        NameValueCollection::value_list_type items = coll.GetValues(i);
        // TODO: Not sure what to do with keys that do not have
        // values. According to HTML/4.01 successful control must provide
        // name/value pair.
        // Quote from HTML spec:
        //      "If a control doesn't have a current value when the form is submitted,
        //      user agents are not required to treat it as a successful control."
        if (items.size() > 0) {
            for (NameValueCollection::value_list_type::const_iterator iter =
                 items.begin(); iter != items.end(); ++iter) {
                // TODO: mitigate null keys/values
                // TODO: URL encode values.
                query_string += key;
                query_string += '=';
                query_string += *iter;
                if (iter != items.end() - 1)
                    query_string += '&';
            }

            if (i < coll.GetCount() - 1)
                query_string += '&';
        }
    }
    return query_string;
}


