#ifndef _SCREEN_CLASS

#include "./screen.h"

Screen::Screen(unsigned int width, unsigned int height)
{
    this->width = width;
    this->height = height;

    // init sdl if it was not
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL cannot be initialed: " << SDL_GetError() << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    this->window = SDL_CreateWindow(
        "Voxel Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        std::cerr << "SDL cannot create window: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Screen::setup_base_shaders(const GLchar* render_shader) {
    this->context = SDL_GL_CreateContext(window);
    if (!context) {
        std::cerr << "SDL OpenGL context creation failed: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }
    init_OpenGL();

    //Initialize OpenGL
    if (!initGL(&this->gProgramID, &this->gVertexPos2DLocation, &this->gVBO, &this->gIBO, this->vertex_shader_source, this->fragment_shader_source, render_shader)) {
        std::cerr << "Unable to initialize OpenGL!" << std::endl;
        exit(EXIT_FAILURE);
    }

    this->set_uniform("WindowSize", this->width, this->height);
    
    // std::cout << this->vertex_shader_source << "\n\n" << this->fragment_shader_source << "\n";
}
bool Screen::set_uniform(const GLchar* name, float value) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform1f(uniform, value);
    glUseProgram(0);
    return true;
}
bool Screen::set_uniform(const GLchar* name, float x, float y) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform2f(uniform, x, y);
    glUseProgram(0);
    return true;
}
bool Screen::set_uniform(const GLchar* name, float x, float y, float z) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform3f(uniform, x, y, z);
    glUseProgram(0);
    return true;
}
bool Screen::set_uniform(const GLchar* name, Vector3 value) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform3f(uniform, value.x, value.y, value.z);
    glUseProgram(0);
    return true;
}
bool Screen::set_uniform(const GLchar* name, int value) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform1i(uniform, value);
    glUseProgram(0);
    return true;
}
bool Screen::set_uniform(const GLchar* name, unsigned int value) {
    int uniform = glGetUniformLocation(this->gProgramID, name);
    if (uniform == -1) return false;
    glUseProgram(this->gProgramID);
    glUniform1ui(uniform, value);
    glUseProgram(0);
    return true;
}

void Screen::close() {
    //Deallocate program
    glDeleteProgram(this->gProgramID);

    //Destroy window  
    SDL_DestroyWindow(this->window);
    window = NULL;

    //Quit SDL subsystems
    SDL_Quit();
}

SDL_Window* Screen::get_window() {
    return this->window;
}

uint64_t get_time() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
void Screen::render() {
    //Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // set time variable
    float time = (get_time() % (1000*1000*1000)) / (1000.0 * 1000.0);
    this->set_uniform("time", time);
    
    //Bind program
    glUseProgram(this->gProgramID);

    //Enable vertex position
    glEnableVertexAttribArray(this->gVertexPos2DLocation);

    //Set vertex data
    glBindBuffer(GL_ARRAY_BUFFER, this->gVBO);
    glVertexAttribPointer(this->gVertexPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

    //Set index data and render
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->gIBO);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

    //Disable vertex position
    glDisableVertexAttribArray(this->gVertexPos2DLocation);

    //Unbind program
    glUseProgram(0);
}
void Screen::update() {
    SDL_GL_SwapWindow(this->window);
}


bool Screen::is_full_screen() {
    return this->full_screen;
}
void Screen::set_full_screen(bool full) {
    this->full_screen = full;
    SDL_SetWindowFullscreen(this->window, full ? SDL_WINDOW_FULLSCREEN : 0);
}

#endif