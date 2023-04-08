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

#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <thread>
#include <fstream>
#include <queue>

#define NOMINMAX 1
#define WINVER 0x0601
#define NOMETAFILE 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include "glad/glad.h"
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#include "bitmap.hpp"
#include "types.hpp"

namespace resource {
#include "resource/background.h"
#include "resource/compass.h"
#include "resource/help.h"
#include "resource/icon_16.h"
#include "resource/icon_32.h"
#include "resource/icon_48.h"
#include "resource/icon_64.h"
#include "resource/shader_frag.h"
#include "resource/shader_vert.h"
#include "resource/texture.h"
}

#define OPENGL_SHADER_TESTS 1

struct __global_type {
    // Windows application instance
    HINSTANCE hInstance = nullptr;
    // Windows global low-level keyboard hook
    HHOOK hhook = nullptr;
    // Blocks the keys registered in 'global_keys' if true
    bool blocking_keys = true;
    // Enables global keys
    bool enable_global_keys = true;
    // If the program is running
    bool running = true;
    // If the window needs to be redrawn
    bool redraw = true;
    // If true discard the incoming update and render
    bool discard = false;
    // Global scale for the window
    unsigned global_scale = 1;
    // Queues an update
    bool queue_update = false;
    // If true display the help image
    bool show_help = false;
    
    struct __window {
        GLFWwindow* handle = nullptr;
        HWND hwnd = nullptr;
        glm::uvec2 size { 600, 600 };
        
        struct __icons {
            bitmap icon_64;
            bitmap icon_48;
            bitmap icon_32;
            bitmap icon_16;
        } icons;
    } window;
    
    struct __opengl {
        struct __shader {
            struct __buffers {
                GLuint quad_vertices_id = 0;
                GLuint quad_indices_id = 0;
                GLuint quad_instanced_pos_id = 0;
                
                GLuint screen_info_id = 0;
                GLuint translation_id = 0;
            } buffers;
            struct __arrays {
                GLuint quad_id = 0;
            } arrays;
            struct __rect_attribs {
                GLuint vertices_id = 0;
                GLuint uv_id = 0;
                // Instanced
                GLuint position_id = 0;
                GLuint size_id = 0;
                GLuint uv_position_id = 0;
                GLuint uv_size_id = 0;
                GLuint uv_tr_id = 0;
            } rect_attribs;
            struct __uniforms {
                GLint border_fade_id = 0;
            } uniforms;
            struct __uniform_buffers {
                const GLuint screen_info_index = 0;
                const GLuint translation_info_index = 1;
            } uniform_buffers;
            GLuint program = 0;
        } shader;
        struct __textures {
            GLuint texture_id = 0;
            GLuint background_id = 0;
            GLuint compass_id = 0;
            GLuint help_id = 0;
        } textures;
    } opengl;
    
    struct __map {
        // Current player position in the map
        glm::ivec2 position { 0, 0 };
        // Current view position
        glm::ivec2 view_position { 0, 0 };
        // The targeted view position
        glm::ivec2 target_view_position { 0, 0 };
        
        bool view_portal_room = false;
        bool show_scale_meter = false;
        
        glm::uint scale = 6;
        
        std::map<point_id_t, room_data> rooms;
        
        bool pick_direction = true;
        
        std::pair<glm::ivec2, path_flag> path[10] { { { 0, 0 }, (path_flag) 0 } };
        unsigned path_head = 0;
        unsigned path_size = 0;
        
        std::vector<glm::ivec2> portal_path;
    } map;
    
    std::set<DWORD> global_keys { VK_DOWN, VK_UP, VK_LEFT, VK_RIGHT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT };
} static global_state;

namespace constants {
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> north { 0, -1 };
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> south { 0, 1 };
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> west { -1, 0 };
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> east { 1, 0 };
    
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> zero { 0, 0 };
    template<typename _Tp>
    inline constexpr glm::vec<2, _Tp, glm::defaultp> one { 1, 1 };
    
    namespace window {
        constexpr unsigned icon_count = sizeof(__global_type::__window::__icons) / sizeof(bitmap);
    }
    
    namespace opengl {
        namespace shader {
            constexpr GLuint buffers_count = sizeof(__global_type::__opengl::__shader::__buffers) / sizeof(GLuint);
            constexpr GLuint arrays_count = sizeof(__global_type::__opengl::__shader::__arrays) / sizeof(GLuint);
            constexpr GLuint rect_attribs_count = sizeof(__global_type::__opengl::__shader::__rect_attribs) / sizeof(GLuint);
        }
        constexpr GLuint textures_count = sizeof(__global_type::__opengl::__textures) / sizeof(GLuint);
    }
    
    namespace map {
        template<typename _Tp>
        inline constexpr _Tp radius = 200;
        
        inline constexpr unsigned min_scale = 1;
        inline constexpr unsigned max_scale = 8;
        
        inline constexpr unsigned path_count = 10;
        
        inline constexpr unsigned room_area = 40;
    }
    
    quad_vertex const quad_vertices[] {
            { { 0, 0, 0 }, { 0, 0 } },
            { { 1, 0, 0 }, { 1, 0 } },
            { { 1, -1, 0 }, { 1, 1 } },
            { { 0, -1, 0 }, { 0, 1 } },
    };
    constexpr GLuint quad_indices[] {
            0, 1, 2, 0, 2, 3
    };
    inline constexpr GLuint quad_indices_count = sizeof(quad_indices) / sizeof(GLuint);
}

namespace __detail {
    //region OpenGL debug code translation add callback ...
    
    std::string
    gl_get_debug_source(unsigned int source) {
        switch(source) {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_APPLICATION: return "Application";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third Party";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window System";
        case GL_DEBUG_SOURCE_OTHER:
        default: return "Undefined";
        }
    }
    
    std::string
    gl_get_debug_type(unsigned int type) {
        switch(type) {
        case GL_DEBUG_TYPE_ERROR: return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
        case GL_DEBUG_TYPE_MARKER: return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "Push group";
        case GL_DEBUG_TYPE_POP_GROUP: return "Pop group";
        case GL_DEBUG_TYPE_OTHER:
        default: return "Undefined";
        }
    }
    
    std::string
    gl_get_debug_severity(unsigned int severity) {
        switch(severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "Info";
        case GL_DEBUG_SEVERITY_LOW: return "Low";
        case GL_DEBUG_SEVERITY_MEDIUM: return "Medium";
        case GL_DEBUG_SEVERITY_HIGH: return "High";
        default: return "Undefined";
        }
    }
    
    void
    gl_debug_callback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int, const char* message, const void*) {
        if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
        
        std::cout << gl_get_debug_source(source) << " [" << id << "] " << gl_get_debug_severity(severity) << " " << gl_get_debug_type(type) << " "
                << message << std::endl;
    }
    //endregion
    
    void
    glfw_error_callback(int type, const char* message) {
        std::cout << type << " " << message << std::endl;
    }
}

inline constexpr point_id_t
point_id(const glm::ivec2& p) {
    constexpr unsigned size = constants::map::radius<unsigned> * 2 + 1;
    constexpr unsigned offset = constants::map::radius<unsigned>;
    return (point_id_t) ((p.x + offset) + (p.y + offset) * size);
}

