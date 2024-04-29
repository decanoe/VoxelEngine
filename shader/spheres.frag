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

mat3 rotation_matrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c          );
}

uniform vec3 player_position;
uniform float facing_pitch;
uniform float facing_yaw;
uniform float FOV;
void main()
{
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
    direction = rotation_matrix(up, FOV * -x) * direction;
    vec3 right = cross(up, direction);
    direction = rotation_matrix(right, FOV * y) * direction;

    vec3 c = vec3(0, 0.1, 0.5);

    vec3 spheres[] = vec3[](
        vec3(5, 0, 0),
        vec3(0, 5, 3),
        vec3(-6, 2, 0)
    );

    vec3 pos = player_position;// + (up * y + right * x) * 10;
    c = normalize(pos);
    uint max_iter = 256;
    for (int i = 0; i < max_iter; i++) {
        pos += direction * 0.2;

        bool hit = false;
        for (int index = 0; index < 3; index++) {
            if (distance(pos, spheres[index]) < 4) {
                c = normalize(pos) * -dot(normalize(pos - spheres[index]), normalize(vec3(-2, -3, -10)));
                hit = true;
                break;
            }
        }

        if (hit) break;
    }

    LFragment = vec4(c.x, c.y, c.z, 1.0);
}