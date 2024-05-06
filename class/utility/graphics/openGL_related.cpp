#ifndef _OPENGL_RELATED

#include "./openGL_related.h"

void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

void init_OpenGL() {
    // Initialize GLEW
    glewExperimental = GL_TRUE; // Enable GLEW experimental features
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(glewError) << std::endl;
        exit(1);
    }
    // Use vsync
    if (SDL_GL_SetSwapInterval(-1) < 0) {
        std::cerr << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
}

// handling for program log
void printProgramLog(GLuint program) {
    if (glIsProgram(program)) {

        int infoLogLength = 0;
        int maxLength = infoLogLength;

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        char* infoLog = new char[sizeof(char) * maxLength];

        glGetProgramInfoLog(program, maxLength, &infoLogLength, infoLog);
        if (infoLogLength > 0) {
        printf("%s\n", infoLog);
        }

        free(infoLog);
    }
    else {
        printf( "Name %d is not a program\n", program );
    }
}
// handling for shader log
void printShaderLog(GLuint shader) {
    //Make sure name is shader
    if(glIsShader( shader )) {
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;

        //Get info string length
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        //Allocate string
        char* infoLog = new char[sizeof(char) * maxLength];

        //Get info log
        glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
        if(infoLogLength > 0) {
            printf( "%s\n", infoLog );
        }

        //Deallocate string
        free(infoLog);
    }
    else {
        printf("Name %d is not a shader\n", shader);
    }
}
bool initGL(GLuint *gProgramID, GLint *gVertexPos2DLocation, GLuint *gVBO, GLuint *gIBO, std::string& vertex_shader_source, std::string& fragment_shader_source, const GLchar* render_shader) {
    *gProgramID = glCreateProgram();

    if (!set_vertex_shader(gProgramID, "./shader/vertex.vert", vertex_shader_source)) {
        return false;
    }

    if (!set_fragment_shader(gProgramID, render_shader, fragment_shader_source)) {
        return false;
    }

    glLinkProgram(*gProgramID);

    // check if program is valid 
    GLint programSuccess = GL_TRUE;
    glGetProgramiv(*gProgramID, GL_LINK_STATUS, &programSuccess);
    if (programSuccess != GL_TRUE) {
        printf("Error linking program %d!\n", *gProgramID);
        printProgramLog(*gProgramID);
        return false;
    }
    
    *gVertexPos2DLocation = glGetAttribLocation(*gProgramID, "LVertexPos2D");

    if (*gVertexPos2DLocation == -1) {
        printf("LVertexPos2D is not a valid glsl program variable!\n");
        return false;
    }
    glClearColor(0.f, 0.f, 0.f, 1.f);


    set_quad_buffer(gProgramID, gVertexPos2DLocation, gVBO, gIBO);
    
    return true;
}

std::string get_file_text(std::string path) {
    std::ifstream file = std::ifstream(path);

    if (!file.is_open())
    {
        std::cerr << "Cannot open the file at " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string content((std::istreambuf_iterator<char>(file)   ),
        (std::istreambuf_iterator<char>()       ) );
    
    file.close();
    
    // make the string NUL-terminated, by putting a 0-Byte at the end of it
    content += "\0";
    return content;
}
bool set_vertex_shader(GLuint *gProgramID, std::string path, std::string& vertex_shader_source) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    vertex_shader_source = get_file_text(path);
    const GLchar* shader_source = vertex_shader_source.c_str();

    glShaderSource(vertexShader, 1, &shader_source, NULL);
    glCompileShader(vertexShader);

    GLint vShaderCompiled = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);

    if (vShaderCompiled != GL_TRUE) {
        printf("Unable to compile vertex shader %d", &vShaderCompiled);
        printShaderLog(vertexShader);

        return false;
    }
    else {
        glAttachShader(*gProgramID, vertexShader);

        return true;
    }
}
bool set_fragment_shader(GLuint *gProgramID, std::string path, std::string& fragment_shader_source) {
    GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

    fragment_shader_source = get_file_text(path);
    const GLchar* shader_source = fragment_shader_source.c_str();

    glShaderSource(fragmentShader, 1, &shader_source, NULL);
    glCompileShader(fragmentShader);

    GLint fShaderCompiled = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
    if(fShaderCompiled != GL_TRUE) {
        printf("Unable to compile fragment shader %d!\n", fragmentShader);
        printShaderLog(fragmentShader);
        return false;
    }
    else {
        glAttachShader(*gProgramID, fragmentShader);

        return true;
    }
}
void set_quad_buffer(GLuint *gProgramID, GLint *gVertexPos2DLocation, GLuint *gVBO, GLuint *gIBO) {
    GLfloat vertexData[] = {
        -1, -1,
        1, -1,
        1, 1,
        -1, 1
    };

    GLuint indexData[] = { 0, 1, 2, 3 };

    glGenBuffers(1, gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, *gVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

    glGenBuffers(1, gIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *gIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);
}

#endif