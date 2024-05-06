#version 430 core
#define PI 3.1415926535897932384626433832795

out vec4 LFragment;
in vec4 gl_FragCoord;

uniform vec2 WindowSize;
uniform float time; // within [0, 1000] in seconds
const float MAX_DISTANCE = 256;

const struct Material {
    vec3 color;
    float reflection;
    float ior;
    float transparency;
    vec3 emision_color;
    float emision_strength;
    vec3 volume_color;
    float volume;
} materials[7] = Material[](
    //         color          reflection       IOR    transparency    emission          emission_strength    volume_color     volume
    Material(vec3(0, 0, 0),         0,          1,          1,      vec3(0, 0, 0),             0,            vec3(0.5, 1, 1),   .005),  // air
    Material(vec3(0, .75, 0),       0,          1,          0,      vec3(0, 0, 0),             0,            vec3(0, 0, 0),        0),  // grass
    Material(vec3(.4, .2, .1),      0,          1,          0,      vec3(0, 0, 0),             0,            vec3(0, 0, 0),        0),  // dirt
    Material(vec3(.25, .25, .25),   0,          1,          0,      vec3(0, 0, 0),             0,            vec3(0, 0, 0),        0),  // stone
    Material(vec3(0.75, 1, 1),      .75,        1.333,      .75,    vec3(0, 0, 0),             0,            vec3(0, .75, .5),   .15),  // water
    Material(vec3(1, .3, 0),        0,          1,          0,      vec3(0, 0, 0),             0,            vec3(0, 0, 0),        0),  // building
    Material(vec3(.9, .9, 1),      .8,          1,          0,      vec3(.9, .9, 1),      /*3*/0,            vec3(0, 0, 0),        0)   // light
);

struct Cell {
    uint value;
    uint sub_cell[8];
};
layout(std430, binding = 1) readonly buffer world_data_layout
{
    Cell world_data[];
};
layout(std430, binding = 2) readonly buffer world_indexes_layout
{
    uint world_width;
    uint chunk_width;
    uint world_indexes[];
};

Material get_material(uint value) {
    if (value > 6) return Material(vec3(1, 0, 1), 0, 1, 0, vec3(1, 0, 1), 1, vec3(0), 0);
    return materials[value];
}
struct CellInfo {
    Cell cell;
    uint width;
    vec3 hit_point;
    vec3 normal;
};
bool in_bounds(vec3 pos) {
    return 
        abs(pos.x) <= chunk_width * ((world_width-1) >> 1) &&
        abs(pos.y) <= chunk_width * ((world_width-1) >> 1) &&
        abs(pos.z) <= chunk_width * ((world_width-1) >> 1);
}
CellInfo get_cell_info(vec3 index) {
    if (!in_bounds(index)) return CellInfo(Cell(0, uint[8](0, 0, 0, 0, 0, 0, 0, 0)), chunk_width, vec3(0, 0, 0), vec3(0, 1, 0));

    int chunk_x = int(floor(index.x / chunk_width));
    int chunk_y = int(floor(index.y / chunk_width));
    int chunk_z = int(floor(index.z / chunk_width));
    uint x = uint(mod(index.x, chunk_width));
    uint y = uint(mod(index.y, chunk_width));
    uint z = uint(mod(index.z, chunk_width));
    chunk_x -= int(world_width) * int(floor(float(chunk_x) / world_width));
    chunk_y -= int(world_width) * int(floor(float(chunk_y) / world_width));
    chunk_z -= int(world_width) * int(floor(float(chunk_z) / world_width));

    uint temp_index = world_width * (world_width * chunk_x + chunk_y) + chunk_z;
    if (temp_index < 0 || temp_index >= world_width * world_width * world_width)
        return CellInfo(Cell(0, uint[8](0, 0, 0, 0, 0, 0, 0, 0)), chunk_width, vec3(0, 0, 0), vec3(0, 1, 0));

    uint chunk_index = world_indexes[temp_index];
    if (chunk_index == 0) return CellInfo(Cell(0, uint[8](0, 0, 0, 0, 0, 0, 0, 0)), chunk_width, vec3(0, 0, 0), vec3(0, 1, 0));
    chunk_index -= 1;
    uint cell_offset = 0;

    uint current_cell_width = chunk_width;
    while (current_cell_width > 1) {
        current_cell_width >>= 1;
        
        // (x, y, z) -> ²xyz
        // (0, 1, 0) -> ²010 = 2
        uint code = (uint(x / current_cell_width) << 2) | (uint(y / current_cell_width) << 1) | uint(z / current_cell_width);

        if (world_data[chunk_index + cell_offset].sub_cell[code] == 0) {
            current_cell_width <<= 1;
            for (int i = 0; i < 8; i++) if (world_data[chunk_index + cell_offset].sub_cell[i] != 0) {
                current_cell_width >>= 1;
                break;
            }
            break;
        }
        cell_offset = world_data[chunk_index + cell_offset].sub_cell[code];

        x %= current_cell_width;
        y %= current_cell_width;
        z %= current_cell_width;
    }

    Cell cell = world_data[chunk_index + cell_offset];
    return CellInfo(cell, current_cell_width, vec3(0, 0, 0), vec3(0, 1, 0));
}

