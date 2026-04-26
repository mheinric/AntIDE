#include "debugger_handlers.h"
#include "parser.h"
#include "json_rpc.h"

cJSON* debugger_handle_initialize(Debugger* /*dbg*/)
{
    //Return the server capabilities
    cJSON* capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(capabilities, "supportsTerminateRequest", true);
    cJSON_AddBoolToObject(capabilities, "supportsInstructionBreakpoints", true);
    cJSON_AddBoolToObject(capabilities, "supportsConfigurationDoneRequest", true);
    return capabilities; 
}

cJSON* 
debugger_handle_disconnect(Debugger* dbg)
{
    dbg->exit_debugger = true;
    sem_destroy(&dbg->pause_semaphore);
    return cJSON_CreateObject();
}

cJSON*
debugger_handle_set_breakpoints(Debugger* /*dbg*/, const cJSON* /*params*/)
{
    return cJSON_CreateObject();
}

cJSON*
debugger_handle_set_simulation_speed(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_launch(Debugger* dbg, const cJSON* params)
{
    dbg->program_file_path = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(params, "program")));
    if (dbg->program_file_path == NULL)
    {
        print_debug("debugger_handle_launch: No filename field in input params");
        goto launch_error;
    }
    ParseResult result = parse_program_from_file(dbg->program_file_path);
    if (parse_result_nb_errors(&result) > 0)
    {
        print_debug("debugger_handle_launch: Failed to parse program");
        goto launch_error;
    }
    Program prog; 
    program_init_move(&prog, &result.program);
    parse_result_cleanup(&result);
    GridMap map; 
    grid_map_init(&map, map_settings_create_default(42));
    dbg->sim = simulation_create(simulation_settings_create_default(42), prog, map);

    const char* sim_speed = cJSON_GetStringValue(cJSON_GetObjectItem(params, "simulationSpeed"));
    if (sim_speed != NULL)
    {
        cJSON* sim_speed_arg = cJSON_CreateObject();
        cJSON_AddStringToObject(sim_speed_arg, "speed", sim_speed); 
        cJSON* resp = debugger_handle_set_simulation_speed(dbg, sim_speed_arg);
        cJSON_free(sim_speed_arg);
        if (resp) cJSON_free(resp); 
    }

    //Prepare the Initialized notification that will be sent after the response has been sent.
    dbg->pending_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(dbg->pending_notif, "type", "event");
    cJSON_AddStringToObject(dbg->pending_notif, "event", "initialized");

    return cJSON_CreateObject();
launch_error:
    //TODO: we don't have any other way of reporting an error here.
    return NULL;
}

cJSON*
debugger_handle_configuration_done(Debugger* dbg)
{
    //TODO: this is temporary for testing
    pthread_create(&dbg->sim_thread, NULL, debugger_simulation_runner, dbg);

    return cJSON_CreateObject();
}

cJSON*
debugger_handle_list_threads(Debugger* dbg)
{
    cJSON* resp = cJSON_CreateObject();
    cJSON* thread_list = cJSON_AddArrayToObject(resp, "threads");
    for (size_t i = 0; i < simulation_get_nb_ants(dbg->sim); i++)
    {
        cJSON* thread_item = cJSON_CreateObject();
        cJSON_AddItemToArray(thread_list, thread_item); 
        cJSON_AddNumberToObject(thread_item, "id", i);
        Ant* ant = simulation_get_ant(dbg->sim, i);
        char thread_name[100];
        const char* tag = simulation_get_tag_name(dbg->sim, ant->tag);
        snprintf(thread_name, 100, "%zd %s", i, tag);
        cJSON_AddStringToObject(thread_item, "name", thread_name);
    }
    return resp;
}

