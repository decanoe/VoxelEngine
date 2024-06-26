#version 430 core
#define PI 3.1415926535897932384626433832795

out vec4 LFragment;
in vec4 gl_FragCoord;

uniform vec2 WindowSize;
uniform float time; // within [0, 1000] in seconds
uniform float deltatime;
uniform vec3 debug_time;

uniform vec3 player_position;
uniform vec3 player_target;
uniform float facing_pitch;
uniform float facing_yaw;
uniform float FOV;

const float MAX_DISTANCE = 256;
const int MAX_ITER = int(ceil(MAX_DISTANCE / 2));
const uint MAX_BOUNCE = 3;

//#region Materials
#define AIR 0
#define GRASS 1
#define DIRT 2
#define STONE 3
#define WATER 4
#define BUILDING 5
#define LIGHT 6
#define START_MATERIAL LIGHT

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
#define UNKNOWN_MATERIAL Material(vec3(1, 0, 1), 0, 1, 0, vec3(1, 0, 1), 1, vec3(0), 0)
Material get_material(uint value) {
    if (value > START_MATERIAL) return UNKNOWN_MATERIAL;
    return materials[value];
}
//#endregion

//#region Data
struct Cell {
    uint value;
    uint sub_cell[8];
};
layout(std430, binding = 1) readonly buffer world_data_layout {
    Cell world_data[];
};
layout(std430, binding = 2) readonly buffer world_indexes_layout {
    uint world_width;
    uint chunk_width;
    uint world_indexes[];
};

bool in_bounds(vec3 pos) {
    return 
        abs(pos.x) <= chunk_width * ((world_width-1) >> 1) &&
        abs(pos.y) <= chunk_width * ((world_width-1) >> 1) &&
        abs(pos.z) <= chunk_width * ((world_width-1) >> 1);
}
struct ValueSize { uint value; uint size; };
ValueSize get_cell_value(vec3 index) {
    if (!in_bounds(index)) return ValueSize(AIR, chunk_width);

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
    if (temp_index >= world_width * world_width * world_width) return ValueSize(AIR, chunk_width);

    uint chunk_index = world_indexes[temp_index];
    if (chunk_index == 0) return ValueSize(AIR, chunk_width);
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

    return ValueSize(world_data[chunk_index + cell_offset].value, current_cell_width);
}
//#endregion

//#region graphic functions
float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}
float gaussian(float v) {
    return exp(-v*v);
}
float smooth_sign(float v) {
    return v / (1 + abs(v));
}
//#endregion
//#region noise
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
//#endregion

//#region Direction
mat3 rotation_matrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c          );
}

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
}
//#endregion


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






struct RaycastHit {
    uint value;
    vec3 hit_point;
    vec3 normal;

