#version 440

layout(location = 0) in frag_data {
    vec2 pixel;
    vec2 position;
    vec2 uv;
} frag;

layout(binding = 0, std140) uniform screen_info {
    uvec2 size;
} screen;

layout(binding = 1, std140) uniform translation_info {
    vec2 position;
    uint scale;
    bool enabled;
} translation;

layout(binding = 0) uniform sampler2D image;
uniform bool border_fade = false;

layout(location = 0) out vec4 out_color;

const int border_begin = 8;
const int border_length = 32;

void main() {
    vec4 color = texture(image, frag.uv);
    //    if(!alpha && color.a < 0.5) discard;
    if (border_fade) {
        ivec2 s = ivec2(screen.size) / 2;
        ivec2 p = ivec2(abs(frag.position * vec2(screen.size) * 0.5));

        vec2 n = clamp(p - s + (border_begin + border_length).xx, 0, border_length);
        float q = max(n.x, n.y);

        color.a *= 1 - (q / float(border_length));
    }
    out_color = color;
}