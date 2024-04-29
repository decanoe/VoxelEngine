#ifndef _WORLD_CLASS
#define FIRST_CHUNK_INDEX 2

#include "./world.h"

#define __pow2(x) ((x)*(x))
#define __pow3(x) ((x)*(x)*(x))


#pragma region World
World::World(unsigned int loading_radius, WorldGenerator* generator) {
    this->world_center = Vector3Int(0, 0, 0);
    this->loading_radius = loading_radius;

    this->generator = generator;
    
    #ifndef DISABLE_BUFFER
    this->GPU_root_indexes = new unsigned int[2 + __pow3(this->loading_radius * 2 + 1)];
    this->GPU_root_indexes[0] = this->loading_radius * 2 + 1; // world width
    this->GPU_root_indexes[1] = CHUNK_WIDTH; // chunk width
    #endif

    this->chunks = new Chunk**[this->loading_radius * 2 + 1];
    int i = FIRST_CHUNK_INDEX;
    for (int x = 0; x < this->loading_radius * 2 + 1; x++)
    {
        this->chunks[x] = new Chunk*[this->loading_radius * 2 + 1];
        for (int y = 0; y < this->loading_radius * 2 + 1; y++) {
            this->chunks[x][y] = new Chunk[this->loading_radius * 2 + 1];
            for (int z = 0; z < this->loading_radius * 2 + 1; z++) {
                this->chunks[x][y][z].world = this;
                #ifndef DISABLE_BUFFER
                this->chunks[x][y][z].last_GPU_size = 0;
                this->chunks[x][y][z].GPU_index = i;
                this->GPU_root_indexes[i] = 0;
                #endif
                i += 1;
            }
        }
    }

    #ifndef DISABLE_THREAD
    this->task_queue = std::queue<GenerationTask>();
    #endif
    this->last_radius_loaded = -1;
}
void World::load_circle(int radius) {
    radius = __min(radius, this->loading_radius);

    for (int ring_radius = this->last_radius_loaded + 1; ring_radius <= radius; ring_radius++)
    {
        int nb_generated = 0;

        // left and right (x:=forward)
        for (int x = -ring_radius; x <= ring_radius; x++) {
        for (int z = -ring_radius; z <= ring_radius; z++) {
            this->regenerate_chunk(Vector3Int(x, -ring_radius, z) + world_center);
            if (ring_radius != 0) this->regenerate_chunk(Vector3Int(x, ring_radius, z) + world_center);
            nb_generated += (ring_radius != 0)?2:1;
        }}
        
        // front and back
        if (ring_radius != 0) { // avoid doing 2 time the central ring
            for (int y = -ring_radius+1; y <= ring_radius-1; y++) {
            for (int z = -ring_radius; z <= ring_radius; z++) {
                this->regenerate_chunk(Vector3Int(-ring_radius, y, z) + world_center);
                this->regenerate_chunk(Vector3Int(ring_radius, y, z) + world_center);
                nb_generated += 2;
            }}
        }

        // top and bottom
        if (ring_radius != 0) {
            for (int x = -ring_radius+1; x <= ring_radius-1; x++) {
            for (int y = -ring_radius+1; y <= ring_radius-1; y++) {
                this->regenerate_chunk(Vector3Int(x, y, ring_radius) + world_center);
                this->regenerate_chunk(Vector3Int(x, y, -ring_radius) + world_center);
                nb_generated += 2;
            }}
        }

        std::cout << "ring " << ring_radius << " generated (" << nb_generated << " chunks)\n";
    }

    this->last_radius_loaded = radius;
}
void World::update(float max_time) {
    #ifndef DISABLE_THREAD
    if (this->task_queue.empty()) {
    #endif
        if (this->last_radius_loaded < (int)this->loading_radius) {
            this->load_circle(this->last_radius_loaded + 1);
        }
    #ifndef DISABLE_THREAD
        return;
    }
    else if (this->get_chunk(this->task_queue.front().chunk_pos)->is_fully_generated()) {
        this->task_queue.front().thread.detach();
        this->send_data(this->task_queue.front().chunk_pos);
        std::cout << "adding " << this->get_chunk(this->task_queue.front().chunk_pos)->flatten()->size() << " |\tbuffer size : " << this->data_buffer.get_used_size() / CELL_MEMORY_SIZE << "\n";
        this->task_queue.pop();
    }
    #endif
}

