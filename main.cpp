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

float get_time_from(std::chrono::_V2::system_clock::time_point point) {
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_time = end-point;
    double elapsed_seconds = elapsed_time.count();
    return elapsed_seconds;
}

int main(int argc, char *args[]) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    Screen screen = Screen(SCREEN_WIDTH, SCREEN_HEIGHT);
    screen.setup_base_shaders("./shader/test.frag");
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
        auto frame_start = std::chrono::system_clock::now();

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
                    case SDLK_F12:
                        screen.set_full_screen(!screen.is_full_screen());
                        break;
                }
                break;
            }

            player.process_specific_event(e, deltatime);
        }
        
        auto world_start = std::chrono::system_clock::now();
        world.update(0);
        float world_time = get_time_from(world_start);
        
        auto player_start = std::chrono::system_clock::now();
        player.process_events(deltatime);
        player.update(deltatime);
        float player_time = get_time_from(player_start);
    
        auto render_start = std::chrono::system_clock::now();
        screen.render();
        float render_time = get_time_from(player_start);

        screen.update();
        screen.set_uniform("debug_time", world_time, player_time, render_time);

        auto frame_end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_time = frame_end-frame_start;
        double elapsed_seconds = elapsed_time.count();
        deltatime = elapsed_seconds;
        // std::cout << "time to render frame : " << elapsed_seconds << " (fps : " << 1/elapsed_seconds << ")\n";
        
        screen.set_uniform("deltatime", deltatime);
        // std::cout << "time to render frame : " << elapsed_seconds << " (fps : " << 1/elapsed_seconds << ")\n";
    }

    world.dispose();

    screen.close();

    SDL_Quit();
    return 0;
}