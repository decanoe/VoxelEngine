#ifndef _PLAYER_CLASS
#define _PLAYER_CLASS

#include "../utility/math/vector3.h"
#include "../utility/graphics/screen.h"
#include "../world/world.h"

#define BASE_PLAYER_SPEED 8
#define BASE_PLAYER_HEIGHT 1.9

enum Game_Mode { Normal, Cheat };

class Player
{
private:
    Game_Mode mode = Game_Mode::Cheat;

    unsigned int placing = MATERIAL_BUILDING;

    Vector3 velocity = Vector3(0, 0, 0);
    float view_pitch = 0, view_yaw = 0;
    int FOV = 60;
    float speed = BASE_PLAYER_SPEED;
    float player_height = BASE_PLAYER_HEIGHT;
    Screen* screen;
    World* world;

    void break_block();
    void place_block();
    void reset_cursor();
    
    void try_movement(Vector3 movement);
public:
    Vector3 position = Vector3(0, 0, 0);

    Player(Vector3 position, Screen* screen, World* world);
    void dispose();

    void send_data();
    void process_events(float deltatime);
    void process_specific_event(SDL_Event event, float deltatime);
    void update(float deltatime);

    Vector3 get_direction();
};


#endif