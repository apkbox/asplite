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

#ifndef ASPLITE_POSTED_FILE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
#define ASPLITE_POSTED_FILE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903

#include <string>


class HttpPostedFile {
public:
    HttpPostedFile() : content_length_(0) {}
    HttpPostedFile(const std::string &file_name, size_t content_length,
                   const std::string &content_type,
                   const std::string &internal_name)
        : file_name_(file_name), content_length_(content_length),
          content_type_(content_type), internal_name_(internal_name) { }

    size_t GetContentLength() const { return content_length_; }
    std::string GetContentType() const { return content_type_; }
    std::string GetFileName() const { return file_name_; }
    void SaveAs(const std::string &name) const;

private:
    size_t content_length_;
    std::string content_type_;
    std::string file_name_;
    std::string internal_name_;
};

#endif // ASPLITE_POSTED_FILE_H_562542B9_D0D5_4362_9B23_E9E1CABF9903
