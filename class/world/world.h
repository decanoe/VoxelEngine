#ifndef _WORLD_CLASS
#define _WORLD_CLASS

#include <iostream>
#ifndef DISABLE_THREAD
#include "../../mingw_stdthreads/mingw.thread.h"
#endif
#include <atomic>

#include <vector>
#include <queue>
#include <chrono>

#include "./materials.h"
#include "./world_generator.h"
class WorldGenerator;
#include "./chunk.h"
class Chunk;

#include "../utility/math/vector3.h"
#ifndef DISABLE_BUFFER
    #include "../utility/graphics/buffer.h"
#endif

#define GPU_CELL_UNUSED_OFFSET 1

struct RaycastHit{
    Vector3 hit_point = Vector3(0, 0, 0);
    Vector3 normal = Vector3(0, 0, 0);
    float distance = 0;
    unsigned int cell_value = 0;
    bool has_hit = false;
};

class World
{
private:
    Chunk*** chunks = nullptr;
    Vector3Int world_center;
    unsigned int loading_radius;
    int last_radius_loaded;

    WorldGenerator* generator = nullptr;
    #ifndef DISABLE_THREAD
    struct GenerationTask { mingw_stdthread::thread thread; Vector3Int chunk_pos; };
    std::queue<GenerationTask> task_queue;
    #endif

    #ifndef DISABLE_BUFFER
    unsigned int* GPU_root_indexes = nullptr;

    GrowableBuffer data_buffer;
    Buffer index_buffer;
    #endif

    RaycastHit get_next_cell(unsigned int& cell_size, Vector3 position, Vector3 direction);
    unsigned int compute_lod(Vector3Int chunk_position);
public:
    void regenerate_chunk(Vector3Int chunk_position);
    World(unsigned int loading_radius, WorldGenerator* generator);
    void load_circle(int radius);

    void update(float max_time);

    void dispose();
    #ifndef DISABLE_BUFFER
    void create_buffer(GLuint data_buffer_binding, GLuint index_buffer_binding);
    #endif
    void send_data();
    void send_data(Vector3Int chunk_pos_modified);

    Chunk* get_chunk(Vector3Int index);
    Chunk* get_chunk(int x, int y, int z);
    unsigned int get(Vector3Int pos, unsigned int default_result = MATERIAL_AIR);
    void set(Vector3Int pos, unsigned int value);

    bool in_bounds(Vector3 position);
    RaycastHit raycast(Vector3 position, Vector3 direction, float max_dist = 0);
    RaycastHit raycast_down(Vector3 position, float max_dist = 0);
};

#endif