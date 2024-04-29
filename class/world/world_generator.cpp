#ifndef _WORLD_GENERATOR_CLASS

#include "./world_generator.h"

WorldGenerator::WorldGenerator() {};
WorldGenerator::WorldGenerator(float block_size) {
    this->block_size = block_size;
}
unsigned int WorldGenerator::generate_value(Vector3 cell_pos) {
    float weight = 8;
    float size = 16;

    float ground_level = 0;

    for (int i = 0; i < 3; i++)
    {
        ground_level += (sin(cell_pos.x / size) + cos(cell_pos.y / size)) * weight;
        weight /= 2;
        size /= 2;
    }
    

    if (cell_pos.z > ground_level) {
        if (cell_pos.z < 0) {
            return MATERIAL_WATER;
        }
        return MATERIAL_AIR;
    }
    else if (cell_pos.z > ground_level - 2) {
        return MATERIAL_GRASS;
    }
    else if (cell_pos.z > ground_level - 8) {
        return MATERIAL_DIRT;
    }
    else {
        return MATERIAL_STONE;
    }
}

#endif