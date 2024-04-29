#ifndef _SCREEN_CLASS
#define _SCREEN_CLASS

#include "./openGL_related.h"
#include "../math/vector3.h"

#include <iostream>
#include <chrono>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>


class Screen
{
private:
    SDL_Window *window = NULL;
    SDL_GLContext context = nullptr;

    GLuint gProgramID = 0;
    GLint gVertexPos2DLocation = -1;
    GLuint gVBO = 0;
    GLuint gIBO = 0;

    std::string vertex_shader_source = "";
    std::string fragment_shader_source = "";
public:
    int width = 1080;
    int height = 768;

    Screen(unsigned int width, unsigned int height);
    void close();
    void setup_base_shaders();

    SDL_Window* get_window();

    void render();
    void update();

    bool set_uniform(const GLchar* name, float value);
    bool set_uniform(const GLchar* name, float x, float y);
    bool set_uniform(const GLchar* name, float x, float y, float z);
    bool set_uniform(const GLchar* name, Vector3 value);
    bool set_uniform(const GLchar* name, int value);
    bool set_uniform(const GLchar* name, unsigned int value);
};

#endif