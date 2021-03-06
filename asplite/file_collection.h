// The MIT License (MIT)
//
// Copyright (c) 2013 Alex Kozlov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ASPLITE_FILE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_FILE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include <map>
#include <string>
#include <vector>

#include "asplite/posted_file.h"

class HttpFileCollection {
public:
  typedef std::string key_type;
  typedef HttpPostedFile value_type;
  typedef std::vector<key_type> key_list_type;
  typedef std::vector<value_type> value_list_type;

  size_t GetCount() const { return values_.size(); }
  key_list_type AllKeys() const;

  bool Get(size_t index, value_type *value) const;
  bool Get(const key_type &name, value_type *value) const;
  bool GetKey(size_t index, key_type *key) const;

  bool GetMultiple(const key_type &name, const value_list_type **values) const;

  bool Add(const std::string &name, const value_type &value);

private:
  typedef std::pair<key_type, value_list_type> pair_type;
  typedef std::vector<pair_type> pair_list_type;
  typedef std::map<key_type, typename pair_list_type::size_type> key_map_type;

  key_map_type keys_;
  pair_list_type values_;
};

#endif  // ASPLITE_FILE_COLLECTION_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
