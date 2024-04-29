#version 430 core

out vec4 LFragment;
in vec4 gl_FragCoord;

uniform vec2 WindowSize;
uniform float time; // within [0, 1000] in seconds


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
    uint world_indexes_count;
    uint chunk_width;
    uint world_indexes[];
};

vec3 cell_color(Cell cell) {
    switch (cell.value) {
        case 0: // air
            return vec3(0, 0.5, 0.5);
        case 1: // ground
            return vec3(0, 0.5, 0);
        default: // unknown
            return vec3(1, 0, 1);
    }
}
struct CellInfo {
    vec3 index;
    Cell cell;
    uint width;
    vec3 hit_point;
    vec3 normal;
};
CellInfo get_cell_info(vec3 index) {
    uint chunk_x = uint(index.x / chunk_width);
    uint chunk_y = uint(index.y / chunk_width);
    uint x = uint(index.x) - chunk_x * chunk_width;
    uint y = uint(index.y) - chunk_y * chunk_width;
    uint z = uint(index.z);

    uint chunk_index = world_indexes[chunk_x + uint(sqrt(world_indexes_count)) * chunk_y];
    uint cell_offset = 0;

    uint current_cell_width = chunk_width;
    while (world_data[chunk_index + cell_offset].sub_cell[0] != 0 && current_cell_width > 1) {
        current_cell_width >>= 1;
        uint code = uint(x / current_cell_width) + (uint(y / current_cell_width) << 1) + (uint(z / current_cell_width) << 2);
        cell_offset = world_data[chunk_index + cell_offset].sub_cell[code];

        x -= uint(x / current_cell_width) * current_cell_width;
        y -= uint(y / current_cell_width) * current_cell_width;
        z -= uint(z / current_cell_width) * current_cell_width;
    }

    Cell cell = world_data[chunk_index + cell_offset];
    return CellInfo(index, cell, current_cell_width, vec3(0, 0, 0), vec3(0, 1, 0));
}

float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}
float gaussian(float v) {
    return exp(-v*v);
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
    float x = (gl_FragCoord.x - WindowSize.x / 2) / (WindowSize.x / 2);
    float y = (gl_FragCoord.y - WindowSize.y / 2) / (WindowSize.x / 2);

    vec3 direction = normalize(vec3(
        cos(facing_pitch) * cos(facing_yaw),
        cos(facing_pitch) * sin(facing_yaw),
        sin(facing_pitch)
        ));
    vec3 up = normalize(vec3(
        cos(facing_pitch + (3.1412 / 2)) * cos(facing_yaw),
        cos(facing_pitch + (3.1412 / 2)) * sin(facing_yaw),
        sin(facing_pitch + (3.1412 / 2))
        ));
    direction = rotation_matrix(up, (FOV / 2) * -x) * direction;
    vec3 right = cross(up, direction);
    direction = rotation_matrix(right, (FOV / 2) * y) * direction;

    return direction;
}
CellInfo get_next_cell(uint cell_size, vec3 position, vec3 cell_index, vec3 direction) {
    vec3 pos_in_cell = fract(position / cell_size); // in [0, 1]
    
    float step_size = 1;
    vec3 cell_jump = vec3(sign(direction.x), 0, 0);
    if (direction.x > 0) {
        step_size = (1 - pos_in_cell.x) / direction.x;
    }
    else if (direction.x < 0) {
        step_size = -pos_in_cell.x / direction.x;
    }
    
    float temp_step = 1;
    if (direction.y > 0) temp_step = (1 - pos_in_cell.y) / direction.y;
    else if (direction.y < 0) temp_step = -pos_in_cell.y / direction.y;
    if (temp_step < step_size) {
        step_size = temp_step;
        cell_jump = vec3(0, sign(direction.y), 0);
    }
    
    temp_step = 1;
    if (direction.z > 0) temp_step = (1 - pos_in_cell.z) / direction.z;
    else if (direction.z < 0) temp_step = -pos_in_cell.z / direction.z;
    if (temp_step < step_size) {
        step_size = temp_step;
        cell_jump = vec3(0, 0, sign(direction.z));
    }

    cell_index += cell_jump * cell_size;

    CellInfo result = get_cell_info(cell_index);
    result.hit_point = position + direction * step_size * cell_size;
    result.normal = -cell_jump;
    return result;
}

uniform vec3 player_position;
void main()
{
    vec3 direction = get_direction();

    vec3 c = vec3(1, 0, 1);

    uint world_side = uint(sqrt(world_indexes_count));

    vec3 pos = player_position;
    uint max_iter = 256;
    for (int i = 0; i < max_iter; i++) {
        pos += direction * 0.05;

        if (pos.x < 0 || pos.y < 0 || pos.z < 0 || pos.x > chunk_width * world_side || pos.y > chunk_width * world_side || pos.z > chunk_width) {
            continue;
        }

        CellInfo cellinfo = get_cell_info(pos);

        if (cellinfo.cell.value < 2) {
            vec3 color = cell_color(cellinfo.cell);
            float dist = float(i) / (max_iter - 1);
            c = color * (1 - dist);
            break;
        }
    }

    LFragment = vec4(c.x, c.y, c.z, 1.0);
}