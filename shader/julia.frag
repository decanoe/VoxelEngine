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
    vec2 c = vec2(cos(2.873 + (time / 50) * 2 * 3.1415), sin(2.873 + (time / 50) * 2 * 3.1415));
    float size = min(WindowSize.x, WindowSize.y);
    vec2 value = vec2(gl_FragCoord.x - WindowSize.x / 2, gl_FragCoord.y - WindowSize.y / 2);
    value *= 1.5 * 2;
    value /= size;

    float color = 0;

    int MAX_ITER = 100;
    while (color < MAX_ITER && value.x * value.x + value.y * value.y < 100*100)
    {
        vec2 next_val;
        next_val.x = value.x * value.x - value.y * value.y + c.x;
        next_val.y = 2 * value.x * value.y + c.y;

        value = next_val;
        color += 1;
    }

    LFragment = vec4(0.0, amplify_01(color / 100.0), amplify_01(color / 100.0), 1.0);
}