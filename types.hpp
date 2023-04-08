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

#ifndef _TYPES_HPP
#define _TYPES_HPP

enum class path_flag : unsigned {
    south = 0x1, east = 0x2, north = 0x4, west = 0x8, all = 0xF
};
enum class uv_translation : unsigned int {
    rot_0, rot_90, rot_180, rot_270, flip_vert, flip_hori
};
enum class room_flag {
    none = 0,
    
    portal = 0x1,
    
    avoid = 0x2,
    
    important_1 = 0x4,
    important_2 = 0x8,
};

enum class point_id_t { };


struct quad_vertex {
    glm::vec3 position { 0 };
    glm::vec2 uv { 0 };
};

typedef unsigned int large_bool;

struct screen_info {
    [[maybe_unused]] glm::uvec2 size { 0 };
};
struct translation_info {
    [[maybe_unused]] glm::vec2 position { 0 };
    [[maybe_unused]] glm::uint scale = 0;
    [[maybe_unused]] large_bool enabled = false;
};

struct quad {
    glm::ivec2 position { 0 };
    glm::uvec2 size { 0 };
};

struct uv_quad {
    glm::vec2 position { 0 }, size { 0 };
};

struct rect {
    [[maybe_unused]] quad dimensions { { 0, 0 }, { 0, 0 } };
    [[maybe_unused]] uv_quad texture { { 0, 0 }, { 0, 0 } };
    [[maybe_unused]] uv_translation uv_tr = uv_translation::rot_0;
};

inline constexpr path_flag
operator|(path_flag left, path_flag right) {
    return (path_flag) ((unsigned) left | (unsigned) right);
}
inline constexpr room_flag
operator|(room_flag left, room_flag right) {
    return (room_flag) ((unsigned) left | (unsigned) right);
}

struct room_data {
    glm::ivec2 position { 0 };
    path_flag paths = path_flag::all;
    room_flag flags = room_flag::none;
    bool visited = false;
};

struct astar_point {
    glm::ivec2 position { 0, 0 };
    std::vector<glm::ivec2> parent_dir;
    unsigned path_length = 0;
    unsigned heuristic = 0;
    
    astar_point( ) noexcept = default;
    
    astar_point(const glm::ivec2& position, std::initializer_list<glm::ivec2> parent_dir, unsigned path_length, unsigned heuristic) :
            position(position), parent_dir(parent_dir), path_length(path_length), heuristic(heuristic) { }
};
struct queued_point {
    unsigned heuristic = 0;
    point_id_t point = (point_id_t) 0;
    
    queued_point( ) noexcept = default;
    
    queued_point(unsigned int heuristic, point_id_t point) :
            heuristic(heuristic), point(point) { }
};

[[nodiscard]] inline constexpr bool
operator<(const queued_point& left, const queued_point& right) {
    return left.heuristic > right.heuristic;
}

#endif //_TYPES_HPP