void World::dispose() {
    #ifndef DISABLE_THREAD
    while (!this->task_queue.empty())
    {
        this->task_queue.front().thread.detach();
        this->task_queue.pop();
    }
    #endif

    if (this->chunks != nullptr) {
        for (int x = 0; x < this->loading_radius * 2 + 1; x++) {
            for (int y = 0; y < this->loading_radius * 2 + 1; y++) {
                for (int z = 0; z < this->loading_radius * 2 + 1; z++) {
                    this->chunks[x][y][z].dispose();
                }
                delete[] this->chunks[x][y];
            }
            delete[] this->chunks[x];
        }
        delete[] this->chunks;
    }
    
    #ifndef DISABLE_BUFFER
    if (this->GPU_root_indexes != nullptr) delete[] this->GPU_root_indexes;

    if (this->data_buffer.is_buffer()) this->data_buffer.dispose();
    if (this->index_buffer.is_buffer()) this->index_buffer.dispose();
    #endif
}
#ifndef DISABLE_BUFFER
void World::create_buffer(GLuint data_buffer_binding, GLuint index_buffer_binding) {
    this->data_buffer = GrowableBuffer(true, CELL_MEMORY_SIZE * 2000 * 10, CELL_MEMORY_SIZE * 200 * this->loading_radius*this->loading_radius*this->loading_radius);
    this->data_buffer.bind_buffer(data_buffer_binding);
    this->index_buffer = Buffer(true);
    this->index_buffer.bind_buffer(index_buffer_binding);
    this->index_buffer.set_data((__pow3(this->loading_radius*2+1) + 2) * sizeof(unsigned int), this->GPU_root_indexes);
}
#endif
void World::send_data() {
    #ifndef DISABLE_BUFFER
    if (!this->data_buffer.is_buffer() || !this->index_buffer.is_buffer()) return;

    for (int x = this->loading_radius; x < this->loading_radius; x++)
    for (int y = this->loading_radius; y < this->loading_radius; y++)
    for (int z = this->loading_radius; z < this->loading_radius; z++)
    {
        this->send_data(Vector3Int(x, y, z) + this->world_center);
    }
    #endif
}
void World::send_data(Vector3Int chunk_pos_modified) {
    #ifndef DISABLE_BUFFER
    if (!this->data_buffer.is_buffer() || !this->index_buffer.is_buffer()) return;
    
    Chunk* chunk = this->get_chunk(chunk_pos_modified);
    unsigned int offset = this->GPU_root_indexes[chunk->GPU_index];
    if (offset != 0) offset -= GPU_CELL_UNUSED_OFFSET;
    
    std::vector<GPUCell>* chunk_data = chunk->flatten(this->compute_lod(chunk_pos_modified));

    unsigned int old_size = chunk->last_GPU_size;
    unsigned int new_size = 0;
    if (chunk_data != nullptr) new_size = chunk_data->size();
    chunk->last_GPU_size = new_size;

    if (new_size == 0) {
        if (old_size == 0) return;

        // erase old data
        data_buffer.replace_data(
            offset * CELL_MEMORY_SIZE,
            old_size * CELL_MEMORY_SIZE,
            0,
            NULL);
        
        for (int i = FIRST_CHUNK_INDEX; i < FIRST_CHUNK_INDEX + __pow3(this->loading_radius * 2 + 1); i++)
        {
            if (this->GPU_root_indexes[i] > this->GPU_root_indexes[chunk->GPU_index]) this->GPU_root_indexes[i] -= old_size;
        }
        this->GPU_root_indexes[chunk->GPU_index] = 0;
        index_buffer.set_data((FIRST_CHUNK_INDEX + __pow3(this->loading_radius * 2 + 1)) * sizeof(unsigned int), this->GPU_root_indexes);
        return;
    }
    else {
        if (old_size == 0) {
            this->GPU_root_indexes[chunk->GPU_index] = data_buffer.push_data(new_size * CELL_MEMORY_SIZE, &((*chunk_data)[0])) / CELL_MEMORY_SIZE + GPU_CELL_UNUSED_OFFSET;
            index_buffer.set_data((FIRST_CHUNK_INDEX + __pow3(this->loading_radius * 2 + 1)) * sizeof(unsigned int), this->GPU_root_indexes);
            return;
        }
        else {
            data_buffer.replace_data(
                offset * CELL_MEMORY_SIZE,
                old_size * CELL_MEMORY_SIZE,
                new_size * CELL_MEMORY_SIZE,
                &((*chunk_data)[0]));
        
            for (int i = FIRST_CHUNK_INDEX; i < FIRST_CHUNK_INDEX + __pow3(this->loading_radius * 2 + 1); i++)
            {
                if (this->GPU_root_indexes[i] > this->GPU_root_indexes[chunk->GPU_index]) this->GPU_root_indexes[i] += new_size - old_size;
            }
            
            index_buffer.set_data((FIRST_CHUNK_INDEX + __pow3(this->loading_radius * 2 + 1)) * sizeof(unsigned int), this->GPU_root_indexes);
            return;
        }
    }
    #endif
}

