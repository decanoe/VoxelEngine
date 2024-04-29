#ifndef _CHUNK_CLASS
#define _CHUNK_CLASS
#define CHUNK_RESOLUTION 5
#define CHUNK_WIDTH (1<<CHUNK_RESOLUTION)

#include <iostream>
#ifndef DISABLE_THREAD
#include "../../mingw_stdthreads/mingw.thread.h"
#endif
#include <atomic>

#include <vector>

#include "./materials.h"
#include "./world_generator.h"
#include "./world.h"
class WorldGenerator;

#include "../utility/math/vector3.h"

#define CELL_MEMORY_SIZE (sizeof(unsigned int) * 9)
struct Cell{
    unsigned int value = MATERIAL_AIR;
};
struct GPUCell{
    unsigned int value = MATERIAL_AIR;
    unsigned int cell_000 = 0;
    unsigned int cell_001 = 0;
    unsigned int cell_010 = 0;
    unsigned int cell_011 = 0;
    unsigned int cell_100 = 0;
    unsigned int cell_101 = 0;
    unsigned int cell_110 = 0;
    unsigned int cell_111 = 0;

    // (x, y, z) -> ²xyz
    // (0, 1, 0) -> ²010 = 2
    unsigned int& operator[](int code);
};

class World;
class Chunk
{
private:
    std::vector<GPUCell> flatten_data;
    int flatten_lod = -1;
    std::atomic_bool fully_generated = {false};

    Cell* operator[](Vector3Int pos);
    
    bool has_subcells(Vector3Int cell_pos, unsigned int cell_size);
    bool has_side_visible(Vector3Int cell_pos);
    bool has_side_visible(Vector3Int cell_pos, unsigned int cell_size);
public:
    #ifndef DISABLE_BUFFER
    unsigned int last_GPU_size = 0;
    unsigned int GPU_index = 0;
    #endif

    World* world;
    Vector3Int chunk_pos;
    Cell*** cells = nullptr;
    Chunk();
    Chunk & operator=(const Chunk&) = delete;
    Chunk(const Chunk&) = delete;
    void dispose();
    bool in_bounds(Vector3Int position);

    bool is_fully_generated();
    
    void generate(WorldGenerator generator, Vector3Int chunk_pos, unsigned int lod);
    #ifndef DISABLE_THREAD
    mingw_stdthread::thread generate_threaded(WorldGenerator generator, Vector3Int chunk_pos, unsigned int lod);
    #endif
    
    unsigned int populate_gpu_data(std::vector<GPUCell>& data, Vector3Int pos, unsigned int cell_size, unsigned int min_cell_size = 1);
    std::vector<GPUCell>* flatten();
    std::vector<GPUCell>* flatten(unsigned int lod);

    unsigned int safe_get(Vector3Int pos, unsigned int default_result = MATERIAL_AIR);
    unsigned int get(Vector3Int pos, unsigned int default_result = MATERIAL_AIR);
    bool set(Vector3Int pos, unsigned int value);
};
#endif