template<bool unbind = true>
void
assign_buffer(GLenum target, GLuint buffer, GLuint size, const void* data, GLenum usage = GL_STATIC_DRAW) {
    glBindBuffer(target, buffer);
    glBufferData(target, size, data, usage);
    if(unbind) glBindBuffer(target, 0);
}
GLuint
load_shader(const std::vector<std::string>& shader, GLenum shader_type) {
    GLuint shader_id = glCreateShader(shader_type);
    if(shader_id == 0) throw std::runtime_error("Unable to create shader");
    
    std::vector<const char*> shader_ptr;
    std::vector<GLsizei> shader_length;
    for(const auto& item: shader) {
        shader_ptr.push_back(item.data( ));
        shader_length.push_back((GLsizei) item.size( ));
    }
    glShaderSource(shader_id, (GLsizei) shader_ptr.size( ), shader_ptr.data( ), shader_length.data( ));
    glCompileShader(shader_id);
#if OPENGL_SHADER_TESTS
    GLint compiled;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
    if(compiled == GL_FALSE) {
        int length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length);
        glGetShaderInfoLog(shader_id, length, nullptr, message.data( ));
        glDeleteShader(shader_id);
        std::string str = std::string(message.begin( ), message.end( ));
        std::cerr << str << std::endl;
        throw std::runtime_error(str);
    }
#endif
    return shader_id;
}
GLuint
load_image(const bitmap& map, GLenum filter = GL_NEAREST, GLenum wrap = GL_CLAMP) {
    if(!map.allocated) return 0;
    GLuint id = 0;
    glGenTextures(1, &id);
    if(id == 0) throw std::runtime_error("Unable to allocated image on graphics pipeline");
    
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint) map.size.x, (GLint) map.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, map.bytes);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint) wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint) wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint) filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint) filter);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

struct attrib_builder {
    GLuint gl_next_attribute;
    GLuint* buffer_ids;
    GLuint buffer_ids_size;
    
    attrib_builder(GLuint* ids, GLuint size) :
            gl_next_attribute(0), buffer_ids(ids), buffer_ids_size(size) { }
    
    void
    attribute(GLint size, GLenum type, GLint stride, GLsizeiptr offset, bool instanced = false) {
        if(gl_next_attribute >= buffer_ids_size) throw std::runtime_error("Assigned outside of bounds");
        
        glEnableVertexAttribArray(gl_next_attribute);
        glVertexAttribPointer(gl_next_attribute, size, type, false, stride, (void*) offset);
        if(instanced) glVertexAttribDivisor(gl_next_attribute, 1);
        *(buffer_ids + gl_next_attribute) = gl_next_attribute;
        gl_next_attribute++;
    }
    
    void
    attribute_i(GLint size, GLenum type, GLint stride, GLsizeiptr offset, bool instanced = false) {
        if(gl_next_attribute >= buffer_ids_size) throw std::runtime_error("Assigned outside of bounds");
        
        glEnableVertexAttribArray(gl_next_attribute);
        glVertexAttribIPointer(gl_next_attribute, size, type, stride, (void*) offset);
        if(instanced) glVertexAttribDivisor(gl_next_attribute, 1);
        *(buffer_ids + gl_next_attribute) = gl_next_attribute;
        gl_next_attribute++;
    }
    
    void
    attribute_l(GLint size, GLenum type, GLint stride, GLsizeiptr offset, bool instanced = false) {
        if(gl_next_attribute >= buffer_ids_size) throw std::runtime_error("Assigned outside of bounds");
        
        glEnableVertexAttribArray(gl_next_attribute);
        glVertexAttribLPointer(gl_next_attribute, size, type, stride, (void*) offset);
        if(instanced) glVertexAttribDivisor(gl_next_attribute, 1);
        *(buffer_ids + gl_next_attribute) = gl_next_attribute;
        gl_next_attribute++;
    }
    
};


