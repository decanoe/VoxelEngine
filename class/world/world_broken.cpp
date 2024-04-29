#ifndef _WORLD_CLASS

#include "./world.h"

#define __pow2(x) (x*x)
#define __pow3(x) (x*x*x)

World::World(unsigned int loading_radius, WorldGenerator* generator, unsigned int depth) {
    this->world_center = Vector3Int(0, 0, 0);
    this->loading_radius = loading_radius;
    this->chunk_depth = depth;

    this->generator = generator;

    this->chunks = new Chunk**[this->loading_radius * 2 + 1];
    for (int x = 0; x < this->loading_radius * 2 + 1; x++)
    {
        this->chunks[x] = new Chunk*[this->loading_radius * 2 + 1];
        for (int y = 0; y < this->loading_radius * 2 + 1; y++) {
            this->chunks[x][y] = new Chunk[this->loading_radius * 2 + 1];
            for (int z = 0; z < this->loading_radius * 2 + 1; z++) {
                this->chunks[x][y][z].world = this;
            }
        }
    }

    this->task_queue = std::queue<GenerationTask>();
    this->last_radius_loaded = -1;
}
void World::load_circle(int radius) {
    radius = __min(radius, this->loading_radius);

    Vector3Int offset = this->world_center;
    for (int ring_radius = this->last_radius_loaded + 1; ring_radius <= radius; ring_radius++)
    {
        int nb_generated = 0;

        for (int x = -ring_radius; x <= ring_radius; x++) {
        for (int z = -ring_radius; z <= ring_radius; z++) {
            this->regenerate_chunk(Vector3Int(x, -ring_radius, z) + offset);
            if (ring_radius != 0) this->regenerate_chunk(Vector3Int(x, ring_radius, z) + offset);
            nb_generated += (ring_radius != 0)?2:1;
        }}
        
        if (ring_radius != 0) { // avoid doing 2 time the central ring
            for (int y = -ring_radius+1; y <= ring_radius-1; y++) {
            for (int z = -ring_radius; z <= ring_radius; z++) {
                this->regenerate_chunk(Vector3Int(-ring_radius, y, z) + offset);
                this->regenerate_chunk(Vector3Int(ring_radius, y, z) + offset);
                nb_generated += 2;
            }}
        }

        if (ring_radius != 0) {
            for (int x = -ring_radius+1; x <= ring_radius-1; x++) {
            for (int y = -ring_radius+1; y <= ring_radius-1; y++) {
                this->regenerate_chunk(Vector3Int(x, y, ring_radius) + offset);
                this->regenerate_chunk(Vector3Int(x, y, -ring_radius) + offset);
                nb_generated += 2;
            }}
        }

        std::cout << "ring " << ring_radius << " generated (" << nb_generated << " chunks)\n";
    }

    this->last_radius_loaded = radius;
}
void World::update(float max_time) {
    if (this->task_queue.empty()) {
        if (this->last_radius_loaded < (int)this->loading_radius) {
            this->load_circle(this->last_radius_loaded + 1);
        }
        return;
    }
    else if (this->get_chunk(this->task_queue.front().chunk_pos).is_fully_generated()) {
        this->task_queue.front().thread.detach();
        this->send_data(this->task_queue.front().chunk_pos);
        this->task_queue.pop();
    }
}

