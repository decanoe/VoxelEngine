#ifndef _WORLD_GENERATOR_CLASS
#define _WORLD_GENERATOR_CLASS

#include <vector>
#include <experimental/random>

#include "./materials.h"
#include "./world.h"
#include "../utility/math/vector3.h"

class Cell;

class WorldGenerator
{
private:
    float block_size = 1;
public:
    WorldGenerator();
    WorldGenerator(float block_size);
    unsigned int generate_value(Vector3 cell_pos);
    void populate_cell(Cell& cell, Vector3Int cell_pos, unsigned int cell_size);
};

#endif