GLFWwindow*
create_window( ) {
    glfwDefaultWindowHints( );
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor( );
    GLFWwindow* window_ref = glfwCreateWindow((int) global_state.window.size.x, (int) global_state.window.size.y, "Vault Mapper", nullptr, nullptr);
    if(window_ref == nullptr) throw std::runtime_error("Failed to create window.");
    glfwSetWindowAttrib(window_ref, GLFW_AUTO_ICONIFY, false);
    glfwSetWindowAttrib(window_ref, GLFW_RESIZABLE, false);
    glfwSetWindowAttrib(window_ref, GLFW_DECORATED, true);
    glfwSetWindowAttrib(window_ref, GLFW_FLOATING, false);
    
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    glfwSetWindowPos(window_ref, (int) (videoMode->width - global_state.window.size.x) / 2, (int) (videoMode->height - global_state.window.size.y) / 2);
    return window_ref;
}
void
window_icons( ) {
    global_state.window.icons.icon_64 = load_image(resource::icon_64, sizeof(resource::icon_64));
    global_state.window.icons.icon_48 = load_image(resource::icon_48, sizeof(resource::icon_48));
    global_state.window.icons.icon_32 = load_image(resource::icon_32, sizeof(resource::icon_32));
    global_state.window.icons.icon_16 = load_image(resource::icon_16, sizeof(resource::icon_16));
    
    GLFWimage icons[] {
            { (int) global_state.window.icons.icon_64.size.x, (int) global_state.window.icons.icon_64.size.y, global_state.window.icons.icon_64.bytes },
            { (int) global_state.window.icons.icon_48.size.x, (int) global_state.window.icons.icon_48.size.y, global_state.window.icons.icon_48.bytes },
            { (int) global_state.window.icons.icon_32.size.x, (int) global_state.window.icons.icon_32.size.y, global_state.window.icons.icon_32.bytes },
            { (int) global_state.window.icons.icon_16.size.x, (int) global_state.window.icons.icon_16.size.y, global_state.window.icons.icon_16.bytes }
    };
    
    glfwSetWindowIcon(global_state.window.handle, (int) constants::window::icon_count, (GLFWimage*) &icons);
}
void
build_gl_images( ) {
    bitmap texture_bitmap = load_image(resource::texture, sizeof(resource::texture));
    global_state.opengl.textures.texture_id = load_image(texture_bitmap);
    delete_image(texture_bitmap);
    
    bitmap background_bitmap = load_image(resource::background, sizeof(resource::background));
    global_state.opengl.textures.background_id = load_image(background_bitmap, GL_LINEAR, GL_REPEAT);
    delete_image(background_bitmap);
    
    bitmap compass_bitmap = load_image(resource::compass, sizeof(resource::compass));
    global_state.opengl.textures.compass_id = load_image(compass_bitmap);
    delete_image(compass_bitmap);
    
    bitmap help_bitmap = load_image(resource::help, sizeof(resource::help));
    global_state.opengl.textures.help_id = load_image(help_bitmap);
    delete_image(help_bitmap);
}
void
build_gl_items( ) {
    glGenBuffers((GLsizei) constants::opengl::shader::buffers_count, (GLuint*) &global_state.opengl.shader.buffers);
    glGenVertexArrays((GLsizei) constants::opengl::shader::arrays_count, (GLuint*) &global_state.opengl.shader.arrays);
    
    glBindVertexArray(0);
    assign_buffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_vertices_id, sizeof(constants::quad_vertices), &constants::quad_vertices);
    assign_buffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_indices_id, sizeof(constants::quad_indices), &constants::quad_indices);
    assign_buffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_instanced_pos_id, sizeof(rect), nullptr, GL_DYNAMIC_DRAW);
    
    /**
     * @code
     * Attrib parts -------------------- Buf - Attributes
     * ==================================================
     * vertex, uv-------------------------------- [0] - 0 1
     * indices ---------------------------------- [1]
     * position, size, uv_position, uv_size - i - [2] - 2 3 4 5
     * @endcode
     */
    glBindVertexArray(global_state.opengl.shader.arrays.quad_id);
    attrib_builder quad_builder((GLuint*) &global_state.opengl.shader.rect_attribs, constants::opengl::shader::rect_attribs_count);
    glBindBuffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_vertices_id);
    quad_builder.attribute(3, GL_FLOAT, sizeof(quad_vertex), offsetof(quad_vertex, position));
    quad_builder.attribute(2, GL_FLOAT, sizeof(quad_vertex), offsetof(quad_vertex, uv));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_indices_id);
    // Instanced buffer
    glBindBuffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_instanced_pos_id);
    quad_builder.attribute_i(2, GL_INT, sizeof(rect), offsetof(rect, dimensions.position), true);
    quad_builder.attribute_i(2, GL_UNSIGNED_INT, sizeof(rect), offsetof(rect, dimensions.size), true);
    quad_builder.attribute(2, GL_FLOAT, sizeof(rect), offsetof(rect, texture.position), true);
    quad_builder.attribute(2, GL_FLOAT, sizeof(rect), offsetof(rect, texture.size), true);
    quad_builder.attribute_i(1, GL_UNSIGNED_INT, sizeof(rect), offsetof(rect, uv_tr), true);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    assign_buffer<false>(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.screen_info_id, sizeof(screen_info), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, global_state.opengl.shader.uniform_buffers.screen_info_index, global_state.opengl.shader.buffers.screen_info_id, 0,
            sizeof(screen_info));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    assign_buffer<false>(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.translation_id, sizeof(translation_info), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferRange(GL_UNIFORM_BUFFER, global_state.opengl.shader.uniform_buffers.translation_info_index, global_state.opengl.shader.buffers.translation_id, 0,
            sizeof(translation_info));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    global_state.opengl.shader.program = glCreateProgram( );
    
    //std::stringstream stream;
    //std::ifstream file_stream("res/shader_vert.glsl");
    //if(!file_stream.is_open( )) throw std::runtime_error("Unable to load fragment shader file");
    //stream << file_stream.rdbuf( );
    //file_stream.close( );
    
    GLuint vert_id = load_shader({ resource::shader_vert }, GL_VERTEX_SHADER);
    glAttachShader(global_state.opengl.shader.program, vert_id);
    
    //stream.str({ });
    //file_stream.open("res/shader_frag.glsl");
    //if(!file_stream.is_open( )) throw std::runtime_error("unable to load vertex shader file");
    //stream << file_stream.rdbuf( );
    //file_stream.close( );
    
    GLuint frag_id = load_shader({ resource::shader_frag }, GL_FRAGMENT_SHADER);
    glAttachShader(global_state.opengl.shader.program, frag_id);
    
    glBindAttribLocation(global_state.opengl.shader.program, global_state.opengl.shader.rect_attribs.vertices_id, "vertex");
    glBindAttribLocation(global_state.opengl.shader.program, global_state.opengl.shader.rect_attribs.uv_id, "uv");
    
    glLinkProgram(global_state.opengl.shader.program);
#if OPENGL_SHADER_TESTS
    GLint linked = GL_FALSE;
    glGetProgramiv(global_state.opengl.shader.program, GL_LINK_STATUS, &linked);
    if(linked == GL_FALSE) {
        int length = 0;
        glGetProgramiv(global_state.opengl.shader.program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length);
        glGetProgramInfoLog(global_state.opengl.shader.program, length, nullptr, message.data( ));
        
        glDeleteShader(vert_id);
        glDeleteShader(frag_id);
        
        throw std::runtime_error(message.data( ));
    }
#endif
    
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    glValidateProgram(global_state.opengl.shader.program);
#if OPENGL_SHADER_TESTS
    glValidateProgram(global_state.opengl.shader.program);
    int valid = GL_FALSE;
    glGetProgramiv(global_state.opengl.shader.program, GL_VALIDATE_STATUS, &valid);
    if(valid == GL_FALSE) {
        int length = 0;
        glGetProgramiv(global_state.opengl.shader.program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> message(length);
        glGetProgramInfoLog(global_state.opengl.shader.program, length, nullptr, message.data( ));
        
        throw std::runtime_error(message.data( ));
    }
#endif
    glUseProgram(0);
    
    GLint uniform_image = glGetUniformLocation(global_state.opengl.shader.program, "image");
    glProgramUniform1i(global_state.opengl.shader.program, uniform_image, 0);
    
    global_state.opengl.shader.uniforms.border_fade_id = glGetUniformLocation(global_state.opengl.shader.program, "border_fade");
    
    GLint uniform_screen_info = glGetUniformBlockIndex(global_state.opengl.shader.program, "screen_info");
    glUniformBlockBinding(global_state.opengl.shader.program, uniform_screen_info, global_state.opengl.shader.uniform_buffers.screen_info_index);
    GLint uniform_translation_info = glGetUniformBlockIndex(global_state.opengl.shader.program, "translation_info");
    glUniformBlockBinding(global_state.opengl.shader.program, uniform_translation_info, global_state.opengl.shader.uniform_buffers.translation_info_index);
    
    build_gl_images( );
}

void
update( );
void
render( );

void
update_translation_position( ) {
    glm::vec2 position = global_state.map.view_position + (glm::ivec2) (global_state.window.size / 2u);
    glBindBuffer(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.translation_id);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec2), &position);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void
update_translation_scale(unsigned scale) {
    scale = global_state.global_scale * scale;
    glBindBuffer(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.translation_id);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec2), sizeof(glm::uint), &scale);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void
enable_translation(large_bool enabled) {
    glBindBuffer(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.translation_id);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec2) + sizeof(glm::uint), sizeof(large_bool), &enabled);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

#define DISCARD { global_state.discard = true; return; }

