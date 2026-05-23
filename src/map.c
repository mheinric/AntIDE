#include "map.h"
#include "internals/rng.h"

// Serialization format (for performance, a binary format is used): 
// type: 1byte
// food_amount: 1byte
// pheromones: 4x2bytes
// Keep this value in sync with grid.html
#define BYTES_PER_CELL 10

void cell_serialization(const Cell *cell, char *buffer)
{
    buffer[0] = '0' + cell->type;
    buffer[1] = '0' + cell->food_amount;
    for (int i = 0; i < 4; i++)
    {
        sprintf(buffer + 2 + 2 * i, "%02x", cell->pheromones[i]);
    }
}

bool map_type_deserialization(const char *str_type, MapType *map_type)
{
    if (strcmp(str_type, "default") == 0)
    {
        *map_type = MAP_TYPE_DEFAULT; 
        return true;
    }
    if (strcmp(str_type, "open") == 0)
    {
        *map_type = MAP_TYPE_OPEN; 
        return true;
    }
    return false;
}

MapSettings map_settings_create_default(size_t random_seed)
{
    return (MapSettings) {
        .random_seed = random_seed,
        .width = 128, 
        .height = 128,
        .map_type = MAP_TYPE_DEFAULT,
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
        .map_type = MAP_TYPE_DEFAULT,
        .generate_nest = false,
        .generate_food = false,
    };
}

void grid_map_init_food_default(GridMap* map, RandomGenerator* rand)
{
    for (int i = 0; i < 1000; i++)
    {
        Position pos = { 
            .x = random_generator_generate(rand, map->width), 
            .y = random_generator_generate(rand, map->height)
        };
        Cell *cell = grid_map_get_cell(map, pos);
        if (cell->type == CELL_TYPE_EMPTY && cell->food_amount < 8)
        {
            cell->food_amount++; 
        }
    }
}

void grid_map_init_food_open(GridMap* map, RandomGenerator* rand)
{
    const size_t max_cell_radius = 7;
    const size_t min_cell_radius = 2;
    size_t total_set_amount = 0;
    while (total_set_amount < 1000)
    {
        //Center of the patch
        //Should not be too close from the border
        Position pos = { 
            .x = max_cell_radius + random_generator_generate(rand, map->width - 2 * max_cell_radius - 1), 
            .y = max_cell_radius + random_generator_generate(rand, map->height - 2 * max_cell_radius - 1)
        };
        const size_t cell_radius = min_cell_radius + random_generator_generate(rand, max_cell_radius - min_cell_radius);
        for (size_t x = pos.x - cell_radius; x <= pos.x + cell_radius; x++)
        {
            for (size_t y = pos.y - cell_radius; y < pos.y + cell_radius; y++)
            {
                const size_t square_dist = (x - pos.x) * (x - pos.x) + (y - pos.y) * (y - pos.y);
                if (square_dist > cell_radius * cell_radius)
                {
                    continue;
                }
                Cell* cell = grid_map_get_cell(map, (Position) { .x = x, .y = y });
                cell->food_amount = 3;
                total_set_amount += 3;
            }
        }
    }
}


void grid_map_init_food(GridMap* map, MapSettings settings, RandomGenerator* rand)
{
    switch (settings.map_type)
    {
        case MAP_TYPE_DEFAULT:
        {
            grid_map_init_food_default(map, rand);
            break;
        }
        case MAP_TYPE_OPEN:
        {
            grid_map_init_food_open(map, rand);
            break;
        }
    }
}

void grid_map_init(GridMap *map, MapSettings settings)
{
    RandomGenerator rand;
    random_generator_init(&rand, settings.random_seed);
    if (settings.generate_nest) {
        map->starting_pos = (Position) { 
            .x = 2 + random_generator_generate(&rand, settings.width - 4), 
            .y = 2 + random_generator_generate(&rand, settings.width - 4) 
        }; 
    }
    else {
        map->starting_pos = (Position) { 
            .x = (settings.width - 1) / 2, 
            .y = (settings.height -1) / 2 
        }; 
    }
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
    if (settings.generate_food)
    {
        grid_map_init_food(map, settings, &rand);
    }
}

bool 
grid_map_init_from_data(GridMap* map, const char* data)
{
    map->width = 0; 
    map->height = 1;
    for (const char* it = data; *it != '\0'; it++)
    {
        if (map->width == 0 && *it == '\n')
        {
            map->width = it - data;
        }
        if (*it == '\n')
        {
            map->height++;
        }
    }
    map->cells = calloc(map->width * map->height, sizeof(Cell));
    size_t column_index = 0; 
    size_t row_index = map->height - 1;
    bool init_pos_found = false;
    for (const char* it = data; *it != '\0'; it++)
    {
        switch(*it)
        {
            case '\n': 
            {
                column_index = 0; 
                row_index--;
                break;
            }
            case '\r': break;
            case ' ':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            {
                Cell* cell = grid_map_get_cell(map, (Position) { .x = column_index, .y = row_index });
                cell->type = CELL_TYPE_EMPTY;
                if (*it != ' ')
                {
                    cell->food_amount = *it - '0';
                }
                column_index++;
                break;
            }
            case 'N': 
            case 'I': 
            {
                Cell* cell = grid_map_get_cell(map, (Position) { .x = column_index, .y = row_index });
                cell->type = CELL_TYPE_NEST;
                if (*it == 'I')
                {
                    init_pos_found = true; 
                    map->starting_pos = (Position) { .x = column_index, .y = row_index };
                }
                column_index++;
                break;
            }
            case 'W': 
            {
                Cell* cell = grid_map_get_cell(map, (Position) { .x = column_index, .y = row_index });
                cell->type = CELL_TYPE_WALL;
                column_index++;
                break;
            }
            default:
            {
                //Invalid character in map data
                return false;
            }
        }
    }
    return init_pos_found;
}

bool
grid_map_init_from_file(GridMap* map, const char* file_name)
{
    char* file_content = read_file_content(file_name);
    if (file_content == NULL)
    {
        memset(map, 0, sizeof(GridMap));
        return false;
    }
    bool res = grid_map_init_from_data(map, file_content);
    free(file_content);
    return res;
}

void grid_map_cleanup(GridMap *map)
{
    if (map->cells != NULL) free(map->cells);
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
