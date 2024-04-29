#version 330 core

out vec4 LFragment;

in vec4 gl_FragCoord;

uniform vec2 WindowSize;
uniform float time; // within [0, 1000] in seconds

float amplify_01(float v) {
    return 1 - (1 - v)*(1 - v)*(1 - v);
}

void main()
{
    float size = min(WindowSize.x, WindowSize.y) - 10;
    vec2 value = vec2(gl_FragCoord.x - WindowSize.x / 2, gl_FragCoord.y - WindowSize.y / 2);
    value /= size;
    value += 0.5;

    for (int i = 0; i < 6; i++) {
        if (value.x < 0 || value.x > 1 || value.y < 0 || value.y > 1) {
            value = vec2(0, 0);
            break;
        }
        if (value.x * 3 > 1 && value.x * 3 < 2 && value.y * 3 > 1 && value.y * 3 < 2) {
            value = vec2(0, 0);
            break;
        }

        if (value.x * 3 < 1) value.x = clamp(value.x * 3, 0, 1);
        else if (value.x * 3 > 2) value.x = clamp(1 - (1 - value.x) * 3, 0, 1);
        else value.x = clamp(value.x * 3 - 1, 0, 1);

        if (value.y * 3 < 1) value.y = clamp(value.y * 3, 0, 1);
        else if (value.y * 3 > 2) value.y = clamp(1 - (1 - value.y) * 3, 0, 1);
        else value.y = clamp(value.y * 3 - 1, 0, 1);
    }

    LFragment = vec4(value.x != 0 ? (cos((time / 15.0) * 2 * 3.1415)+1)/2:0, 0.0, value.y != 0 ? (sin((time / 15.0) * 2 * 3.1415)+1)/2:0, 1.0);
}