LRESULT CALLBACK
WindowGlobalKeyboard(int code, WPARAM wParam, LPARAM lParam);
void
keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void
init_map(path_flag paths);
void
reset_map( );
void
add_surrounding_rooms(glm::ivec2 pos) {
    path_flag paths = global_state.map.rooms[point_id(pos)].paths;
    
    for(int i = 0; i < 8; i++) {
        int index = i + (i > 3);
        int x = index % 3 - 1;
        int y = index / 3 - 1;
        
        bool corner = x && y;
        bool left = x == -1;
        bool right = x == 1;
        bool down = y == 1;
        bool up = y == -1;
        
        glm::ivec2 around { pos.x + x, pos.y + y };
        if(std::abs(around.x) > constants::map::radius<unsigned> || std::abs(around.y) > constants::map::radius<unsigned>) continue;
        
        bool edge_up = std::abs(around.y) == constants::map::radius<unsigned> && around.y < 0;
        bool edge_left = std::abs(around.x) == constants::map::radius<unsigned> && around.x < 0;
        bool edge_down = std::abs(around.y) == constants::map::radius<unsigned> && around.y > 0;
        bool edge_right = std::abs(around.x) == constants::map::radius<unsigned> && around.x > 0;
        
        auto p = (unsigned) paths;
        unsigned open = (corner ? ((!edge_down << 0) | (!edge_right << 1) | (!edge_up << 2) | (!edge_left << 3)) : (
                ((((p & 0x4) || !up) && !edge_down) << 0)
                        | ((((p & 0x8) || !left) && !edge_right) << 1)
                        | ((((p & 0x1) || !down) && !edge_up) << 2)
                        | ((((p & 0x2) || !right) && !edge_left) << 3)
        ));
        
        auto found = global_state.map.rooms.find(point_id(around));
        if(found != std::end(global_state.map.rooms)) {
            if(!corner) found->second.paths = (path_flag) ((unsigned) found->second.paths & open);
        } else {
            global_state.map.rooms[point_id(around)] = { around, (path_flag) open, room_flag::none, false };
        }
    }
}
void
add_room(glm::ivec2 pos, path_flag paths, room_flag flags, bool visited = true) {
    if(global_state.map.rooms.find(point_id(pos)) != std::end(global_state.map.rooms)) return;
    global_state.map.rooms[point_id(pos)] = room_data { pos, paths, flags, visited };
}
void
push_path(const glm::ivec2& point, path_flag direction) {
    unsigned position = global_state.map.path_size;
    if(global_state.map.path_size == 10) {
        position = global_state.map.path_head;
        if(global_state.map.path_head == 9) global_state.map.path_head = 0;
        else global_state.map.path_head++;
    } else global_state.map.path_size++;
    global_state.map.path[position] = { point, direction };
}
unsigned
calc_heuristic(glm::ivec2 p) {
    //auto dif = global_state.map.position - p;
    return p.x * p.x + p.y * p.y /*+ dif.x * dif.x + dif.y * dif.y*/;
}
void
find_path( ) {
    static constexpr glm::ivec2 directions[] { constants::south<int>, constants::east<int>, constants::north<int>, constants::west<int> };
    
    global_state.map.portal_path.clear( );
    if(global_state.map.position == constants::zero<int>) return;
    std::map<point_id_t, astar_point> point_data;
    std::priority_queue<queued_point> points;
    
    point_id_t start_id = point_id(global_state.map.position);
    point_id_t end_id = point_id(constants::zero<int>);
    
    unsigned start_heuristic = calc_heuristic(global_state.map.position);
    point_data[start_id] = { global_state.map.position, { }, 0, start_heuristic };
    points.emplace(start_heuristic, start_id);
    
    bool found_portal = false;
    unsigned max_length = 64;
    while(!points.empty( )) {
        auto pi = points.top( ).point;
        auto p = point_data[pi];
        points.pop( );
        
        auto found = global_state.map.rooms.find(pi);
        path_flag paths = path_flag::all;
        if(found != std::end(global_state.map.rooms))
            paths = found->second.paths;
        
        if(p.path_length + 1 > max_length) continue;
        for(int i = 0; i < 4; i++) {
            if(!((unsigned) paths & (1 << i))) continue;
            
            auto dir = directions[i];
            auto n = p.position + dir;
            auto ni = point_id(n);
            
            if(std::abs(n.x) > constants::map::radius<unsigned> || std::abs(n.y) > constants::map::radius<unsigned>) continue;
            
            auto found_data = point_data.find(ni);
            if(found_data != std::end(point_data)) {
                if(p.path_length < found_data->second.path_length)
                    found_data->second.parent_dir.emplace_back(-dir);
                continue;
            }
            float scale = 1;
            found = global_state.map.rooms.find(ni);
            if(found != std::end(global_state.map.rooms)) {
                if((unsigned) found->second.flags & (unsigned) room_flag::avoid)
                    scale = 5;
                else if(!found->second.visited)
                    scale = 1.5f;
            } else scale = 3;
            
            auto point_heuristic = (unsigned) ((float) calc_heuristic(n) * scale);
            point_data[ni] = { n, { -dir }, p.path_length + 1, point_heuristic };
            points.emplace(point_heuristic, ni);
            
            if((found_portal = ni == end_id))
                max_length = p.path_length + 1;
            
            //if((found_portal = ni == end_id)) break;
        }
        //if(found_portal) break;
    }
    
    auto found = point_data.find(end_id);
    if(found != std::end(point_data)) {
        auto pos = constants::zero<int>;
        auto dest = found->second;
        while(dest.path_length != 0) {
            auto next = dest.parent_dir.front( );
            for(const auto& item: dest.parent_dir) {
                auto point_test = point_data.at(point_id(pos + item));
                auto point_next = point_data.at(point_id(pos + next));
                
                //if(point_test.heuristic < point_next.heuristic)
                //    next = item;
                //else if(point_test.heuristic == point_next.heuristic)
                if(point_test.path_length < point_next.path_length)
                    next = item;
            }
            
            global_state.map.portal_path.push_back(next);
            pos += next;
            dest = point_data.at(point_id(pos));
        }
    }
}


int
main( ) {
    if(!glfwInit( ))
        throw std::runtime_error("Unable to initialize GLFW");
    glfwSetErrorCallback(__detail::glfw_error_callback);
    
    global_state.hInstance = GetModuleHandle(nullptr);
    global_state.hhook = SetWindowsHookEx(WH_KEYBOARD_LL, WindowGlobalKeyboard, nullptr, 0);
    
    global_state.window.handle = create_window( );
    global_state.window.hwnd = glfwGetWin32Window(global_state.window.handle);
    window_icons( );
    
    glfwMakeContextCurrent(global_state.window.handle);
    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
        throw std::runtime_error("Unable to load OpenGL context");
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(__detail::gl_debug_callback, nullptr);
    
    glfwSetKeyCallback(global_state.window.handle, keyboard_callback);
    build_gl_items( );
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(global_state.opengl.shader.arrays.quad_id);
    glUseProgram(global_state.opengl.shader.program);
    
    screen_info screen { global_state.window.size };
    glBindBuffer(GL_UNIFORM_BUFFER, global_state.opengl.shader.buffers.screen_info_id);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(screen_info), &screen);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    update_translation_position( );
    update_translation_scale(global_state.map.scale);
    enable_translation(false);
    
    glfwShowWindow(global_state.window.handle);
    while(global_state.running) {
        glfwWaitEvents( );
        if(glfwWindowShouldClose(global_state.window.handle))
            break;
        
        if(global_state.discard && !global_state.queue_update) continue;
        global_state.queue_update = false;
        //std::cout << "Update" << std::endl;
        update( );
        global_state.discard = true;
        if(global_state.queue_update) glfwPostEmptyEvent( );
        if(!global_state.redraw) continue;
    
        //std::cout << "Render" << std::endl;
        render( );
        glfwSwapBuffers(global_state.window.handle);
        global_state.redraw = false;
    }
    global_state.running = false;
    
    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glDeleteProgram(global_state.opengl.shader.program);
    glDeleteVertexArrays((GLsizei) constants::opengl::shader::arrays_count, (GLuint*) &global_state.opengl.shader.arrays);
    glDeleteBuffers((GLsizei) constants::opengl::shader::buffers_count, (GLuint*) &global_state.opengl.shader.buffers);
    glDeleteTextures((GLsizei) constants::opengl::textures_count, (GLuint*) &global_state.opengl.textures);
    
    glfwTerminate( );
    for(int i = 0; i < constants::window::icon_count; i++) {
        auto ptr = ((bitmap*) &global_state.window.icons) + i;
        delete_image(*ptr);
    }
    
    UnhookWindowsHookEx(global_state.hhook);
    return EXIT_SUCCESS;
}