Chunk* World::get_chunk(int x, int y, int z) {
    int new_x = x - (2*this->loading_radius+1) * floorf((float)x / (2*this->loading_radius+1));
    int new_y = y - (2*this->loading_radius+1) * floorf((float)y / (2*this->loading_radius+1));
    int new_z = z - (2*this->loading_radius+1) * floorf((float)z / (2*this->loading_radius+1));

    return &(this->chunks[new_x][new_y][new_z]);
}
Chunk* World::get_chunk(Vector3Int index) {
    return this->get_chunk(index.x, index.y, index.z);
}
unsigned int World::get(Vector3Int pos, unsigned int default_result) {
    if (!in_bounds(pos)) return default_result;

    unsigned int chunk_x = pos.x >> CHUNK_RESOLUTION;
    unsigned int chunk_y = pos.y >> CHUNK_RESOLUTION;
    unsigned int chunk_z = pos.z >> CHUNK_RESOLUTION;
    
    pos -= Vector3Int(chunk_x, chunk_y, chunk_z) * CHUNK_WIDTH;

    return this->get_chunk(chunk_x, chunk_y, chunk_z)->safe_get(pos, default_result);
}
void World::set(Vector3Int pos, unsigned int value) {
    unsigned int chunk_x = pos.x >> CHUNK_RESOLUTION;
    unsigned int chunk_y = pos.y >> CHUNK_RESOLUTION;
    unsigned int chunk_z = pos.z >> CHUNK_RESOLUTION;
    
    pos -= Vector3Int(chunk_x, chunk_y, chunk_z) * CHUNK_WIDTH;

    this->get_chunk(chunk_x, chunk_y, chunk_z)->set(pos, value);
    this->send_data(Vector3Int(chunk_x, chunk_y, chunk_z));
}

bool World::in_bounds(Vector3 position) {
    return
        abs(position.x - this->world_center.x) < this->loading_radius * CHUNK_WIDTH &&
        abs(position.y - this->world_center.y) < this->loading_radius * CHUNK_WIDTH &&
        abs(position.z - this->world_center.z) < this->loading_radius * CHUNK_WIDTH;
}
unsigned int World::compute_lod(Vector3Int chunk_position) {
    Vector3Int relative_pos = chunk_position - this->world_center;
    relative_pos.z *= 2;

    float dist = relative_pos.magnitude();

    int lod = CHUNK_RESOLUTION;
    if (dist - 1 > this->loading_radius / 2) lod -= 1;
    // if (dist > this->loading_radius) lod -= 1;

    return lod;
}

