#ifndef _CHUNK_CLASS

#include "./chunk.h"

#define __pow2(x) ((x)*(x))
#define __pow3(x) ((x)*(x)*(x))

#pragma region GPUCell

unsigned int& GPUCell::operator[](int code) {
    switch (code)
    {
    case 0: return this->cell_000;
    case 1: return this->cell_001;
    case 2: return this->cell_010;
    case 3: return this->cell_011;
    case 4: return this->cell_100;
    case 5: return this->cell_101;
    case 6: return this->cell_110;
    case 7: return this->cell_111;
    default:
        std::cerr << "ERROR : trying to acces cell child of code " << code << "\n";
        exit(1);
    }
}

#pragma endregion


#pragma region Chunk
Chunk::Chunk() {
    this->world = nullptr;
    this->fully_generated = {false};
    this->flatten_data = std::vector<GPUCell>();
}
void Chunk::dispose() {
    this->flatten_data.clear();

    if (this->cells == nullptr) return;

    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int y = 0; y < CHUNK_WIDTH; y++) {
            delete this->cells[x][y];
        }
        delete this->cells[x];
    }
    delete this->cells;
}
bool Chunk::in_bounds(Vector3Int position) {
    return 
        position.z >= 0 &&
        position.z < CHUNK_WIDTH &&
        position.x >= 0 &&
        position.x < CHUNK_WIDTH &&
        position.y >= 0 &&
        position.y < CHUNK_WIDTH;
}

bool Chunk::is_fully_generated() {
    return this->fully_generated;
}

unsigned int Chunk::populate_gpu_data(std::vector<GPUCell>& data, Vector3Int pos, unsigned int cell_size, unsigned int min_cell_size) {
    if (cell_size < min_cell_size) return 0;
    if (!has_side_visible(pos, cell_size)) return 0;

    Cell* cell = this->operator[](pos);
    unsigned int added_index = data.size();
    data.push_back({ cell->value });

    if (cell_size == min_cell_size) return added_index;
    if (!this->has_subcells(pos, cell_size)) return added_index;

    cell_size >>= 1;
    int code = -1;
    for (int x = 0; x <= 1; x++)
    for (int y = 0; y <= 1; y++)
    for (int z = 0; z <= 1; z++)
    {
        code++;
        Vector3Int subcell_pos = pos + (Vector3Int(x, y, z) * cell_size);
        Cell* subcell = this->operator[](subcell_pos);
        
        if (!this->has_subcells(subcell_pos, cell_size) && subcell->value == cell->value) continue;

        unsigned int index = populate_gpu_data(data, subcell_pos, cell_size, min_cell_size);
        data[added_index][code] = index;
        // std::cout << code << " -> " << index << " | " << subcell_pos.to_str() << " => " << subcell->value << "\n";
    }
    return added_index;
}
bool Chunk::has_subcells(Vector3Int cell_pos, unsigned int cell_size) {
    if (cell_size == 1) return false;

    unsigned int type = this->get(cell_pos);
    cell_size >>= 1;
    for (int x = 0; x <= 1; x++)
    for (int y = 0; y <= 1; y++)
    for (int z = 0; z <= 1; z++)
    {
        if (this->has_subcells(cell_pos + Vector3Int(x, y, z) * cell_size, cell_size)) return true;
        if (this->get(cell_pos + Vector3Int(x, y, z) * cell_size) != type) return true;
    }
    return false;
}
bool Chunk::has_side_visible(Vector3Int cell_pos) {
    if (Materials::see_through(this->get(cell_pos))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(1, 0, 0)))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(-1, 0, 0)))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(0, 1, 0)))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(0, -1, 0)))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(0, 0, 1)))) return true;
    if (Materials::see_through(this->get(cell_pos + Vector3Int(0, 0, -1)))) return true;
    return false;
}
bool Chunk::has_side_visible(Vector3Int cell_pos, unsigned int cell_size) {
    for (int x = 0; x < cell_size; x++)
    for (int y = 0; y < cell_size; y++)
    for (int z = cell_size - 1; z >= 0; z--)
    {
        if (this->has_side_visible(cell_pos + Vector3Int(x, y, z))) return true;
    }
    return false;
}
std::vector<GPUCell>* Chunk::flatten() {
    if (!this->is_fully_generated()) return nullptr;
    if (this->flatten_lod < 0)
        return this->flatten(CHUNK_RESOLUTION);
    else
        return &(this->flatten_data);
}
std::vector<GPUCell>* Chunk::flatten(unsigned int lod) {
    if (!this->is_fully_generated()) return nullptr;

    if (this->flatten_lod < (int)lod) {
        this->flatten_data.clear();
        populate_gpu_data(this->flatten_data, Vector3Int(0, 0, 0), CHUNK_WIDTH, 1<<(CHUNK_RESOLUTION - lod));
        this->flatten_lod = lod;
    }
    return &(this->flatten_data);
}