float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}
float gaussian(float v) {
    return exp(-v*v);
}
float smooth_sign(float v) {
    return v / (1 + abs(v));
}

mat3 rotation_matrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c          );
}

uniform float facing_pitch;
uniform float facing_yaw;
uniform float FOV;
vec3 get_direction() {
    float x = (gl_FragCoord.x / WindowSize.x - 0.5) * 2;
    float y = (gl_FragCoord.y / WindowSize.y - 0.5) * 2;
    float aspect = WindowSize.x / WindowSize.y;

    vec3 direction = normalize(vec3(
        1 / tan(FOV / 2),
        aspect * x,
        y
    ));
    direction = rotation_matrix(vec3(0, 1, 0), facing_pitch) * direction;
    direction = rotation_matrix(vec3(0, 0, 1), -facing_yaw) * direction;
    return direction;


    // float x = gl_FragCoord.x + 0.5;
    // float y = gl_FragCoord.y + 0.5;
    // float aspect = WindowSize.x / WindowSize.y;

    // vec3 direction = normalize(vec3(
    //     1 / tan(FOV / 2),
    //     aspect * (2 * x / WindowSize.x) - 1,
    //     (2 * y / WindowSize.y) - 1
    // ));
    // direction = rotation_matrix(vec3(0, 1, 0), facing_pitch) * direction;
    // direction = rotation_matrix(vec3(0, 0, 1), -facing_yaw) * direction;
    // return direction;
}
CellInfo get_next_cell(uint cell_size, vec3 position, vec3 direction) {
    vec3 pos_in_cell = mod(position / cell_size, 1.0); // in [0, 1[
    if (direction.x < 0 && pos_in_cell.x == 0) pos_in_cell.x = 1; // avoid being trap on the edge
    if (direction.y < 0 && pos_in_cell.y == 0) pos_in_cell.y = 1; // avoid being trap on the edge
    if (direction.z < 0 && pos_in_cell.z == 0) pos_in_cell.z = 1; // avoid being trap on the edge
    
    float step_size = 3;
    vec3 cell_jump = vec3(sign(direction.x), 0, 0);
    if (direction.x > 0) step_size = (1 - pos_in_cell.x) / direction.x;
    else if (direction.x < 0) step_size = -pos_in_cell.x / direction.x;
    
    float temp_step = 3;
    if (direction.y > 0) temp_step = (1 - pos_in_cell.y) / direction.y;
    else if (direction.y < 0) temp_step = -pos_in_cell.y / direction.y;
    if (temp_step < step_size) {
        step_size = temp_step;
        cell_jump = vec3(0, sign(direction.y), 0);
    }
    
    temp_step = 3;
    if (direction.z > 0) temp_step = (1 - pos_in_cell.z) / direction.z;
    else if (direction.z < 0) temp_step = -pos_in_cell.z / direction.z;
    if (temp_step < step_size) {
        step_size = temp_step;
        cell_jump = vec3(0, 0, sign(direction.z));
    }

    position += direction * step_size * cell_size;

    CellInfo result = get_cell_info(floor(position + 0.1 * cell_jump));
    result.hit_point = position;
    result.normal = -cell_jump;

    return result;
}

uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}
uint hash( uvec2 v ) { return hash( uint(v.x) ^ hash(uint(v.y))                         ); }
uint hash( uvec3 v ) { return hash( uint(v.x) ^ hash(uint(v.y)) ^ hash(uint(v.z))             ); }
uint hash( uvec4 v ) { return hash( uint(v.x) ^ hash(uint(v.y)) ^ hash(uint(v.z)) ^ hash(uint(v.w)) ); }
// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}
float simplex(vec2 pos) {
    /*
    v1      i1      v2

            I


    v3      i2      v4
    */

    uvec2 p1 = uvec2(abs(floor(pos)));
    float i1 = mix(floatConstruct(hash(p1 + uvec2(0, 0))), floatConstruct(hash(p1 + uvec2(1, 0))), fract(pos.x));
    float i2 = mix(floatConstruct(hash(p1 + uvec2(0, 1))), floatConstruct(hash(p1 + uvec2(1, 1))), fract(pos.x));
    return mix(i1, i2, fract(pos.y));
}
float simplex(vec3 pos) {
    /*
    v1      i1      v2

            I


    v3      i2      v4
    */

    uvec3 p1 = uvec3(abs(floor(pos)));
    float i1 = mix(floatConstruct(hash(p1 + uvec3(0, 0, 0))), floatConstruct(hash(p1 + uvec3(1, 0, 0))), fract(pos.x));
    float i2 = mix(floatConstruct(hash(p1 + uvec3(0, 1, 0))), floatConstruct(hash(p1 + uvec3(1, 1, 0))), fract(pos.x));
    float v_down = mix(i1, i2, fract(pos.y));
    i1 = mix(floatConstruct(hash(p1 + uvec3(0, 0, 1))), floatConstruct(hash(p1 + uvec3(1, 0, 1))), fract(pos.x));
    i2 = mix(floatConstruct(hash(p1 + uvec3(0, 1, 1))), floatConstruct(hash(p1 + uvec3(1, 1, 1))), fract(pos.x));
    float v_up = mix(i1, i2, fract(pos.y));
    return mix(v_down, v_up, fract(pos.z));
}
float perlin(vec2 pos, uint occ, float min_v, float max_v) {
    float v = simplex(pos);
    float strength = 1;

    for (int i = 0; i < occ; i++) {
        pos *= 2;
        strength /= 2;
        v += simplex(pos) * strength;
    }
    return clamp(v * (max_v - min_v) + min_v, min_v, max_v);
}
float perlin(vec3 pos, uint occ, float min_v, float max_v) {
    float v = simplex(pos);
    float strength = 1;

    for (int i = 0; i < occ; i++) {
        pos *= 2;
        strength /= 2;
        v += simplex(pos) * strength;
    }
    return clamp(v * (max_v - min_v) + min_v, min_v, max_v);
}

vec3 bump(vec3 normal, vec3 pos, float strength, float size, uint detail) {
    pos -= normal * dot(normal, pos);
    vec3 left = vec3(0, 1, 0);
    vec3 forward = vec3(1, 0, 0);
    if (normal != left) forward = cross(normal, left);
    if (normal != forward) left = cross(forward, normal);

    // float delta_lr = (perlin((pos + left * 0.01) / size, detail, -1, 1) - perlin((pos - left * 0.01) / size, detail, -1, 1)) / 0.02;
    // float delta_bf = (perlin((pos + forward * 0.01) / size, detail, -1, 1) - perlin((pos - forward * 0.01) / size, detail, -1, 1)) / 0.02;
    // return normalize(normal + (delta_lr * left + delta_bf * forward) * strength);

    return normalize(normal + (perlin(pos.xyz / size, detail, -1, 1) * left + perlin(pos.yxz / size, detail, -1, 1) * forward) * strength);
}

uniform vec3 player_position;
uniform vec3 player_target;