    int step_taken;
};
RaycastHit raycast(vec3 direction, vec3 start_position, int max_step, float max_dist, uint ignored_material) {
    //#region initialize steps
    vec3 next_pos_X = vec3(0);
    vec3 next_pos_Y = vec3(0);
    vec3 next_pos_Z = vec3(0);
    vec3 step_X = vec3(0);
    vec3 step_Y = vec3(0);
    vec3 step_Z = vec3(0);
    float next_dist_X = 0;
    float next_dist_Y = 0;
    float next_dist_Z = 0;
    float step_size_X = 0;
    float step_size_Y = 0;
    float step_size_Z = 0;

    vec3 pos_in_cell = mod(start_position, 1.0);
    if (direction.x < 0 && pos_in_cell.x == 0) pos_in_cell.x = 1; // avoid being trap on the edge
    if (direction.y < 0 && pos_in_cell.y == 0) pos_in_cell.y = 1; // avoid being trap on the edge
    if (direction.z < 0 && pos_in_cell.z == 0) pos_in_cell.z = 1; // avoid being trap on the edge

    //#region initialize X
    if (abs(direction.x) > 0.001) {
        step_X = direction / abs(direction.x);
        step_size_X = 1 / abs(direction.x);
        
        if (direction.x > 0) next_pos_X = start_position + (1 - pos_in_cell.x) * step_X;
        else next_pos_X = start_position + pos_in_cell.x * step_X;
        
        next_dist_X = distance(next_pos_X, start_position);
    }
    else {
        next_dist_X = MAX_DISTANCE * 2;
    }
    //#endregion
    //#region initialize Y
    if (abs(direction.y) > 0.001) {
        step_Y = direction / abs(direction.y);
        step_size_Y = 1 / abs(direction.y);
        
        if (direction.y > 0) next_pos_Y = start_position + (1 - pos_in_cell.y) * step_Y;
        else next_pos_Y = start_position + pos_in_cell.y * step_Y;
        
        next_dist_Y = distance(next_pos_Y, start_position);
    }
    else {
        next_dist_Y = MAX_DISTANCE * 2;
    }
    //#endregion
    //#region initialize Z
    if (abs(direction.z) > 0.001) {
        step_Z = direction / abs(direction.z);
        step_size_Z = 1 / abs(direction.z);
        
        if (direction.z > 0) next_pos_Z = start_position + (1 - pos_in_cell.z) * step_Z;
        else next_pos_Z = start_position + pos_in_cell.z * step_Z;
        
        next_dist_Z = distance(next_pos_Z, start_position);
    }
    else {
        next_dist_Z = MAX_DISTANCE * 2;
    }
    //#endregion
    //#endregion

    int nb_steps = 0;
    vec3 cell_pos = floor(start_position);
    vec3 position = start_position;
    vec3 normal = -direction;
    float distance = 0;

    ValueSize vs = get_cell_value(cell_pos);
    if (vs.value != ignored_material) return RaycastHit(vs.value, position, normal, nb_steps);
    vec3 last_cell_pos = cell_pos;

    while (nb_steps < max_step && distance < max_dist) {

        if (floor(cell_pos / vs.size) != floor(last_cell_pos / vs.size)) {
            nb_steps++;
            vs = get_cell_value(cell_pos);
            if (vs.value != ignored_material) return RaycastHit(vs.value, position, normal, nb_steps);
            last_cell_pos = cell_pos;
        }

        if (next_dist_X <= next_dist_Y && next_dist_X <= next_dist_Z) {
            cell_pos.x += sign(direction.x);
            position = next_pos_X;
            normal = vec3(-sign(direction.x), 0, 0);
            distance = next_dist_X;
            next_dist_X += step_size_X;
            next_pos_X += step_X;
        }
        else if (next_dist_Y <= next_dist_Z) {
            cell_pos.y += sign(direction.y);
            position = next_pos_Y;
            normal = vec3(0, -sign(direction.y), 0);
            distance = next_dist_Y;
            next_dist_Y += step_size_Y;
            next_pos_Y += step_Y;
        }
        else {
            cell_pos.z += sign(direction.z);
            position = next_pos_Z;
            normal = vec3(0, 0, -sign(direction.z));
            distance = next_dist_Z;
            next_dist_Z += step_size_Z;
            next_pos_Z += step_Z;
        }
    }

    return RaycastHit(AIR, start_position + direction * MAX_DISTANCE, -direction, nb_steps);
}

