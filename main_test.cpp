#include <iostream>
#include <chrono>
#include <cmath>
// #define SDL_MAIN_HANDLED
// #include <GL/glew.h>
// #include "class/utility/graphics/screen.h"
// #include "class/utility/graphics/buffer.h"
// #include "class/world/world_generator.h"
// #include "class/world/world.h"
// #include "class/gameplay/player.h"

#define dist 5

int main(int argc, char *args[]) {

    for (int x = -dist; x <= dist; x++)
    {
        int new_x = x - (2*dist+1) * floorf((float)x / (2*dist+1));
        std::cout << x << " => " << new_x << "\n";
        // std::cout << x << " => " << (x % (dist * 2 + 1)) << "\n";
    }

    return 0;
}