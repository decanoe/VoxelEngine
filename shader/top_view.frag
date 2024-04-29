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
        case 1: // air
            return vec3(0, 0.5, 0.5);
        case 2: // grass
            return vec3(0, 0.5, 0);
        case 100: // temp
            return vec3(0.5, 0.5, 0.5);
        default: // unknown
            return vec3(1, 0, 1);
    }
}
struct CellInfo {
    Cell cell;
    uint width;
};
CellInfo get_cell_info(uint x, uint y, uint z) {
    uint chunk_x = uint(x / chunk_width);
    uint chunk_y = uint(y / chunk_width);
    x -= chunk_x * chunk_width;
    y -= chunk_y * chunk_width;

    uint chunk_index = world_indexes[chunk_x + uint(sqrt(world_indexes_count)) * chunk_y];
    uint cell_offset = 0;

    uint current_cell_width = chunk_width;
    while (world_data[chunk_index + cell_offset].sub_cell[0] != 0 && current_cell_width > 1) {
        current_cell_width >>= 1;
        uint code = (uint(x / current_cell_width) << 2) + (uint(y / current_cell_width) << 1) + uint(z / current_cell_width);
        cell_offset = world_data[chunk_index + cell_offset].sub_cell[code];

        x %= current_cell_width;
        y %= current_cell_width;
        z %= current_cell_width;
    }

    Cell cell = world_data[chunk_index + cell_offset];
    return CellInfo(cell, current_cell_width);
}

float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}
float gaussian(float v) {
    return exp(-v*v);
}

uniform uint max_nb_cell_per_chunk;
void main()
{
    float x = gl_FragCoord.x / WindowSize.x;
    float y = gl_FragCoord.y / WindowSize.y;

    vec3 c = vec3(0, 0, 0);

    uint side = uint(sqrt(world_indexes_count));
    x = (gl_FragCoord.x - WindowSize.x / 2);
    y = (gl_FragCoord.y - WindowSize.y / 2);
    uint min_cell_width = 4;
    uint graphic_square_size = chunk_width * side * min_cell_width;

    if (abs(x) < graphic_square_size / 2 && abs(y) < graphic_square_size / 2) {
        x += graphic_square_size / 2;
        y += graphic_square_size / 2;

        CellInfo cellinfo = get_cell_info(uint(x / min_cell_width), uint(y / min_cell_width), 0);
        uint last_cell_width = min_cell_width * cellinfo.width;
        uint x_in_cell = uint(x) % last_cell_width;
        uint y_in_cell = uint(y) % last_cell_width;

        if (x_in_cell == 0 || x_in_cell == last_cell_width || y_in_cell == 0 || y_in_cell == last_cell_width) {
            c = vec3(1, 1, 1);
        }
        else {
            c = cell_color(cellinfo.cell);
        }
    }

    LFragment = vec4(c.x, c.y, c.z, 1.0);
}