void World::dispose() {
    this->data_buffer.dispose();
    this->index_buffer.dispose();

    delete this->generator;

    for (int x = 0; x < this->loading_radius * 2 + 1; x++)
    {
        for (int y = 0; y < this->loading_radius * 2 + 1; y++)
        {
            for (int z = 0; z < this->loading_radius * 2 + 1; z++)
            {
                this->chunks[x][y][z].dispose();
            }
            
            delete[] this->chunks[x][y];
        }
        
        delete[] this->chunks[x];
    }
    delete[] this->chunks;

    while (!this->task_queue.empty())
    {
        this->task_queue.front().thread.detach();
        this->task_queue.pop();
    }
}
void World::create_buffer(GLuint data_buffer_binding, GLuint index_buffer_binding) {
    this->data_buffer = GrowableBuffer(true, CELL_MEMORY_SIZE * 2000 * 10, CELL_MEMORY_SIZE * 200 * this->loading_radius*this->loading_radius*this->loading_radius);
    this->data_buffer.bind_buffer(data_buffer_binding);
    this->index_buffer = Buffer(true);
    this->index_buffer.bind_buffer(index_buffer_binding);
}
unsigned int get_GPU_index(int x, int y, int z, int loading_radius, Vector3Int center) {
    x = (x - center.x + loading_radius);
    y = (y - center.y + loading_radius);
    z = (z - center.z + loading_radius);
    return (loading_radius * 2 + 1) * ((loading_radius * 2 + 1) * x + y) + z;
}
void World::send_data() {
    this->sent_indexes = std::vector<unsigned int>(2 + __pow3(this->loading_radius * 2 + 1));
    this->sent_sizes = std::vector<unsigned int>(__pow3(this->loading_radius * 2 + 1));
    this->sent_indexes[1] = (1 << this->chunk_depth); // nb of minimal size cells in the side of a chunk 
    std::vector<GPUCell> data = std::vector<GPUCell>();

    auto start = std::chrono::system_clock::now();

    for (int x = this->world_center.x - this->loading_radius; x <= this->world_center.x + this->loading_radius; x++)
    for (int y = this->world_center.y - this->loading_radius; y <= this->world_center.y + this->loading_radius; y++)
    for (int z = this->world_center.z - this->loading_radius; z <= this->world_center.z + this->loading_radius; z++)
    {
        std::vector<GPUCell>* chunk_data = this->get_chunk(x, y, z).flatten(this->compute_lod(Vector3Int(x, y, z)));
    
        unsigned int index = get_GPU_index(x, y, z, this->loading_radius, this->world_center);

        if (chunk_data != nullptr) {
            this->sent_indexes[2 + index] = data.size() + CHUNK_INDEX_UNUSED_OFFSET;
            this->sent_sizes[index] = chunk_data->size();
            data.reserve(data.size() + chunk_data->size());
            data.insert(data.end(), chunk_data->begin(), chunk_data->end());
        }
        else {
            this->sent_indexes[2 + index] = 0;
            this->sent_sizes[index] = 0;
        }
    }

    this->sent_indexes.front() = (unsigned int)(this->loading_radius * 2 + 1); // side of the world in chunks
    
    index_buffer.set_data(this->sent_indexes.size() * sizeof(unsigned int), &this->sent_indexes[0]);
    data_buffer.set_data(data.size() * CELL_MEMORY_SIZE, &data[0]);
}
void World::send_data(Vector3Int chunk_pos_modified) {
    std::cout << chunk_pos_modified.to_str() << "\n";
    return;
    unsigned int modified_chunk_index = get_GPU_index(chunk_pos_modified.x, chunk_pos_modified.y, chunk_pos_modified.z, this->loading_radius, this->world_center);
    unsigned int offset = this->sent_indexes[2 + modified_chunk_index];
    if (offset != 0) offset -= CHUNK_INDEX_UNUSED_OFFSET;
    
    std::vector<GPUCell>* chunk_data = this->get_chunk(chunk_pos_modified).flatten(this->compute_lod(chunk_pos_modified));

    unsigned int old_size = this->sent_sizes[modified_chunk_index];
    unsigned int new_size = 0;
    if (chunk_data != nullptr) new_size = chunk_data->size();
    this->sent_sizes[modified_chunk_index] = new_size;

    if (new_size == 0) {
        if (old_size == 0) return;

        // erase old data
        data_buffer.replace_data(
            offset * CELL_MEMORY_SIZE,
            old_size * CELL_MEMORY_SIZE,
            0,
            NULL);
        
        this->sent_indexes[2 + modified_chunk_index] = 0;
        for (int i = 2; i < this->sent_indexes.size(); i++)
        {
            if (this->sent_indexes[i] > offset + CHUNK_INDEX_UNUSED_OFFSET) this->sent_indexes[i] -= old_size;
        }
        index_buffer.set_data(this->sent_indexes.size() * sizeof(unsigned int), &this->sent_indexes[0]);
        return;
    }
    else {
        if (old_size == 0) {
            this->sent_indexes[2 + modified_chunk_index] = data_buffer.push_data(new_size * CELL_MEMORY_SIZE, &((*chunk_data)[0])) / CELL_MEMORY_SIZE + CHUNK_INDEX_UNUSED_OFFSET;
            index_buffer.set_data(this->sent_indexes.size() * sizeof(unsigned int), &this->sent_indexes[0]);
            return;
        }
        else {
            data_buffer.replace_data(
                offset * CELL_MEMORY_SIZE,
                old_size * CELL_MEMORY_SIZE,
                new_size * CELL_MEMORY_SIZE,
                &((*chunk_data)[0]));
        
            for (int i = 2; i < this->sent_indexes.size(); i++)
            {
                if (this->sent_indexes[i] > offset + CHUNK_INDEX_UNUSED_OFFSET) this->sent_indexes[i] += new_size - old_size;
            }
            
            index_buffer.set_data(this->sent_indexes.size() * sizeof(unsigned int), &this->sent_indexes[0]);
            return;
        }
    }
}

