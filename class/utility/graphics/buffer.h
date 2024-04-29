#ifndef _BUFFER_CLASS
#define _BUFFER_CLASS

#include <iostream>
#include <fstream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

class Buffer
{
private:
protected:
    bool initialized = false;
    GLuint buffer_id = 0;
    unsigned int buffer_size = 0;
    GLuint binding = 0;
public:
    Buffer(bool initialize = false);
    void dispose();
    bool is_buffer();

    void set_data(GLsizeiptr size, const void *data);
    void set_data(GLsizeiptr offset, GLsizeiptr size, const void *data);

    void bind_buffer(GLuint index);

    unsigned int get_used_size();
};

class GrowableBuffer: public Buffer
{
private:
    unsigned int buffer_added_storage = 0;
    unsigned int used_storage = 0;
public:
    GrowableBuffer(bool initialize = false, unsigned int added_storage = sizeof(unsigned int), unsigned int start_size = 0);

    void set_data(GLsizeiptr size, const void *data);
    void set_data(GLsizeiptr offset, GLsizeiptr size, const void *data);
    unsigned int push_data(GLsizeiptr size, const void *data);
    void replace_data(GLsizeiptr offset, GLsizeiptr old_size, GLsizeiptr new_size, const void *data);
    
    unsigned int get_used_size();
};

#endif