void Chunk::generate(WorldGenerator generator, Vector3Int chunk_pos, unsigned int lod) {
    this->chunk_pos = chunk_pos;
    this->dispose();

    Vector3Int chunk_world_pos = chunk_pos * CHUNK_WIDTH;

    this->cells = new Cell**[CHUNK_WIDTH];
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        this->cells[x] = new Cell*[CHUNK_WIDTH];
        for (int y = 0; y < CHUNK_WIDTH; y++) {
            this->cells[x][y] = new Cell[CHUNK_WIDTH];
            for (int z = 0; z < CHUNK_WIDTH; z++)
            {
                this->cells[x][y][z] = { generator.generate_value(chunk_world_pos + Vector3Int(x, y, z)) };
            }
        }
    }

    this->flatten_data.clear();
    populate_gpu_data(this->flatten_data, Vector3Int(0, 0, 0), CHUNK_WIDTH , 1<<(CHUNK_RESOLUTION - lod));
    this->flatten_lod = lod;
    // if (this->flatten_data.size() != 1 && lod == CHUNK_RESOLUTION) std::cout << "nb cells: " << this->flatten_data.size() << "\n";

    this->fully_generated = true;
}
#ifndef DISABLE_THREAD
mingw_stdthread::thread Chunk::generate_threaded(WorldGenerator generator, Vector3Int chunk_pos, unsigned int lod) {
    this->fully_generated = false;
    mingw_stdthread::thread th(Chunk::generate, this, generator, chunk_pos, lod);
    return th;
}
#endif

Cell* Chunk::operator[](Vector3Int pos) {
    if (this->cells == nullptr) {
        std::cerr << "accessing undefined chunk cells\n";
        exit(1);
    }
    if (!this->in_bounds(pos)) {
        std::cerr << "cell not in chunk (pos: " << pos.to_str() << ")\n";
        exit(1);
    }
    
    return &(this->cells[pos.x][pos.y][pos.z]);
}
unsigned int Chunk::safe_get(Vector3Int pos, unsigned int default_result) {
    if (!this->is_fully_generated()) return default_result;
    if (!this->in_bounds(pos)) return default_result;

    return this->cells[pos.x][pos.y][pos.z].value;
}
unsigned int Chunk::get(Vector3Int pos, unsigned int default_result) {
    if (!this->in_bounds(pos)) {
        if (this->world != nullptr) return world->get(pos + this->chunk_pos * CHUNK_WIDTH, default_result);
        return default_result;
    }

    return this->cells[pos.x][pos.y][pos.z].value;
}
bool Chunk::set(Vector3Int pos, unsigned int value) {
    if (!this->is_fully_generated()) return false;
    if (!this->in_bounds(pos)) return false;

    this->cells[pos.x][pos.y][pos.z].value = value;

    // reflatten the data
    this->flatten_lod = -1;

    return true;
}

#pragma endregion

#endif