LRESULT CALLBACK
WindowGlobalKeyboard(int code, WPARAM wParam, LPARAM lParam) {
    if(code != HC_ACTION) return 0;
    if(!global_state.enable_global_keys) return 0;
    if(global_state.window.hwnd == GetFocus( )) return 0;
    
    switch((DWORD) wParam) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        auto param = (PKBDLLHOOKSTRUCT) lParam;
        bool down = (wParam == WM_KEYDOWN) || (wParam == WM_SYSKEYDOWN);
        if(global_state.global_keys.find(param->vkCode) != std::end(global_state.global_keys)) {
            ULONG l = 1 & 0xFFFF                        // Repeat count
                    | (param->scanCode & 0x1FF) << 16   // Scancode
                    | (param->flags & 1) << 24          // Extended
                    | ((param->flags >> 5) & 1) << 29   // Context
                    | (!down) << 30                     // Previous state
                    | ((param->flags >> 7) & 1) << 31;  // Transition
            SendMessage(global_state.window.hwnd, down ? WM_KEYDOWN : WM_KEYUP, (WPARAM) param->vkCode, (LPARAM) l);
            if(global_state.blocking_keys) return 1;
        }
        
        break;
    }
    default: break;
    }
    
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
void
keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    global_state.discard = false;
    
    static room_data dummy_room { constants::zero<int>, (path_flag) 0, room_flag::none, false };
    room_data* room = &dummy_room;
    if(!global_state.map.pick_direction)
        room = &global_state.map.rooms.at(point_id(global_state.map.position));
    
    switch(key) {
    case GLFW_KEY_UP: {
        if(global_state.show_help) DISCARD
        if(global_state.map.pick_direction) {
            if(mods & GLFW_MOD_ALT) DISCARD
            if(action == GLFW_RELEASE) return;
            init_map(path_flag::north);
            global_state.redraw = true;
            break;
        }
        
        if(global_state.map.view_portal_room) DISCARD
        if(global_state.map.position.y - 1 < -constants::map::radius<int>) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(room->flags == room_flag::portal) DISCARD
            room_data* facing_room = &global_state.map.rooms[point_id(global_state.map.position + constants::north<int>)];
            if(facing_room->flags == room_flag::portal) DISCARD
            if(action == GLFW_RELEASE) return;
            
            (unsigned&) (room->paths) ^= (unsigned) path_flag::north;
            (unsigned&) (facing_room->paths) ^= (unsigned) path_flag::south;
            find_path( );
            global_state.redraw = true;
            return;
        }
        
        if(!((unsigned) room->paths & (unsigned) path_flag::north)) DISCARD
        if(action == GLFW_RELEASE) return;
        
        global_state.map.position.y--;
        push_path(global_state.map.position, path_flag::south);
        find_path( );
        global_state.map.rooms.at(point_id(global_state.map.position)).visited = true;
        add_surrounding_rooms(global_state.map.position);
        global_state.map.target_view_position.y = global_state.map.position.y * -40;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_DOWN: {
        if(global_state.show_help) DISCARD
        if(global_state.map.pick_direction) {
            if(mods & GLFW_MOD_ALT) DISCARD
            if(action == GLFW_RELEASE) return;
            init_map(path_flag::south);
            global_state.redraw = true;
            break;
        }
        
        if(global_state.map.view_portal_room) DISCARD
        if(global_state.map.position.y + 1 > constants::map::radius<int>) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(room->flags == room_flag::portal) DISCARD
            room_data* facing_room = &global_state.map.rooms[point_id(global_state.map.position + constants::south<int>)];
            if(facing_room->flags == room_flag::portal) DISCARD
            if(action == GLFW_RELEASE) return;
            
            (unsigned&) (room->paths) ^= (unsigned) path_flag::south;
            (unsigned&) (facing_room->paths) ^= (unsigned) path_flag::north;
            find_path( );
            global_state.redraw = true;
            return;
        }
        
        if(!((unsigned) room->paths & (unsigned) (path_flag::south))) DISCARD
        if(action == GLFW_RELEASE) return;
        
        global_state.map.position.y++;
        push_path(global_state.map.position, path_flag::north);
        find_path( );
        global_state.map.rooms.at(point_id(global_state.map.position)).visited = true;
        add_surrounding_rooms(global_state.map.position);
        global_state.map.target_view_position.y = global_state.map.position.y * -40;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_LEFT: {
        if(global_state.show_help) DISCARD
        if(global_state.map.pick_direction) {
            if(mods & GLFW_MOD_ALT) DISCARD
            if(action == GLFW_RELEASE) return;
            init_map(path_flag::west);
            global_state.redraw = true;
            break;
        }
        
        if(global_state.map.view_portal_room) DISCARD
        if(global_state.map.position.x - 1 < -constants::map::radius<int>) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(room->flags == room_flag::portal) DISCARD
            room_data* facing_room = &global_state.map.rooms[point_id(global_state.map.position + constants::west<int>)];
            if(facing_room->flags == room_flag::portal) DISCARD
            if(action == GLFW_RELEASE) return;
            
            (unsigned&) (room->paths) ^= (unsigned) path_flag::west;
            (unsigned&) (facing_room->paths) ^= (unsigned) path_flag::east;
            find_path( );
            global_state.redraw = true;
            return;
        }
        
        if(!((unsigned) room->paths & (unsigned) path_flag::west)) DISCARD
        if(action == GLFW_RELEASE) return;
        
        global_state.map.position.x--;
        push_path(global_state.map.position, path_flag::east);
        find_path( );
        global_state.map.rooms.at(point_id(global_state.map.position)).visited = true;
        add_surrounding_rooms(global_state.map.position);
        global_state.map.target_view_position.x = global_state.map.position.x * -40;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_RIGHT: {
        if(global_state.show_help) DISCARD
        if(global_state.map.pick_direction) {
            if(mods & GLFW_MOD_ALT) DISCARD
            if(action == GLFW_RELEASE) return;
            init_map(path_flag::east);
            global_state.redraw = true;
            return;
        }
        
        if(global_state.map.view_portal_room) DISCARD
        if(global_state.map.position.x + 1 > constants::map::radius<int>) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(room->flags == room_flag::portal) DISCARD
            room_data* facing_room = &global_state.map.rooms[point_id(global_state.map.position + constants::east<int>)];
            if(facing_room->flags == room_flag::portal) DISCARD
            if(action == GLFW_RELEASE) return;
            
            (unsigned&) (room->paths) ^= (unsigned) path_flag::east;
            (unsigned&) (facing_room->paths) ^= (unsigned) path_flag::west;
            find_path( );
            global_state.redraw = true;
            return;
        }
        
        if(!((unsigned) room->paths & (unsigned) path_flag::east)) DISCARD
        if(action == GLFW_RELEASE) return;
        
        global_state.map.position.x++;
        push_path(global_state.map.position, path_flag::west);
        find_path( );
        global_state.map.rooms.at(point_id(global_state.map.position)).visited = true;
        add_surrounding_rooms(global_state.map.position);
        global_state.map.target_view_position.x = global_state.map.position.x * -40;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_PAGE_DOWN: {
        if(global_state.show_help) DISCARD
        if(global_state.map.view_portal_room) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(global_state.map.scale <= constants::map::min_scale) DISCARD
            if(action == GLFW_RELEASE) return;
            global_state.map.scale = std::max(global_state.map.scale - 1, constants::map::min_scale);
            update_translation_scale(global_state.map.scale);
            global_state.map.show_scale_meter = true;
            global_state.redraw = true;
            return;
        }
        
        if(global_state.map.pick_direction) DISCARD
        if(room->flags == room_flag::portal) DISCARD
        if(action == GLFW_RELEASE) return;
        room->flags = room->flags == room_flag::important_1 ? room_flag::none : room_flag::important_1;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_PAGE_UP: {
        if(global_state.show_help) DISCARD
        if(global_state.map.view_portal_room) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(global_state.map.scale >= constants::map::max_scale) DISCARD
            if(action == GLFW_RELEASE) return;
            global_state.map.scale = std::min(global_state.map.scale + 1, constants::map::max_scale);
            update_translation_scale(global_state.map.scale);
            global_state.map.show_scale_meter = true;
            global_state.redraw = true;
            return;
        }
        
        if(global_state.map.pick_direction) DISCARD
        if(room->flags == room_flag::portal) DISCARD
        if(action == GLFW_RELEASE) return;
        room->flags = room->flags == room_flag::important_2 ? room_flag::none : room_flag::important_2;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_HOME: {
        if(global_state.show_help) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(action == GLFW_RELEASE) return;
            if(action != GLFW_PRESS) DISCARD
            
            global_state.enable_global_keys = !global_state.enable_global_keys;
            
            ULONG l = 1 & 0xFFFF | (71) << 16 | (1) << 24 | (1) << 29 | (1) << 30 | (1) << 31;
            SendMessage(global_state.window.hwnd, WM_KEYUP, (WPARAM) VK_HOME, (LPARAM) l);
            global_state.redraw = true;
            return;
        }
        
        if(global_state.map.pick_direction) DISCARD
        if(action == GLFW_REPEAT) DISCARD
        if(action == GLFW_PRESS) {
            global_state.map.target_view_position = constants::zero<int>;
            
            update_translation_position( );
            update_translation_scale(1);
            global_state.map.view_portal_room = true;
            global_state.redraw = true;
            return;
        }
        
        global_state.map.target_view_position = global_state.map.position * -40;
        
        update_translation_position( );
        update_translation_scale(global_state.map.scale);
        global_state.map.view_portal_room = false;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_END: {
        if(global_state.show_help) DISCARD
        if(global_state.map.view_portal_room) DISCARD
        if(global_state.map.pick_direction) DISCARD
        
        if(mods & GLFW_MOD_ALT) {
            if(action != GLFW_PRESS) DISCARD
            reset_map( );
            global_state.redraw = true;
            return;
        }
        
        if(room->flags == room_flag::portal) DISCARD
        if(action == GLFW_RELEASE) return;
        room->flags = room->flags == room_flag::avoid ? room_flag::none : room_flag::avoid;
        global_state.redraw = true;
        break;
    }
    
    case GLFW_KEY_F1: {
        if(action == GLFW_REPEAT) DISCARD
        if(global_state.map.view_portal_room) DISCARD
        if(!global_state.show_help && action == GLFW_RELEASE) DISCARD
        
        global_state.show_help = !global_state.show_help;
        global_state.redraw = true;
        break;
    }
    case GLFW_KEY_ESCAPE: {
        global_state.running = false;
        break;
    }
    default: DISCARD
    }
}