float chunk_edge_distance(vec3 pos) {
    vec3 pos_in_chunk = mod(pos / chunk_width, 1.0); // in [0, 1]
    pos_in_chunk = pos_in_chunk * 2 - vec3(1, 1, 1); // in [-1, 1] with 0 in center
    pos_in_chunk = abs(pos_in_chunk);
    float corner_strenght = max(pos_in_chunk.x, max(pos_in_chunk.y, pos_in_chunk.z)); // in [0, 1] with 1 on the edges

    // return 0;
    return corner_strenght;
}
vec4 compute_color(CellInfo cell_info) {
    float edge = pow(chunk_edge_distance(cell_info.hit_point), 4);
    edge = min(0.75, edge);

    Material material = get_material(cell_info.cell.value);

    vec4 color = vec4(mix(material.color, vec3(1, 1, 1), edge), 1);

    vec4 random_color = vec4(1,1,1,1);
    vec3 p = floor(cell_info.hit_point / cell_info.width) * cell_info.width;
    random_color.x = floatConstruct(hash(uvec3(floor(p.xyz))));
    random_color.y = floatConstruct(hash(uvec3(floor(p.yzx))));
    random_color.z = floatConstruct(hash(uvec3(floor(p.zxy))));
    color = mix(color, random_color, 0.1);

    if (floor(cell_info.hit_point - cell_info.normal * 0.1) == floor(player_target))
        color = mix(color, vec4(1, 1, 1, 1), 0.5);

    float light = clamp(-dot(cell_info.normal, normalize(vec3(-2, -4, -8))), 0.25, 1);
    // color.xyz = mix(color.xyz * light, material.emision_color, material.emision_strength);
    color.xyz = (color.xyz + material.emision_color * material.emision_strength)* light;
    
    float dist = distance(player_position, cell_info.hit_point); // [0; +inf[
    dist = clamp(1 - dist / MAX_DISTANCE, 0, 1); // [0; 1[ with 1 near player
    color.w = dist;

    return color;
}

vec3 raycast(vec3 direction, vec3 start_position) {
    vec4 final_color = vec4(0, 0, 0, 0);
    uint coord_offset = 1;

    if (start_position.x == floor(start_position.x)) start_position.x += 0.001;
    if (start_position.y == floor(start_position.y)) start_position.y += 0.001;
    if (start_position.z == floor(start_position.z)) start_position.z += 0.001;

    CellInfo cell_info = get_cell_info(floor(start_position));
    uint start_cell_type = cell_info.cell.value;
    float current_ior = 1;

    cell_info.hit_point = start_position;
    if (cell_info.cell.value != 0 && in_bounds(start_position)) {
        float x = (gl_FragCoord.x - WindowSize.x / 2) / (WindowSize.x / 2);
        float y = (gl_FragCoord.y - WindowSize.y / 2) / (WindowSize.y / 2);
        Material material = get_material(cell_info.cell.value);
        final_color = vec4(material.color, 1) * min(0.1 + (x*x+y*y) / 2, 1) * (1 - material.transparency);

        current_ior = material.ior;
    }

    uint max_iter = uint(ceil(MAX_DISTANCE / 2));
    float max_dist = MAX_DISTANCE;
    int iter = 0;
    for (int i = 0; i < max_iter; i++) {
        iter = i;
        if (distance(start_position, cell_info.hit_point) > max_dist) break;
        
        cell_info = get_next_cell(cell_info.width, cell_info.hit_point, direction);

        if (cell_info.cell.value != start_cell_type && in_bounds(cell_info.hit_point - cell_info.normal * 0.01)) {
            final_color += vec4(get_material(start_cell_type).volume_color, 1) * (1 - final_color.w) * smooth_sign(get_material(start_cell_type).volume * distance(start_position, cell_info.hit_point));

            max_dist -= distance(start_position, cell_info.hit_point);
            start_position = cell_info.hit_point;

            vec3 true_normal = cell_info.normal;
            if (cell_info.cell.value == 4 /*INTO WATER*/) {
                cell_info.normal = bump(cell_info.normal, cell_info.hit_point + vec3(time * 0.5, time, 0), 0.05, 1, 2);
            }
            if (start_cell_type == 4 /*OUT OF WATER*/) {
                cell_info.normal = bump(cell_info.normal, cell_info.hit_point + vec3(time * 0.5, time, 0), -0.05, 1, 2);
            }
            Material material = get_material(cell_info.cell.value);

            bool refraction_ray = material.transparency != 0;
            bool reflection_ray = material.reflection != 0;
            
            if (refraction_ray && dot(cell_info.normal, direction) >= 0) {
                refraction_ray = false;
            }
            else if (refraction_ray && refract(direction, cell_info.normal, current_ior / material.ior) == vec3(0)) { // switch to inner refraction
                refraction_ray = false;
                reflection_ray = true;
                material = get_material(start_cell_type);
            }
            else if (refraction_ray && reflection_ray) {
                coord_offset *= 2;
                refraction_ray = int(gl_FragCoord.x+gl_FragCoord.y) % coord_offset < coord_offset / 2;
                reflection_ray = ! refraction_ray;
            }

            if (refraction_ray) {
                start_cell_type = cell_info.cell.value;
                final_color += compute_color(cell_info) * (1 - final_color.w) * (1 - material.transparency);

                direction = refract(direction, cell_info.normal, current_ior / material.ior);
                cell_info.hit_point -= true_normal * 0.01;
                current_ior = material.ior;

                continue;
            }
            else if (reflection_ray) {
                final_color += compute_color(cell_info) * (1 - final_color.w) * (1 - material.reflection);

                direction = reflect(direction, cell_info.normal);
                cell_info.hit_point += true_normal * 0.01;
                cell_info.width = 1;

                continue;
            }

            final_color += compute_color(cell_info) * (1 - final_color.w);
            break;
        }
    }
    final_color += vec4(get_material(start_cell_type).volume_color, 1) * (1 - final_color.w) * smooth_sign(get_material(start_cell_type).volume * max_dist);

    vec3 sky_color = vec3(0, 0.75, 1) * 2;
    final_color.xyz = mix(sky_color, final_color.xyz, final_color.w);

    float v = iter / (ceil(MAX_DISTANCE / 2));
    return vec3(v, v, v);
    return final_color.xyz;
}

