#include "map.h"
#include "internals/rng.h"

// Serialization format: 
// type: 1byte
// food_amount: 1byte
// nb_ants: 8bytes
// Keep this value in sync with grid.html
#define BYTES_PER_CELL (1 + 1 + 8)

void cell_serialization(const Cell *cell, char *buffer)
{
    buffer[0] = '0' + cell->type;
    buffer[1] = '0' + cell->food_amount;
    sprintf(buffer + 2, "%08x", (unsigned int) cell->nb_ants); 
}

MapSettings map_settings_create_default(size_t random_seed)
{
    return (MapSettings) {
        .random_seed = random_seed,
        .width = 128, 
        .height = 128,
        .generate_nest = true,
        .generate_food = true,
    };
}

MapSettings map_settings_create_test()
{
    return (MapSettings) {
        .random_seed = 42,
        .width = 10, 
        .height = 10,
        .generate_nest = false,
        .generate_food = false,
    };
}

void grid_map_init(GridMap *map, MapSettings settings)
{
    map->starting_pos = (Position) { .x = (settings.width - 1) / 2, .y = (settings.height -1) / 2 }; 
    map->width = settings.width; 
    map->height = settings.height;
    map->cells = calloc(map->width * map->height, sizeof(Cell));
    for (size_t y = 0; y < map->height; y++)
    {
        for (size_t x = 0; x < map->width; x++)
        {
            Position pos = { .x = x, .y = y };
            if (x == 0 || y == 0 || x == map->width - 1 || y == map->height - 1)
            {
                //Put walls on all the borders of the map.
                grid_map_get_cell(map, pos)->type = CELL_TYPE_WALL; 
            }
        }
    }
    if (settings.generate_nest)
    {
        for (int i = -1; i < 2; i++)
        {
            for (int j = -1; j < 2; j++)
            {
                Position nest_pos = { .x = map->starting_pos.x + i, .y = map->starting_pos.y + j };
                grid_map_get_cell(map, nest_pos)->type = CELL_TYPE_NEST;
            }
        }
    }
    RandomGenerator rand;
    random_generator_init(&rand, settings.random_seed); 
    if (settings.generate_food)
    {
        for (int i = 0; i < 1000; i++)
        {
            Position pos = { 
                .x = random_generator_generate(&rand, settings.width), 
                .y = random_generator_generate(&rand, settings.height)
            };
            Cell *cell = grid_map_get_cell(map, pos);
            if (cell->type == CELL_TYPE_EMPTY && cell->food_amount < 8)
            {
                cell->food_amount++; 
            }
        }
    }
}

void grid_map_cleanup(GridMap *map)
{
    free(map->cells);
    memset(map, 0, sizeof(GridMap));
}

cJSON *grid_map_to_json(const GridMap *map)
{
    cJSON* map_json = cJSON_CreateObject(); 
    cJSON_AddNumberToObject(map_json, "width", map->width); 
    cJSON_AddNumberToObject(map_json, "height", map->height);

    char* cell_data = calloc(map->width * map->height + 1, BYTES_PER_CELL); 
    char* it = cell_data;
    for (size_t y = 0; y < map->height; y++)
    {
        for (size_t x = 0; x < map->width; x++)
        {
            cell_serialization(grid_map_get_cell(map, (Position) { .x = x, .y = y }), it); 
            it += BYTES_PER_CELL;
        }
    }

    cJSON_AddStringToObject(map_json, "cells", cell_data);
    free(cell_data); 
    return map_json;
}

Cell *grid_map_get_cell(const GridMap *map, Position pos)
{
    assert(pos.x < map->width);
    assert(pos.y < map->height);
    return &map->cells[map->width * pos.y + pos.x];
}