void
init_map(path_flag paths) {
    add_room({ 0, 0 }, paths, room_flag::portal);
    add_surrounding_rooms({ 0, 0 });
    global_state.map.pick_direction = false;
}
void
reset_map( ) {
    global_state.map.position = { 0, 0 };
    global_state.map.view_position = { 0, 0 };
    global_state.map.target_view_position = { 0, 0 };
    update_translation_position( );
    
    global_state.map.view_portal_room = false;
    
    global_state.map.rooms.clear( );
    
    global_state.map.pick_direction = true;
    
    global_state.map.path_head = 0;
    global_state.map.path_size = 0;
    
    global_state.map.portal_path.clear( );
}

namespace textures {
    inline constexpr uv_quad all { { 0, 0 }, { 1, 1 } };
    
    inline constexpr uv_quad player_dot { { 0.625f, 0.75f }, { 0.125f, 0.125f } };
    inline constexpr uv_quad portal { { 0.625f, 0.875f }, { 0.125f, 0.125f } };
    
    inline constexpr uv_quad visited_end_room { { 0.25f, 0.0f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad visited_corner_room { { 0.0f, 0.0f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad visited_edge_room { { 0.0f, 0.25f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad visited_cross_room { { 0.25f, 0.25f }, { 0.25f, 0.25f } };
    
    inline constexpr uv_quad unvisited_end_room { { 0.25f, 0.5f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad unvisited_corner_room { { 0.0f, 0.5f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad unvisited_edge_room { { 0.0f, 0.75f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad unvisited_cross_room { { 0.25f, 0.75f }, { 0.25f, 0.25f } };
    
    inline constexpr uv_quad cross { { 0.75f, 0.75f }, { 0.25f, 0.25f } };
    
    inline constexpr uv_quad visited_path_down { { 0.5f, 0.5f }, { 0.125f, 0.0625f } };
    inline constexpr uv_quad visited_path_right { { 0.625f, 0.5f }, { 0.0625f, 0.125f } };
    
    inline constexpr uv_quad unvisited_path_down_transition { { 0.625f, 0.25f }, { 0.125f, 0.0625f } };
    inline constexpr uv_quad unvisited_path_right_transition { { 0.625f, 0.375f }, { 0.0625f, 0.125f } };
    
    inline constexpr uv_quad unvisited_path_down { { 0.5f, 0.625f }, { 0.125f, 0.0625f } };
    inline constexpr uv_quad unvisited_path_right { { 0.625f, 0.625f }, { 0.0625f, 0.125f } };
    
    inline constexpr uv_quad unvisited_path_down_end { { 0.5f, 0.75f }, { 0.125f, 0.0625f } };
    inline constexpr uv_quad unvisited_path_right_end { { 0.5f, 0.875f }, { 0.0625f, 0.125f } };
    
    inline constexpr uv_quad help_text { { 0.625f, 0.125f }, { 0.125f, 0.125f } };
    
    inline constexpr uv_quad yellow { { 0.5f, 0.0f }, { 0.03125f, 0.03125f } };
    inline constexpr uv_quad red { { 0.5f, 0.0625f }, { 0.03125f, 0.03125f } };
    
    inline constexpr uv_quad bar_on { { 0.625f, 0.0f }, { 0.0625f, 0.0625f } };
    inline constexpr uv_quad bar_off { { 0.625f, 0.0625f }, { 0.0625f, 0.0625f } };
    
    inline constexpr uv_quad marker_yellow { { 0.75f, 0.0f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad marker_green { { 0.75f, 0.25f }, { 0.25f, 0.25f } };
    inline constexpr uv_quad marker_red { { 0.75f, 0.5f }, { 0.25f, 0.25f } };
    
    struct {
        uv_quad uv;
        uv_translation translation;
    } mapped_rooms[] {
            // @formatter:off
            { cross, uv_translation::rot_0 },
            { unvisited_end_room, uv_translation::rot_0 }, { unvisited_end_room, uv_translation::rot_90 }, { unvisited_corner_room, uv_translation::rot_0 }, { unvisited_end_room, uv_translation::rot_180 }, // 1-4
            { cross, uv_translation::rot_0 }, { unvisited_corner_room, uv_translation::rot_90 }, { unvisited_edge_room, uv_translation::rot_0 }, { unvisited_end_room, uv_translation::rot_270 },
            { unvisited_corner_room, uv_translation::rot_270 }, { cross, uv_translation::rot_0 }, { unvisited_edge_room, uv_translation::rot_270 }, { unvisited_corner_room, uv_translation::rot_180 },
            { unvisited_edge_room, uv_translation::rot_180 }, { unvisited_edge_room, uv_translation::rot_90 }, { unvisited_cross_room, uv_translation::rot_0 },
        
            { cross, uv_translation::rot_0 },
            { visited_end_room, uv_translation::rot_0 }, { visited_end_room, uv_translation::rot_90 }, { visited_corner_room, uv_translation::rot_0 }, { visited_end_room, uv_translation::rot_180 }, // 1-4
            { cross, uv_translation::rot_0 }, { visited_corner_room, uv_translation::rot_90 }, { visited_edge_room, uv_translation::rot_0 }, { visited_end_room, uv_translation::rot_270 },
            { visited_corner_room, uv_translation::rot_270 }, { cross, uv_translation::rot_0 }, { visited_edge_room, uv_translation::rot_270 }, { visited_corner_room, uv_translation::rot_180 },
            { visited_edge_room, uv_translation::rot_180 }, { visited_edge_room, uv_translation::rot_90 }, { visited_cross_room, uv_translation::rot_0 },
            // @formatter:on
    };
}

void
update( ) {
    if(global_state.map.view_position != global_state.map.target_view_position) {
        bool x_less = global_state.map.view_position.x < global_state.map.target_view_position.x;
        bool y_less = global_state.map.view_position.y < global_state.map.target_view_position.y;
        glm::vec2 pos = glm::mix((glm::vec2) global_state.map.view_position, (glm::vec2) global_state.map.target_view_position, 0.1f);
        
        global_state.map.view_position = glm::ivec2((int) (x_less ? std::ceil(pos.x) : std::floor(pos.x)), (int) (y_less ? std::ceil(pos.y) : std::floor(pos.y)));
        global_state.queue_update = true;
        global_state.redraw = true;
        update_translation_position( );
    }
}

inline void
bind_texture(GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
}
void
draw_rect(const std::vector<rect>& rects) {
    static unsigned rect_draw_buffer_size = 1;
    if(rects.empty( )) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_instanced_pos_id);
    if(rect_draw_buffer_size < rects.size( )) {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (sizeof(rect) * rects.size( )), rects.data( ), GL_DYNAMIC_DRAW);
        rect_draw_buffer_size = rects.size( );
    } else
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr) (sizeof(rect) * rects.size( )), rects.data( ));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawElementsInstanced(GL_TRIANGLES, constants::quad_indices_count, GL_UNSIGNED_INT, nullptr, (int) rects.size( ));
}
void
draw_rect(const rect& r) {
    glBindBuffer(GL_ARRAY_BUFFER, global_state.opengl.shader.buffers.quad_instanced_pos_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr) sizeof(rect), &r);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawElementsInstanced(GL_TRIANGLES, constants::quad_indices_count, GL_UNSIGNED_INT, nullptr, 1);
}

void
render_background( ) {
    enable_translation(false);
    bind_texture(global_state.opengl.textures.background_id);
    draw_rect(rect { { constants::zero<int>, global_state.window.size }, textures::all, uv_translation::rot_0 });
}
void
render_map_scale( ) {
    std::vector<rect> bars;
    rect r { { (glm::ivec2) global_state.window.size - glm::ivec2(16, 6), { 8, 8 } }, textures::bar_on, uv_translation::rot_0 };
    for(unsigned i = constants::map::min_scale; i <= constants::map::max_scale; i++) {
        if(i - 1 == global_state.map.scale) r.texture = textures::bar_off;
        r.dimensions.position.y -= 4;
        bars.push_back(r);
    }
    
    enable_translation(false);
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(bars);
}
void
render_map( ) {
    rect r { };
    
    enable_translation(true);
    
    std::vector<rect> rects;
    rects.reserve(global_state.map.rooms.size( ));
    
    for(const auto& item: global_state.map.rooms) {
        rect room_rect { };
        room_rect.dimensions.position = item.second.position;
        (room_rect.dimensions.position *= 40) -= glm::ivec2(16);
        room_rect.dimensions.size = { 32, 32 };
        
        unsigned index = (unsigned) item.second.paths | item.second.visited << 4;
        auto room_view = textures::mapped_rooms[index];
        room_rect.texture = room_view.uv;
        room_rect.uv_tr = room_view.translation;
        
        rects.push_back(room_rect);
        
        if(item.second.flags != room_flag::none) {
            if(item.second.flags == room_flag::important_1)
                room_rect.texture = textures::marker_yellow;
            else if(item.second.flags == room_flag::important_2)
                room_rect.texture = textures::marker_green;
            else if(item.second.flags == room_flag::avoid)
                room_rect.texture = textures::marker_red;
            
            if(item.second.flags != room_flag::portal)
                rects.push_back(room_rect);
        }
        
        if((unsigned) (item.second.paths) & 0x1) {
            glm::ivec2 down = item.second.position + glm::ivec2 { 0, 1 };
            
            rect path_rect { };
            path_rect.dimensions.position = room_rect.dimensions.position + glm::ivec2(10, 32);
            path_rect.dimensions.size = { 16, 8 };
            path_rect.uv_tr = uv_translation::rot_0;
            
            auto found = global_state.map.rooms.find(point_id(down));
            if(found != std::end(global_state.map.rooms)) {
                if((unsigned) (found->second.paths) & 0x4) {
                    bool visited = item.second.visited && found->second.visited;
                    bool one_visited = item.second.visited || found->second.visited;
                    
                    bool other = found->second.visited;
                    if(one_visited && other) path_rect.uv_tr = uv_translation::flip_vert;
                    path_rect.texture = visited ? textures::visited_path_down : one_visited ? textures::unvisited_path_down_transition : textures::unvisited_path_down;
                }
            } else path_rect.texture = textures::unvisited_path_down_end;
            
            rects.push_back(path_rect);
        }
        
        if((unsigned) (item.second.paths) & 0x2) {
            glm::ivec2 right = item.second.position + glm::ivec2 { 1, 0 };
            
            rect path_rect { };
            path_rect.dimensions.position = room_rect.dimensions.position + glm::ivec2(32, 10);
            path_rect.dimensions.size = { 8, 16 };
            path_rect.uv_tr = uv_translation::rot_0;
            
            auto found = global_state.map.rooms.find(point_id(right));
            if(found != std::end(global_state.map.rooms)) {
                if((unsigned) (found->second.paths) & 0x8) {
                    bool visited = item.second.visited && found->second.visited;
                    bool one_visited = item.second.visited || found->second.visited;
                    bool other = found->second.visited;
                    if(one_visited && other) path_rect.uv_tr = uv_translation::flip_hori;
                    path_rect.texture = visited ? textures::visited_path_right : one_visited ? textures::unvisited_path_right_transition : textures::unvisited_path_right;
                }
            } else path_rect.texture = textures::unvisited_path_right_end;
            
            rects.push_back(path_rect);
        }
        
        if((unsigned) (item.second.paths) & 0x4) {
            glm::ivec2 up = item.second.position + glm::ivec2 { 0, -1 };
            
            auto found = global_state.map.rooms.find(point_id(up));
            if(found == std::end(global_state.map.rooms)) {
                rect path_rect { };
                path_rect.dimensions.position = room_rect.dimensions.position + glm::ivec2(10, -8);
                path_rect.dimensions.size = { 16, 8 };
                path_rect.uv_tr = uv_translation::flip_vert;
                path_rect.texture = textures::unvisited_path_down_end;
                rects.push_back(path_rect);
            }
        }
        
        if((unsigned) (item.second.paths) & 0x8) {
            glm::ivec2 left = item.second.position + glm::ivec2 { -1, 0 };
            
            auto found = global_state.map.rooms.find(point_id(left));
            if(found == std::end(global_state.map.rooms)) {
                rect path_rect { };
                path_rect.dimensions.position = room_rect.dimensions.position + glm::ivec2(-8, 10);
                path_rect.dimensions.size = { 8, 16 };
                path_rect.uv_tr = uv_translation::flip_hori;
                path_rect.texture = textures::unvisited_path_right_end;
                rects.push_back(path_rect);
            }
        }
        
    }
    
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(rects);
}
void
render_player_dot( ) {
    // Player dot
    enable_translation(true);
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(rect { { (global_state.map.position * 40 - glm::ivec2(8)) * (int) global_state.global_scale,
            glm::uvec2(16, 16) * global_state.global_scale }, textures::player_dot, uv_translation::rot_0 });
}
void
render_portal( ) {
    // Portal
    enable_translation(true);
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(rect { { glm::ivec2(-3, -4) * (int) global_state.global_scale, glm::uvec2(16, 16) * global_state.global_scale }, textures::portal, uv_translation::rot_0 });
}
void
render_map_compass( ) {
    // Compass
    enable_translation(false);
    bind_texture(global_state.opengl.textures.compass_id);
    draw_rect(rect { { glm::uvec2(global_state.window.size.x - 128 - 8, 8), { 128, 128 } }, textures::all, uv_translation::rot_0 });
}
void
render_last_path( ) {
    std::vector<rect> paths;
    rect hori { { constants::zero<int>, { 14, 2 } }, textures::red, uv_translation::rot_0 };
    rect vert { { constants::zero<int>, { 2, 14 } }, textures::red, uv_translation::rot_0 };
    
    for(unsigned i = 0; i < global_state.map.path_size; i++) {
        unsigned index = i + global_state.map.path_head;
        if(index >= constants::map::path_count) index -= constants::map::path_count;
        
        auto [point, dir] = global_state.map.path[index];
        glm::ivec2 pos = point *= 40;
        
        if(dir == path_flag::north) {
            pos += glm::ivec2(-1, -28);
            vert.dimensions.position = pos;
            paths.push_back(vert);
        }
        if(dir == path_flag::south) {
            pos += glm::ivec2(-1, 14);
            vert.dimensions.position = pos;
            paths.push_back(vert);
        }
        if(dir == path_flag::west) {
            pos += glm::ivec2(-28, -1);
            hori.dimensions.position = pos;
            paths.push_back(hori);
        }
        if(dir == path_flag::east) {
            pos += glm::ivec2(14, -1);
            hori.dimensions.position = pos;
            paths.push_back(hori);
        }
    }
    
    enable_translation(true);
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(paths);
}
void
render_portal_path( ) {
    if(global_state.map.portal_path.empty( )) return;
    
    std::vector<rect> paths;
    rect hori { { constants::zero<int>, { 40, 2 } }, textures::yellow, uv_translation::rot_0 };
    rect vert { { constants::zero<int>, { 2, 40 } }, textures::yellow, uv_translation::rot_0 };
    
    glm::ivec2 point = constants::zero<int>;
    for(const auto& dir: global_state.map.portal_path) {
        glm::ivec2 pos = point * 40;
        
        if(dir == constants::north<int>) {
            pos += glm::ivec2(-1, -39);
            vert.dimensions.position = pos;
            paths.push_back(vert);
        }
        if(dir == constants::south<int>) {
            pos += glm::ivec2(-1, -1);
            vert.dimensions.position = pos;
            paths.push_back(vert);
        }
        if(dir == constants::west<int>) {
            pos += glm::ivec2(-39, -1);
            hori.dimensions.position = pos;
            paths.push_back(hori);
        }
        if(dir == constants::east<int>) {
            pos += glm::ivec2(-1, -1);
            hori.dimensions.position = pos;
            paths.push_back(hori);
        }
        
        point += dir;
    }
    
    enable_translation(true);
    bind_texture(global_state.opengl.textures.texture_id);
    draw_rect(paths);
}
void
render( ) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    render_background( );
    
    if(!global_state.map.pick_direction) {
        glProgramUniform1i(global_state.opengl.shader.program, global_state.opengl.shader.uniforms.border_fade_id, 1);
        render_map( );
        render_last_path( );
        render_portal_path( );
        render_portal( );
        render_player_dot( );
        glProgramUniform1i(global_state.opengl.shader.program, global_state.opengl.shader.uniforms.border_fade_id, 0);
        render_map_compass( );
    } else {
        // Compass
        enable_translation(false);
        bind_texture(global_state.opengl.textures.compass_id);
        draw_rect(rect { { { (global_state.window.size.x - 256) >> 1, (global_state.window.size.y - 256) >> 1 }, { 256, 256 } }, textures::all, uv_translation::rot_0 });
    }
    
    render_map_scale( );
    
    if(global_state.enable_global_keys) {
        // Global keys dot
        enable_translation(false);
        bind_texture(global_state.opengl.textures.texture_id);
        draw_rect(rect { { { 8, global_state.window.size.y - 16 - 8 }, glm::uvec2(16, 16) * global_state.global_scale }, textures::player_dot, uv_translation::rot_0 });
    }
    
    if(global_state.show_help) {
        // Help
        enable_translation(false);
        bind_texture(global_state.opengl.textures.help_id);
        draw_rect(rect { { constants::zero<int>, global_state.window.size }, textures::all, uv_translation::rot_0 });
    } else {
        // Help F1
        enable_translation(false);
        bind_texture(global_state.opengl.textures.texture_id);
        draw_rect(rect { { { 12, 12 }, { 32, 32 } }, textures::help_text, uv_translation::rot_0 });
    }
}
