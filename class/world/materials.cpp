#ifndef _MATERIALS
#include "./materials.h"

bool Materials::see_through(unsigned int material_id) {
    switch (material_id)
    {
    case MATERIAL_AIR:
        return true;
    case MATERIAL_WATER:
        return true;
    
    default:
        return false;
    }
}
bool Materials::is_solid(unsigned int material_id) {
    switch (material_id)
    {
    case MATERIAL_AIR:
        return false;
    case MATERIAL_WATER:
        return false;
    
    default:
        return true;
    }
}

#endif