vec3 get_color() {
    vec3 direction = get_direction();
    int step_left = MAX_ITER;
    float dist_left = MAX_DISTANCE;
    vec3 pos = player_position + direction * 0.5;
    uint start_value = get_cell_value(player_position).value;
    Material start_material = get_material(start_value);

    vec4 final_color = vec4(0, 0, 0, 0);

    uint coord_offset = 1;

    for (uint b = 0; b <= MAX_BOUNCE; b++) {
        RaycastHit hit = raycast(direction, pos, step_left, dist_left, start_value);
        step_left -= hit.step_taken;
        dist_left -= distance(pos, hit.hit_point);
        if (step_left <= 0 || dist_left <= 0) break;

        // add material volume
        final_color += vec4(start_material.volume_color, 1) * (1 - final_color.w) * smooth_sign(start_material.volume * distance(pos, hit.hit_point));
        pos = hit.hit_point;


        Material hit_material = get_material(hit.value);

        vec3 modified_normals = hit.normal;
        //#region water normals
        if (hit.value == WATER /*INTO WATER*/) {
            modified_normals = bump(modified_normals, pos + vec3(time * 0.5, time, 0), 0.05, 1, 2);
        }
        else if (start_value == WATER /*OUT OF WATER*/) {
            modified_normals = bump(modified_normals, pos + vec3(time * 0.5, time, 0), -0.05, 1, 2);
        }
        //#endregion

        bool transparent_ray = hit_material.transparency != 0;
        bool reflection_ray = hit_material.reflection != 0;
        //#region transparency reflection conflict
        if (transparent_ray && refract(direction, modified_normals, start_material.ior / hit_material.ior) == vec3(0)) { // switch to inner refraction
            transparent_ray = false;
            reflection_ray = true;
        }
        else if (transparent_ray && reflection_ray) {
            coord_offset *= 2;
            transparent_ray = int(gl_FragCoord.x+gl_FragCoord.y) % coord_offset < coord_offset / 2;
            reflection_ray = !transparent_ray;
        }
        //#endregion

        //#region compute hit_color
        vec4 hit_color = vec4(hit_material.color, 1);
        if (floor(hit.hit_point - hit.normal * 0.1) == floor(player_target)) hit_color = mix(hit_color, vec4(1, 1, 1, 1), 0.5);

        float light = clamp(-dot(hit.normal, normalize(vec3(-2, -4, -8))), 0.25, 1);
        hit_color.xyz = (hit_color.xyz + hit_material.emision_color * hit_material.emision_strength) * light;
        //#endregion

        if (transparent_ray) {
            final_color += hit_color * (1 - final_color.w) * (1 - hit_material.transparency);

            direction = refract(direction, modified_normals, start_material.ior / hit_material.ior);
            pos -= hit.normal * 0.01;

            start_value = hit.value;
            start_material = hit_material;
        }
        else if (reflection_ray) {
            final_color += hit_color * (1 - final_color.w) * (1 - hit_material.reflection);

            direction = reflect(direction, modified_normals);
            pos += hit.normal * 0.01;
        }
        else {
            final_color += hit_color * (1 - final_color.w);
            break;
        }
    }

    // last unused volume
    final_color += vec4(start_material.volume_color, 1) * (1 - final_color.w);// * smooth_sign(start_material.volume * distance(pos, hit.hit_point));

    // fill empty color with skybox
    vec3 sky_color = vec3(0, 0.75, 1) * 2;
    final_color.xyz = mix(sky_color, final_color.xyz, final_color.w);

    return final_color.xyz;
}






void main()
{
    int fps = int(round(1 / max(1/120, deltatime)));
    if (gl_FragCoord.x < 5 && gl_FragCoord.y < fps * (WindowSize.y / 120)) {
        if (fps >= 60) LFragment = vec4(0, 1, 0, 1.0);
        else if (fps >= 30) LFragment = vec4(1, 1, 0, 1.0);
        else if (fps >= 15) LFragment = vec4(1, 0, 0, 1.0);
        else LFragment = vec4(0, 0, 0, 1.0);

        return;
    }

    vec3 d_times = debug_time / (debug_time.x + debug_time.y + debug_time.z) * WindowSize.y;
    if (gl_FragCoord.x >= 5 && gl_FragCoord.x < 10) {
        if (gl_FragCoord.y < d_times.x) LFragment = vec4(0, 1, 0, 1.0);
        else if (gl_FragCoord.y < d_times.x + d_times.y) LFragment = vec4(1, 0, 1, 1.0);
        else LFragment = vec4(0, 0, 1, 1.0);

        return;
    }

    float x = gl_FragCoord.x - WindowSize.x / 2;
    float y = gl_FragCoord.y - WindowSize.y / 2;

    if (abs(x) <= 1 && abs(y) <= 32 || abs(y) <= 1 && abs(x) <= 32) // cross
        LFragment = vec4(1.0, 1.0, 1.0, 1.0);
    else {
        LFragment = vec4(get_color(), 1.0);
    }
}