cJSON* 
debugger_handle_terminate(Debugger* dbg, const cJSON* /*params*/)
{
    // Stop the simulation, but don't stop the debugger
    // Note: there is a 'restart' param if we want to restart the simulation from the start that needs to be accounted for
    dbg->stop_sim = true;
    dbg->pause_sim = false; 
    sem_post(&dbg->pause_semaphore);
    if (dbg->program_file_path) free(dbg->program_file_path);
    dbg->program_file_path = NULL;
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_pause(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->pause_sim = true;
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_get_stack_trace(Debugger* dbg, const cJSON* params)
{
    size_t ant_id = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "threadId"));
    cJSON* result = cJSON_CreateObject(); 
    cJSON_AddNumberToObject(result, "totalFrames", 1);
    cJSON* stackFrames = cJSON_AddArrayToObject(result, "stackFrames");
    cJSON* frame = cJSON_CreateObject(); 
    cJSON_AddItemToArray(stackFrames, frame);
    cJSON_AddNumberToObject(frame, "id", ant_id);
    cJSON_AddStringToObject(frame, "name", "main");
    cJSON_AddNumberToObject(frame, "line", 3);
    cJSON_AddNumberToObject(frame, "column", 0);
    cJSON* source_obj = cJSON_AddObjectToObject(frame, "source");
    cJSON_AddStringToObject(source_obj, "path", dbg->program_file_path);

    return result;
}

cJSON* 
debugger_handle_get_scope(Debugger* dbg, const cJSON* params)
{
    const size_t ant_id = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "frameId"));
    cJSON* result = cJSON_CreateObject(); 
    cJSON* scopes_array = cJSON_AddArrayToObject(result, "scopes");
    //Adding a scope for registers
    cJSON* reg_scope = cJSON_CreateObject(); 
    cJSON_AddItemToArray(scopes_array, reg_scope);
    cJSON_AddStringToObject(reg_scope, "name", "Registers");
    cJSON_AddStringToObject(reg_scope, "presentationHint", "registers");
    //Note: variablesReference should start at 1, because 0 means 'there is no variable'.
    cJSON_AddNumberToObject(reg_scope, "variablesReference", ant_id + 1);
    cJSON_AddNumberToObject(reg_scope, "namedVariables", 9);
    cJSON_AddBoolToObject(reg_scope, "expensive", false);

    //Adding a scope for the state of the ant
    cJSON* state_scope = cJSON_CreateObject(); 
    cJSON_AddItemToArray(scopes_array, state_scope);
    cJSON_AddStringToObject(state_scope, "name", "State");
    cJSON_AddNumberToObject(state_scope, "variablesReference", ant_id + 1 + simulation_get_nb_ants(dbg->sim));
    cJSON_AddNumberToObject(reg_scope, "namedVariables", 4);
    cJSON_AddBoolToObject(reg_scope, "expensive", false);

    return result;
}

static
void 
add_int_variable(cJSON* var_array, const char* var_name, int32_t var_value)
{
    cJSON* var_data = cJSON_CreateObject(); 
    cJSON_AddItemToArray(var_array, var_data);
    cJSON_AddStringToObject(var_data, "name", var_name);
    char value_string[MAX_DIGITS_32BITS]; //Sufficient to write any 32-bit signed number.
    snprintf(value_string, sizeof(value_string), "%d", var_value);
    cJSON_AddStringToObject(var_data, "value", value_string);
    cJSON_AddNumberToObject(var_data, "variablesReference", 0);
}

static 
void 
add_string_variable(cJSON* var_array, const char* var_name, const char* var_value)
{
    cJSON* var_data = cJSON_CreateObject(); 
    cJSON_AddItemToArray(var_array, var_data);
    cJSON_AddStringToObject(var_data, "name", var_name);
    cJSON_AddStringToObject(var_data, "value", var_value);
    cJSON_AddNumberToObject(var_data, "variablesReference", 0);
}

static 
cJSON*
get_registers(Debugger* dbg, size_t ant_id)
{
    if (ant_id >= simulation_get_nb_ants(dbg->sim))
    {
        print_debug("Error: Ant id out of range");
        return NULL;
    }
    Ant* ant = simulation_get_ant(dbg->sim, ant_id);
    cJSON* resp = cJSON_CreateObject(); 
    cJSON* var_array = cJSON_AddArrayToObject(resp, "variables");

    //Program counter register
    add_int_variable(var_array, "pc", ant->pc);

    //The other registers
    for (int i = 0; i < 8; i++)
    {
        char reg_name[3];
        reg_name[0] = 'r';
        reg_name[1] = '0' + i;
        reg_name[2] = 0;
        add_int_variable(var_array, reg_name, ant->registers[i]);
    }
    return resp;
}