Chunk& World::get_chunk(int x, int y, int z) {
    return this->chunks[x % (this->loading_radius * 2 + 1)][y % (this->loading_radius * 2 + 1)][z % (this->loading_radius * 2 + 1)];
}
Chunk& World::get_chunk(Vector3Int index) {
    return this->get_chunk(index.x, index.y, index.z);
}
unsigned int World::get(Vector3Int pos, unsigned int default_result) {
    if (!in_bounds(pos)) return default_result;

    unsigned int chunk_x = pos.x >> this->chunk_depth;
    unsigned int chunk_y = pos.y >> this->chunk_depth;
    unsigned int chunk_z = pos.z >> this->chunk_depth;
    
    pos -= Vector3Int(chunk_x, chunk_y, chunk_z) * (1 << this->chunk_depth);

    return this->get_chunk(chunk_x, chunk_y, chunk_z).safe_get(pos);
}
void World::set(Vector3Int pos, unsigned int value) {
    unsigned int chunk_x = pos.x >> this->chunk_depth;
    unsigned int chunk_y = pos.y >> this->chunk_depth;
    unsigned int chunk_z = pos.z >> this->chunk_depth;
    
    pos -= Vector3Int(chunk_x, chunk_y, chunk_z) * (1 << this->chunk_depth);

    this->get_chunk(chunk_x, chunk_y, chunk_z).set(pos, value);
    this->send_data(Vector3Int(chunk_x, chunk_y, chunk_z));
}

bool World::in_bounds(Vector3 position) {
    return
        std::abs(position.z - this->world_center.x) < this->loading_radius * (1<<this->chunk_depth) &&
        std::abs(position.y - this->world_center.y) < this->loading_radius * (1<<this->chunk_depth) &&
        std::abs(position.z - this->world_center.z) < this->loading_radius * (1<<this->chunk_depth);
}
unsigned int World::compute_lod(Vector3Int chunk_position) {
    Vector3Int relative_pos = chunk_position - this->world_center;
    relative_pos.z *= 2;

    float dist = relative_pos.magnitude();

    int lod = this->get_chunk(chunk_position).depth;
    if (dist - 1 > this->loading_radius / 2) lod -= 1;
    // if (dist > this->loading_radius) lod -= 1;

    return lod;
}

void World::regenerate_chunk(Vector3Int chunk_position) {
    if (this->world_center != chunk_position) return;
    this->get_chunk(chunk_position).depth = this->chunk_depth;

    this->task_queue.push({
        this->get_chunk(chunk_position).generate_threaded(
            *(this->generator),
            chunk_position * (1<<this->chunk_depth),
            this->compute_lod(chunk_position)),
        chunk_position
    });
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
        
        if (!in_bounds(hit.hit_point)) cell_width = 1<<this->chunk_depth;
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

#endif