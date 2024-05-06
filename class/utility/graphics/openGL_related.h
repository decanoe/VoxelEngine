#ifndef _OPENGL_RELATED
#define _OPENGL_RELATED

#include <iostream>
#include <fstream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

void init_OpenGL();

// handling for program log
void printProgramLog(GLuint program);
// handling for shader log
void printShaderLog(GLuint shader);
bool initGL(GLuint *gProgramID, GLint *gVertexPos2DLocation, GLuint *gVBO, GLuint *gIBO, std::string& vertex_shader_source, std::string& fragment_shader_source, const GLchar* render_shader);

bool set_vertex_shader(GLuint *gProgramID, std::string path, std::string& vertex_shader_source);
bool set_fragment_shader(GLuint *gProgramID, std::string path, std::string& fragment_shader_source);
void set_quad_buffer(GLuint *gProgramID, GLint *gVertexPos2DLocation, GLuint *gVBO, GLuint *gIBO);

#endif