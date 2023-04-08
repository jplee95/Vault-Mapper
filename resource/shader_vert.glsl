#version 440

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
// Instanced
layout(location = 2) in ivec2 position;
layout(location = 3) in uvec2 size;
layout(location = 4) in vec2 uv_position;
layout(location = 5) in vec2 uv_size;
layout(location = 6) in uint uv_tr;

layout(binding = 0, std140) uniform screen_info {
    uvec2 size;
} screen;

layout(binding = 1, std140) uniform translation_info {
    vec2 position;
    uint scale;
    bool enabled;
} translation;

layout(location = 0) out frag_data {
    vec2 pixel;
    vec2 position;
    vec2 uv;
} frag;

vec2
transform_uv(vec2 uv, uint index);

void main() {
    vec2 pixel = vec2(2.0 / float(screen.size.x), 2.0 / float(screen.size.y));
    frag.uv = uv_position + (uv_size * transform_uv(uv, uv_tr));

    vec2 pixel_position = vec2(position.x, -position.y) * pixel + vec2(-1, 1);
    vec2 current_position = vec2(vertex.xy * pixel * size + pixel_position);
    if (translation.enabled) current_position = (vec2(translation.position.x, -translation.position.y) * pixel + current_position) * float(translation.scale);

    frag.pixel = pixel;
    frag.position = current_position;
    gl_Position = vec4(current_position, vertex.z, 1.0);
}

const vec2 uv_rotations[] = vec2[](
vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1), // Rotation 0
vec2(1, 0), vec2(1, 1), vec2(0, 0), vec2(0, 1), // Rotation 90
vec2(1, 1), vec2(0, 1), vec2(1, 0), vec2(0, 0), // Rotation 180
vec2(0, 1), vec2(0, 0), vec2(1, 1), vec2(1, 0), // Rotation 270
vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0), // Flip Verticaly
vec2(1, 0), vec2(0, 0), vec2(1, 1), vec2(0, 1)  // Flip Horizontaily
);

vec2
transform_uv(vec2 uv, uint index) {
    uint a = index * 4 + uint(uv.y) * 2 + uint(uv.x);
    return uv_rotations[a];
}