static 
cJSON*
get_state(Debugger* dbg, size_t ant_id)
{
    if (ant_id >= simulation_get_nb_ants(dbg->sim))
    {
        print_debug("Error: Ant id out of range");
        return NULL;
    }
    Ant* ant = simulation_get_ant(dbg->sim, ant_id);
    cJSON* resp = cJSON_CreateObject(); 
    cJSON* var_array = cJSON_AddArrayToObject(resp, "variables");

    add_int_variable(var_array, "id", (int32_t) ant->id);
    char pos_str[MAX_DIGITS_32BITS * 2 + 10];
    snprintf(pos_str, sizeof(pos_str), "x = %d, y = %d", (int32_t) ant->position.x, (int32_t) ant->position.y);
    add_string_variable(var_array, "position", pos_str);
    add_int_variable(var_array, "carrying", ant->carrying_food);
    add_string_variable(var_array, "tag", simulation_get_tag_name(dbg->sim, ant->tag));

    return resp;
}

cJSON* 
debugger_handle_get_variables(Debugger* dbg, const cJSON* params)
{
    const size_t var_refs = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "variablesReference"));
    
    if (var_refs <= simulation_get_nb_ants(dbg->sim))
    {
        //Note: -1 because variablesReference start counting at 1.
        return get_registers(dbg, var_refs - 1);
    }
    else
    {
        return get_state(dbg, var_refs - simulation_get_nb_ants(dbg->sim) - 1);
    }
}

cJSON*
debugger_handle_continue(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->pause_sim = false; 
    sem_post(&dbg->pause_semaphore);
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_set_simulation_speed(Debugger* dbg, const cJSON* params)
{
    const char* new_value = cJSON_GetStringValue(cJSON_GetObjectItem(params, "speed"));
    if (new_value == NULL)
    {
        print_debug("Missing argument 'speed' to setSimulationSpeed request");
        return NULL;
    }
    if (strcmp(new_value, "x1") == 0)
    {
        dbg->sim_speed = 1;
    }
    else if (strcmp(new_value, "x2") == 0)
    {
        dbg->sim_speed = 2;
    }
    else if (strcmp(new_value, "x5") == 0)
    {
        dbg->sim_speed = 5;
    }
    else if (strcmp(new_value, "x10") == 0)
    {
        dbg->sim_speed = 10;
    }
    else if (strcmp(new_value, "xMax") == 0)
    {
        dbg->sim_speed = -1;
    }
    else
    {
        print_debug("Invalid argument value 'speed' to setSimulationSpeed request");
        print_debug(new_value);
        return NULL;
    }
    return cJSON_CreateNull();
}

cJSON* 
debugger_handle_request(Debugger* dbg, const char* method, const cJSON* params)
{
    if (strcmp(method, "initialize") == 0)
    {
        return debugger_handle_initialize(dbg);
    }
    if (strcmp(method, "disconnect") == 0)
    {
        return debugger_handle_disconnect(dbg);
    }
    if (strcmp(method, "setBreakpoints") == 0 || 
        strcmp(method, "setInstructionBreakpoints") == 0)
    {
        return debugger_handle_set_breakpoints(dbg, params);
    }
    if (strcmp(method, "launch") == 0)
    {
        return debugger_handle_launch(dbg, params);
    }
    if (strcmp(method, "configurationDone") == 0)
    {
        return debugger_handle_configuration_done(dbg);
    }
    if (strcmp(method, "threads") == 0)
    {
        return debugger_handle_list_threads(dbg);
    }
    if (strcmp(method, "terminate") == 0)
    {
        return debugger_handle_terminate(dbg, params);
    }
    if (strcmp(method, "pause") == 0)
    {
        return debugger_handle_pause(dbg, params);
    }
    if (strcmp(method, "stackTrace") == 0)
    {
        return debugger_handle_get_stack_trace(dbg, params);
    }
    if (strcmp(method, "scopes") == 0)
    {
        return debugger_handle_get_scope(dbg, params);
    }
    if (strcmp(method, "variables") == 0)
    {
        return debugger_handle_get_variables(dbg, params);
    }
    if (strcmp(method, "continue") == 0)
    {
        return debugger_handle_continue(dbg, params);
    }
    if (strcmp(method, "setSimulationSpeed") == 0)
    {
        return debugger_handle_set_simulation_speed(dbg, params);
    }
    print_debug("No handler for request:");
    print_debug(method);
    return NULL;
}

void
debugger_handle_notification(Debugger* /*dbg*/, const char* method, const cJSON* /*params*/)
{
    print_debug("No handler for notification");
    print_debug(method);
}