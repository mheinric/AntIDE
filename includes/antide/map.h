#pragma once
#include "utils.h"
#include <cJSON/cJSON.h>

typedef struct {
    size_t x; 
    size_t y;
} Position;

typedef enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_WALL,
    CELL_TYPE_NEST,
} CellType;

cJSON*
cell_type_to_json(CellType cellType);

typedef struct {
    CellType type;
    uint8_t pheromones[4];
    uint8_t food_amount;
    size_t nb_ants;
} Cell;

cJSON*
cell_to_json(const Cell* cell);

typedef struct {
    size_t random_seed; 
    size_t width; 
    size_t height;
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

void 
grid_map_cleanup(GridMap* map);

cJSON*
grid_map_to_json(const GridMap* map);

Cell*
grid_map_get_cell(const GridMap* map, Position pos);