#include <iostream>
#include <chrono>
#include <cmath>
#define SDL_MAIN_HANDLED
#include <GL/glew.h>
#include "class/utility/graphics/screen.h"
#include "class/utility/graphics/buffer.h"
#include "class/world/world_generator.h"
#include "class/world/world.h"
#include "class/gameplay/player.h"

const int SCREEN_WIDTH = 1080;
const int SCREEN_HEIGHT = 768;

#define WORLD_DATA_BUFFER_BINDING 1
#define WORLD_INDEX_BUFFER_BINDING 2

#define PLAYER_SPEED 8
#define LOADING_RADIUS 5

int main(int argc, char *args[]) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    Screen screen = Screen(SCREEN_WIDTH, SCREEN_HEIGHT);
    screen.setup_base_shaders();
    SDL_SetRelativeMouseMode(SDL_TRUE);

    WorldGenerator generator = WorldGenerator(1);
    World world = World(LOADING_RADIUS, &generator);
    #ifndef DISABLE_BUFFER
    world.create_buffer(WORLD_DATA_BUFFER_BINDING, WORLD_INDEX_BUFFER_BINDING);
    #endif
    world.send_data();

    Player player = Player(
        Vector3(
            0,
            0,
            CHUNK_WIDTH + 0.01
        )
        , &screen, &world);

    bool loop = true;
    float deltatime = 0.001;
    while (loop) {
        auto start = std::chrono::system_clock::now();

        // std::cout << "Events - ";
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type)
            {
            case SDL_QUIT:
                loop = false;
                break;

            case SDL_KEYDOWN:
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        loop = false;
                        break;
                    case SDLK_F5:
                        world.send_data();
                        break;
                }
                break;
            }

            player.process_specific_event(e, deltatime);
        }
        
        // std::cout << "world - ";
        world.update(0);
        
        // std::cout << "player events - ";
        player.process_events(deltatime);
        // std::cout << "player physics - ";
        player.update(deltatime);
    
        // std::cout << "render - ";
        screen.render();
        // std::cout << "update - ";
        screen.update();
        // std::cout << "end of frame\n";

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = end-start;
        double elapsed_seconds = elapsed_time.count();
        deltatime = elapsed_seconds;
        // std::cout << "time to render frame : " << elapsed_seconds << " (fps : " << 1/elapsed_seconds << ")\n";
    }

    world.dispose();

    screen.close();

    SDL_Quit();
    return 0;
}