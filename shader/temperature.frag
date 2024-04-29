#version 430 core

out vec4 LFragment;

struct TempPoint {
    float x;
    float y;
    float temperature;
};
layout(std430, binding = 3) readonly buffer world_data_layout
{
    TempPoint world_data[];
};
uniform int WorldCount;

in vec4 gl_FragCoord;

uniform vec2 WindowSize;
uniform float time; // within [0, 1000] in seconds

float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}
float gaussian(float v) {
    return exp(-v*v);
}

void main()
{
    float t = 0;
    for (int i = 0; i < WorldCount; i++) {
        vec2 pos = vec2(world_data[i].x, world_data[i].y);
        if (distance(pos, gl_FragCoord.xy) > 300) continue;
        t += world_data[i].temperature / max(distance(pos, gl_FragCoord.xy), 0.1);
    }
    
    
    float r = 1 - amplify_01(1 - t);
    float b = 1 - amplify_01(amplify_01(1 - t));
    float g = t;
    LFragment = vec4(r, b, g, 1.0);
}