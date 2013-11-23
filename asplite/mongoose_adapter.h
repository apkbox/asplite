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

#ifndef ASPLITE_MONGOOSE_ADAPTER_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_MONGOOSE_ADAPTER_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include "asplite/asplite.h"
#include "mongoose/mongoose.h"

class AspliteMongooseAdapter;

class AspliteMongooseAdapter {
public:
  static int RequestHandler(struct mg_connection *conn);

  bool Init(const AspliteConfig &config);

private:
  AspliteConfig config_;
};

#endif  // ASPLITE_MONGOOSE_ADAPTER_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
