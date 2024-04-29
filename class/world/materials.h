#ifndef _MATERIALS
#define _MATERIALS

#define MATERIAL_AIR        0U
#define MATERIAL_GRASS      1U
#define MATERIAL_DIRT       2U
#define MATERIAL_STONE      3U
#define MATERIAL_WATER      4U
#define MATERIAL_BUILDING   5U
#define MATERIAL_LIGHT      6U

namespace Materials {
    bool see_through(unsigned int material_id);
    bool is_solid(unsigned int material_id);
}

#endif