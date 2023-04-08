// Copyright 2023 Jordan Paladino
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _BITMAP_HPP
#define _BITMAP_HPP

#include <filesystem>

#include <glm/glm.hpp>

#include "lodepng.h"

struct bitmap {
    typedef unsigned char byte;
    
    glm::uvec2 size { 0 };
    byte* bytes = nullptr;
    bool allocated = false;
    
    bitmap( ) noexcept = default;
    bitmap(const glm::uvec2& size, byte* bytes, bool allocated) :
        size(size), bytes(bytes), allocated(allocated) { }
    
    static inline std::vector<byte> __file_buffer;
    static inline std::vector<byte> __byte_buffer;
};

inline bitmap
load_image(const std::string& file) {
    if(!std::filesystem::exists(file)) throw std::runtime_error("File does not exist");
    unsigned error;
    
    bitmap::__file_buffer.clear( );
    error = lodepng::load_file(bitmap::__file_buffer, file);
    if(error) throw std::runtime_error("Unable to load file");
    
    bitmap::__byte_buffer.clear( );
    bitmap map { };
    error = lodepng::decode(bitmap::__byte_buffer, map.size.x, map.size.y, bitmap::__file_buffer);
    if(error) throw std::runtime_error("Unable to decode image file");
    
    map.bytes = new bitmap::byte[bitmap::__byte_buffer.size( )];
    std::move(std::begin(bitmap::__byte_buffer), std::end(bitmap::__byte_buffer), map.bytes);
    map.allocated = true;
    
    return map;
}

inline bitmap
load_image(const unsigned char* data, size_t size) {
    bitmap::__byte_buffer.clear( );
    bitmap map { };
    unsigned error = lodepng::decode(bitmap::__byte_buffer, map.size.x, map.size.y, data, size);
    if(error) throw std::runtime_error("Unable to decode image file");
    
    map.bytes = new bitmap::byte[bitmap::__byte_buffer.size( )];
    std::move(std::begin(bitmap::__byte_buffer), std::end(bitmap::__byte_buffer), map.bytes);
    map.allocated = true;
    
    return map;
}

inline void
delete_image(bitmap& map) {
    if(!map.allocated) return;
    
    map.size = { 0, 0 };
    map.allocated = false;
    delete[] map.bytes;
    map.bytes = nullptr;
}

#endif //_BITMAP_HPP
