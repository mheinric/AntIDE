#pragma once
#include "utils.h"
/// Datastructures for holding the simulation map.

typedef struct {
    size_t x; 
    size_t y;
} Position;

typedef enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_WALL,
    CELL_TYPE_NEST,
} CellType;

typedef struct {
    CellType type;
    uint8_t pheromones[4];
    uint8_t food_amount;
    size_t nb_ants;
} Cell;

void
cell_serialization(const Cell* cell, char* buffer);

typedef enum {
    MAP_TYPE_DEFAULT,
    MAP_TYPE_OPEN,
} MapType;

bool 
map_type_deserialization(char* str_type, MapType* map_type);

typedef struct {
    size_t random_seed; 
    size_t width; 
    size_t height;
    MapType map_type;
    bool generate_nest;
    bool generate_food;
} MapSettings;

MapSettings
map_settings_create_default(size_t random_seed);

MapSettings
map_settings_create_test();

typedef struct {
    size_t width; 
    size_t height; 
    Cell* cells;
    Position starting_pos;
} GridMap; 

void
grid_map_init(GridMap* map, MapSettings settings);

bool 
grid_map_init_from_data(GridMap* map, const char* data);

bool
grid_map_init_from_file(GridMap* map, const char* file_name);

void 
grid_map_cleanup(GridMap* map);

cJSON*
grid_map_to_json(const GridMap* map);

Cell*
grid_map_get_cell(const GridMap* map, Position pos);