uniform float deltatime;

void main()
{
    int fps = int(round(1 / max(1/120, deltatime)));
    if (gl_FragCoord.x < 10 && gl_FragCoord.y < fps * (WindowSize.y / 120)) {
        if (fps >= 60) LFragment = vec4(0, 1, 0, 1.0);
        else if (fps >= 30) LFragment = vec4(1, 1, 0, 1.0);
        else if (fps >= 15) LFragment = vec4(1, 0, 0, 1.0);
        else LFragment = vec4(0, 0, 0, 1.0);

        return;
    }

    float x = gl_FragCoord.x - WindowSize.x / 2;
    float y = gl_FragCoord.y - WindowSize.y / 2;

    // if (gl_FragCoord.x * 2 < world_width * world_width * world_width) {
    //     int start = int(world_indexes[int(gl_FragCoord.x / 2)]);
    //     if (start != 0) {
    //         int end = -1;
    //         for (int i = 0; i < world_width * world_width * world_width; i++) {
    //             if (world_indexes[i] >= start && i != int(gl_FragCoord.x / 2)) {
    //                 if (end == -1 || (end - start > world_indexes[i] - start)) end = int(world_indexes[i]);
    //             }
    //         }
    //         if (end == -1) end = start;

    //         int non_air = 0;
    //         int air = 0;
    //         for (int i = start; i < end; i++) {
    //             if (world_data[i].value != 0) non_air ++;
    //             else air ++;
    //         }

    //         if (gl_FragCoord.y < non_air / 10) {
    //             LFragment = vec4(1.0, 0, 1.0, 1.0);
    //             return;
    //         }
    //         if (gl_FragCoord.y < (non_air + air) / 10) {
    //             LFragment = vec4(0, 0.5, 1.0, 1.0);
    //             return;
    //         }
    //     }
    // }

    if (abs(x) <= 1 && abs(y) <= 32 || abs(y) <= 1 && abs(x) <= 32) // cross
        LFragment = vec4(1.0, 1.0, 1.0, 1.0);
    else {
        vec3 direction = get_direction();
        LFragment = vec4(raycast(direction, player_position + direction * 0.5), 1.0);
    }
}