#ifndef _PLAYER_CLASS

#include "./player.h"

Player::Player(Vector3 position, Screen* screen, World* world) {
    this->position = position;
    this->screen = screen;
    this->world = world;

    this->send_data();
}
void Player::dispose() {}

Vector3 Player::get_direction() {
    return Vector3(
        cos(this->view_pitch) * cos(this->view_yaw),
        cos(this->view_pitch) * sin(this->view_yaw),
        sin(this->view_pitch)
        ).normalized();
}
void Player::send_data() {
    this->screen->set_uniform("player_position", this->position + Vector3(0, 0, this->player_height));
    this->screen->set_uniform("facing_pitch", this->view_pitch);
    this->screen->set_uniform("facing_yaw", this->view_yaw);
    this->screen->set_uniform("FOV", (this->FOV * 3.1412f) / 180.0f);
}
void Player::try_movement(Vector3 movement) {
    if (this->mode == Game_Mode::Cheat) {
        this->position += movement;
        this->screen->set_uniform("player_position", this->position + Vector3(0, 0, this->player_height));
        this->reset_cursor();
        return;
    }

    float movement_size = movement.magnitude();
    movement /= movement_size;

    for (int i = 0; i < 8; i++)
    {
        RaycastHit hit = this->world->raycast(this->position + Vector3(0, 0, 0.01) + movement * 0.01, movement, movement_size);
        if (hit.distance > 0.01) this->position += movement * hit.distance + hit.normal * 0.01;

        movement_size -= hit.distance;
        movement += hit.normal * (movement.dot(hit.normal));
        movement.normalize();
    }

    this->screen->set_uniform("player_position", this->position + Vector3(0, 0, this->player_height));
    this->reset_cursor();
}
void Player::process_events(float deltatime) {
    Vector3 forward = Vector3(cos(this->view_yaw), sin(this->view_yaw), 0);
    Vector3 right = Vector3(cos(this->view_yaw + 3.1412 / 2), sin(this->view_yaw + 3.1412 / 2), 0);

    Vector3 movement = Vector3(0, 0, 0);
    bool change_pos_needed = false;

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_W]) movement += forward;
    if (keystate[SDL_SCANCODE_S]) movement -= forward;
    if (keystate[SDL_SCANCODE_D]) movement += right;
    if (keystate[SDL_SCANCODE_A]) movement -= right;
    if (keystate[SDL_SCANCODE_SPACE]) {
        if (this->mode == Game_Mode::Cheat) movement += Vector3(0, 0, 1);
        else {
            RaycastHit hit = world->raycast_down(this->position + Vector3(0, 0, 0.1), 0.2);
            if (hit.has_hit && hit.hit_point.z == this->position.z) {
                this->velocity.z = 7.5;
            }
        }
    }
    if (keystate[SDL_SCANCODE_LSHIFT]) {
        if (this->mode == Game_Mode::Cheat) movement -= Vector3(0, 0, 1);

        player_height = BASE_PLAYER_HEIGHT * 0.75;
        change_pos_needed = true;
    }
    else if (player_height != BASE_PLAYER_HEIGHT) {
        player_height = BASE_PLAYER_HEIGHT;
        change_pos_needed = true;
    }

    if (keystate[SDL_SCANCODE_KP_PLUS]) {
        this->FOV += 1;
        this->screen->set_uniform("FOV", (this->FOV * 3.1412f) / 180.0f);
    }
    if (keystate[SDL_SCANCODE_KP_MINUS]) {
        this->FOV -= 1;
        this->screen->set_uniform("FOV", (this->FOV * 3.1412f) / 180.0f);
    }
    
    if (movement.sqrmagnitude() != 0) {
        movement.normalize();

        this->try_movement(movement * this->speed * deltatime);
    }
    else if (change_pos_needed)
        this->screen->set_uniform("player_position", this->position + Vector3(0, 0, this->player_height));
    

    const Uint32 mousestate = SDL_GetMouseState(NULL, NULL);
    if (mousestate & SDL_BUTTON(SDL_BUTTON_LEFT)) break_block();
    if (mousestate & SDL_BUTTON(SDL_BUTTON_RIGHT)) place_block();
}

void Player::break_block() {
    RaycastHit hit = world->raycast(this->position + Vector3(0, 0, this->player_height), this->get_direction(), 500);
    if (!hit.has_hit) return;
    world->set((hit.hit_point - hit.normal * 0.1).floor(), MATERIAL_AIR);
}
void Player::place_block() {
    RaycastHit hit = world->raycast(this->position + Vector3(0, 0, this->player_height), this->get_direction(), 500);
    if (!hit.has_hit) return;
    world->set((hit.hit_point + hit.normal * 0.1).floor(), this->placing);
}
void Player::reset_cursor() {
    RaycastHit hit = world->raycast(this->position + Vector3(0, 0, this->player_height), this->get_direction(), 500);
    
    if (hit.has_hit) this->screen->set_uniform("player_target", hit.hit_point - hit.normal * 0.1);
    else this->screen->set_uniform("player_target", Vector3(0, 0, 0));
}

void Player::process_specific_event(SDL_Event event, float deltatime) {
    switch (event.type)
    {
        case SDL_MOUSEMOTION:
        {
            float x_move = event.motion.xrel;
            float y_move = event.motion.yrel;

            this->view_pitch = __max(-1.5708, __min(1.5708, this->view_pitch - y_move * this->FOV / 40000));
            this->view_yaw += x_move * FOV / 40000;

            this->screen->set_uniform("facing_pitch", this->view_pitch);
            this->screen->set_uniform("facing_yaw", this->view_yaw);

            this->reset_cursor();
        }
        break;
        
        case SDL_MOUSEBUTTONUP:
        {
            switch (event.button.button)
            {
            // case SDL_BUTTON_LEFT:
            //     break_block();
            //     break;
            // case SDL_BUTTON_RIGHT:
            //     place_block();
            //     break;
            }
        }
        break;

        case SDL_MOUSEWHEEL:
        {
            this->speed *= powf(1.05, event.wheel.y);
            this->speed = __max(this->speed, 0.001);
            this->speed = __min(this->speed, 128);
        }
        break;

        case SDL_KEYDOWN:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_F4:
                if (this->mode == Game_Mode::Normal) this->mode = Game_Mode::Cheat;
                else if (this->mode == Game_Mode::Cheat) this->mode = Game_Mode::Normal;
                break;
            
            
            case SDLK_1: this->placing = MATERIAL_GRASS; break;
            case SDLK_2: this->placing = MATERIAL_DIRT; break;
            case SDLK_3: this->placing = MATERIAL_STONE; break;
            case SDLK_4: this->placing = MATERIAL_WATER; break;
            case SDLK_5: this->placing = MATERIAL_BUILDING; break;
            case SDLK_6: this->placing = MATERIAL_LIGHT; break;
            }
        }
        break;
    }
}
void Player::update(float deltatime) {
    if (this->mode == Game_Mode::Cheat) {
        this->velocity.z = 0;
    }
    else {
        this->velocity.z -= 9.81 * deltatime * 2;

        RaycastHit hit = world->raycast_down(this->position + Vector3(0, 0, 0.1), this->velocity.magnitude() * deltatime + 0.5);
        if (hit.has_hit && hit.hit_point.z == this->position.z && this->velocity.z < 0) {
            this->velocity.z = 0;
        }
        else {
            this->position += this->velocity * deltatime;
            if (hit.has_hit) this->position.z = __max(this->position.z, hit.hit_point.z);
            this->screen->set_uniform("player_position", this->position + Vector3(0, 0, this->player_height));
        }
    }
}

#endif