void World::regenerate_chunk(Vector3Int chunk_position) {
    #ifndef DISABLE_THREAD
    this->task_queue.push({
        this->get_chunk(chunk_position)->generate_threaded(
            *(this->generator),
            chunk_position,
            this->compute_lod(chunk_position)),
        chunk_position
    });
    #else
    this->get_chunk(chunk_position)->generate(
            *(this->generator),
            chunk_position,
            this->compute_lod(chunk_position));
    this->send_data(chunk_position);
    #endif
}

#pragma region raycast
int sign(float v) { return (v > 0) - (v < 0); };
RaycastHit World::get_next_cell(unsigned int& cell_size, Vector3 position, Vector3 direction) {
    Vector3 pos_in_cell = (position / cell_size) % 1; // in [0, 1[
    if (direction.x < 0 && pos_in_cell.x == 0) pos_in_cell.x = 1; // avoid being trap on the edge
    if (direction.y < 0 && pos_in_cell.y == 0) pos_in_cell.y = 1; // avoid being trap on the edge
    if (direction.z < 0 && pos_in_cell.z == 0) pos_in_cell.z = 1; // avoid being trap on the edge
    
    float step_size = 3;
    Vector3 cell_jump = Vector3(0, 0, 0);
    for (int i = 0; i < 3; i++)
    {
        float temp = 3;
        if (direction[i] > 0) temp = (1 - pos_in_cell[i]) / direction[i];
        else if (direction[i] < 0) temp = -pos_in_cell[i] / direction[i];

        if (temp < step_size) {
            step_size = temp;
            cell_jump = Vector3(0, 0, 0);
            cell_jump[i] = sign(direction[i]);
        }
    }
    position += direction * step_size * cell_size;

    unsigned int type = get((position + cell_jump * 0.1).floor());
    return {
        position,                                               // hit position
        cell_jump * -1,                                         // hit normal
        0,                                                      // distance
        type,                                                   // cell value
        Materials::is_solid(type)                               // has hit ?
    };
}
RaycastHit World::raycast(Vector3 start_position, Vector3 direction, float max_dist) {
    if (start_position.x == floor(start_position.x)) start_position.x += 0.001;
    if (start_position.y == floor(start_position.y)) start_position.y += 0.001;
    if (start_position.z == floor(start_position.z)) start_position.z += 0.001;

    unsigned int cell_width = 1;
    unsigned int start_type = get(start_position.floor());
    RaycastHit hit = {
        start_position,                             // hit position
        direction * -1,                             // hit normal
        0,                                          // distance
        start_type,                                 // cell value
        Materials::is_solid(start_type)             // has hit ?
    };

    for (int i = 0; i < max_dist + 2; i++) {
        if (hit.has_hit) {
            hit.distance = __min((start_position - hit.hit_point).magnitude(), max_dist);
            return hit;
        }
        if ((start_position - hit.hit_point).magnitude() > max_dist) break;
        
        if (!in_bounds(hit.hit_point)) cell_width = CHUNK_WIDTH;
        hit = get_next_cell(cell_width, hit.hit_point, direction);
    }
    
    hit = RaycastHit();
    hit.distance = max_dist;
    return hit;
}
RaycastHit World::raycast_down(Vector3 start_position, float max_dist) {
    Vector3Int pos = start_position;

    while (this->in_bounds(pos))
    {
        unsigned int type = this->get(pos);

        if (Materials::is_solid(type)) {
            Vector3 hit_point = Vector3(start_position.x, start_position.y, __min((int)start_position.z, pos.z + 1));
            return {
                hit_point,                              // hit position
                Vector3(0, 0, 1),                       // hit normal
                (start_position.z - hit_point.z),       // distance
                type,                                   // cell value
                true                                    // has hit ?
            };
        }

        if (max_dist < 0 || (start_position.z - pos.z) > max_dist) break;

        pos.z -= 1;
    }

    RaycastHit result = RaycastHit();
    result.distance = max_dist;
    return result;
}
#pragma endregion raycast

#pragma endregion World

#endif