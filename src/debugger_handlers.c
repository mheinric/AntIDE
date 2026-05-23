#include "debugger_handlers.h"
#include "parser.h"
#include "json_rpc.h"

cJSON* debugger_handle_initialize(Debugger* /*dbg*/)
{
    //Return the server capabilities
    cJSON* capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(capabilities, "supportsTerminateRequest", true);
    cJSON_AddBoolToObject(capabilities, "supportsConfigurationDoneRequest", true);
    return capabilities; 
}

cJSON* 
debugger_handle_disconnect(Debugger* dbg)
{
    dbg->state = DBG_EXIT;
    return cJSON_CreateObject();
}

cJSON*
debugger_handle_set_breakpoints(Debugger* dbg, const cJSON* params)
{
    cJSON* breakpoints = cJSON_GetObjectItem(params, "breakpoints");
    pthread_mutex_lock(&dbg->sim_mutex);
    Program* prog = simulation_get_program(dbg->sim);
    program_clear_breakpoints(prog); 
    size_t nb_breakpoints = cJSON_GetArraySize(breakpoints); 

    cJSON* result_bp = cJSON_CreateArray();
    for (size_t i = 0; i < nb_breakpoints; i++)
    {
        size_t line_nb = cJSON_GetNumberValue(
        cJSON_GetObjectItem(cJSON_GetArrayItem(breakpoints, i), "line"));
        size_t inst_index = program_get_instruction_index(prog, line_nb); 
        program_set_breakpoint(prog, inst_index);
        size_t actual_line_nb = program_get_source_line(prog, inst_index);


        cJSON* bp = cJSON_CreateObject(); 
        cJSON_AddBoolToObject(bp, "verified", true);
        cJSON_AddNumberToObject(bp, "line", actual_line_nb);
        cJSON_AddItemToArray(result_bp, bp);
    }
    pthread_mutex_unlock(&dbg->sim_mutex);

    cJSON* resp = cJSON_CreateObject(); 
    cJSON_AddItemToObject(resp, "breakpoints", result_bp);

    return resp;
}

cJSON*
debugger_handle_launch(Debugger* dbg, const cJSON* params)
{
    const char* program = cJSON_GetStringValue(cJSON_GetObjectItem(params, "program"));
    if (program == NULL)
    {
        print_debug("debugger_handle_launch: Missing program field");
        return NULL;
    }
    dbg->program_file_path = strdup(program);
    pthread_mutex_lock(&dbg->sim_mutex);
    if (dbg->program_file_path == NULL)
    {
        print_debug("debugger_handle_launch: No filename field in input params");
        goto launch_error;
    }
    const char* map_data = cJSON_GetStringValue(cJSON_GetObjectItem(params, "map"));
    if (map_data != NULL)
    {
        if (dbg->map_data != NULL) free(dbg->map_data);
        dbg->map_data = NULL;
        if (!map_type_deserialization(map_data, &dbg->map_settings.map_type)) {
            dbg->map_data = strdup(map_data);
        }
    }

    if (!debugger_init_simulation(dbg))
    {
        print_debug("debugger_handle_launch: Failed to init sim");
        goto launch_error;
    }

    const char* sim_speed = cJSON_GetStringValue(cJSON_GetObjectItem(params, "simulationSpeed"));
    if (sim_speed != NULL)
    {
        cJSON* sim_speed_arg = cJSON_CreateObject();
        cJSON_AddStringToObject(sim_speed_arg, "speed", sim_speed); 
        cJSON* resp = debugger_handle_set_simulation_speed(dbg, sim_speed_arg);
        cJSON_free(sim_speed_arg);
        if (resp) cJSON_free(resp); 
    }
    pthread_create(&dbg->sim_thread, NULL, debugger_simulation_runner, dbg);
    pthread_mutex_unlock(&dbg->sim_mutex);

    return cJSON_CreateObject();
launch_error:
    //TODO: we don't have any other way of reporting an error here.
    pthread_mutex_unlock(&dbg->sim_mutex);
    return NULL;
}

cJSON*
debugger_handle_configuration_done(Debugger* dbg)
{
    dbg->state = DBG_RUN;
    sem_post(&dbg->pause_semaphore);

    return cJSON_CreateObject();
}

cJSON*
debugger_handle_list_threads(Debugger* dbg)
{
    pthread_mutex_lock(&dbg->sim_mutex);
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
    pthread_mutex_unlock(&dbg->sim_mutex);
    return resp;
}

cJSON* 
debugger_handle_terminate(Debugger* dbg, const cJSON* /*params*/)
{
    // Stop the simulation, but don't stop the debugger
    // Note: there is a 'restart' param if we want to restart the simulation from the start that needs to be accounted for
    dbg->state = DBG_STOP;
    sem_post(&dbg->pause_semaphore);
    if (dbg->program_file_path) free(dbg->program_file_path);
    dbg->program_file_path = NULL;
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_pause(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->state = DBG_PAUSE;
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_get_stack_trace(Debugger* dbg, const cJSON* params)
{
    pthread_mutex_lock(&dbg->sim_mutex);
    size_t ant_id = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "threadId"));
    cJSON* result = cJSON_CreateObject(); 
    cJSON_AddNumberToObject(result, "totalFrames", 1);
    cJSON* stackFrames = cJSON_AddArrayToObject(result, "stackFrames");
    cJSON* frame = cJSON_CreateObject(); 
    cJSON_AddItemToArray(stackFrames, frame);
    cJSON_AddNumberToObject(frame, "id", ant_id);
    cJSON_AddStringToObject(frame, "name", "main");
    if (ant_id >= simulation_get_nb_ants(dbg->sim))
    {
        print_debug("Ant index out of range");
        pthread_mutex_unlock(&dbg->sim_mutex);
        return NULL;
    }
    Ant* ant = simulation_get_ant(dbg->sim, ant_id);
    size_t inst_index = (size_t) ant->pc;
    cJSON_AddNumberToObject(frame, "line", 
        program_get_source_line(simulation_get_program(dbg->sim), inst_index));
    cJSON_AddNumberToObject(frame, "column", 0);
    cJSON* source_obj = cJSON_AddObjectToObject(frame, "source");
    cJSON_AddStringToObject(source_obj, "path", dbg->program_file_path);

    pthread_mutex_unlock(&dbg->sim_mutex);
    return result;
}

cJSON* 
debugger_handle_get_scope(Debugger* dbg, const cJSON* params)
{
    pthread_mutex_lock(&dbg->sim_mutex);
    const size_t ant_id = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "frameId"));
    if (ant_id != dbg->last_stop_ant)
    {
        //This is a hack to prevent VSCode from showing the current lines for *all* 
        //the ants the user clicked on at the same time: 
        //get_scope is called when the user clicks on the stacktrace for one ant.
        //by sending this notif, we effectively invalidate the previous stacktrace,
        //which clears the marker from the view.
        dbg->last_stop_ant = ant_id; 
        debugger_send_pause_notif(dbg->state == DBG_PAUSE ? "pause" : "step", dbg->last_stop_ant);
        debugger_send_update(dbg);
    }
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
    cJSON_AddNumberToObject(state_scope, "namedVariables", 4);
    cJSON_AddBoolToObject(state_scope, "expensive", false);

    pthread_mutex_unlock(&dbg->sim_mutex);
    return result;
}

static
void 
add_int_variable(cJSON* var_array, const char* var_name, int32_t var_value, bool add_hex)
{
    cJSON* var_data = cJSON_CreateObject(); 
    cJSON_AddItemToArray(var_array, var_data);
    cJSON_AddStringToObject(var_data, "name", var_name);
    char value_string[MAX_DIGITS_32BITS + MAX_HEX_DIGITS_32BITS + 3]; //Sufficient to write any 32-bit signed number + its hex representation in parenthesis
    if (add_hex) {
        const uint8_t byte0 = (uint8_t) (var_value & 0xFF);
        const uint8_t byte1 = (uint8_t) ((var_value >> 8) & 0xFF);
        const uint8_t byte2 = (uint8_t) ((var_value >> 16) & 0xFF);
        const uint8_t byte3 = (uint8_t) ((var_value >> 24) & 0xFF);
        snprintf(value_string, sizeof(value_string), "%d (0x%02x_%02x_%02x_%02x)", var_value, byte3, byte2, byte1, byte0);
    }
    else {
        snprintf(value_string, sizeof(value_string), "%d", var_value);
    }
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
    add_int_variable(var_array, "pc", ant->pc, false);

    //The other registers
    const Program* prog = simulation_get_program(dbg->sim);
    for (int i = 0; i < 8; i++)
    {
        add_int_variable(var_array, program_get_register_name(prog, i), ant->registers[i], true);
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

    add_int_variable(var_array, "id", (int32_t) ant->id, false);
    char pos_str[MAX_DIGITS_32BITS * 2 + 10];
    snprintf(pos_str, sizeof(pos_str), "x = %d, y = %d", (int32_t) ant->position.x, (int32_t) ant->position.y);
    add_string_variable(var_array, "position", pos_str);
    add_int_variable(var_array, "carrying", ant->carrying_food, false);
    add_string_variable(var_array, "tag", simulation_get_tag_name(dbg->sim, ant->tag));

    return resp;
}

cJSON* 
debugger_handle_get_variables(Debugger* dbg, const cJSON* params)
{
    const size_t var_refs = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "variablesReference"));
    pthread_mutex_lock(&dbg->sim_mutex);
    cJSON* res;
    if (var_refs <= simulation_get_nb_ants(dbg->sim))
    {
        //Note: -1 because variablesReference start counting at 1.
        res = get_registers(dbg, var_refs - 1);
    }
    else
    {
        res = get_state(dbg, var_refs - simulation_get_nb_ants(dbg->sim) - 1);
    }
    pthread_mutex_unlock(&dbg->sim_mutex);
    return res;
}

cJSON*
debugger_handle_continue(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->state = DBG_RUN; 
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

cJSON *
debugger_handle_step_out(Debugger *dbg)
{
    dbg->state = DBG_STEP_OUT;
    sem_post(&dbg->pause_semaphore);
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_step(Debugger *dbg)
{
    dbg->state = DBG_STEP;
    sem_post(&dbg->pause_semaphore);
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_set_current_step(Debugger* dbg, const cJSON* params)
{
    bool paused = debugger_is_paused(dbg);
    dbg->state = DBG_FAST_SIM;
    dbg->target_step_nb = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "step"));
    if (paused)
    {
        sem_post(&dbg->pause_semaphore);
    }
    return cJSON_CreateNull();
}

cJSON*
debugger_handle_set_current_ant(Debugger* dbg, const cJSON* params)
{
    size_t ant_id = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "id"));
    dbg->last_stop_ant = ant_id;
    debugger_send_pause_notif(dbg->state == DBG_PAUSE ? "pause" : "step", dbg->last_stop_ant);
    pthread_mutex_lock(&dbg->sim_mutex);
    debugger_send_update(dbg);
    pthread_mutex_unlock(&dbg->sim_mutex);

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
    if (strcmp(method, "setBreakpoints") == 0)
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
    if (strcmp(method, "stepOut") == 0)
    {
        return debugger_handle_step_out(dbg);
    }
    if (strcmp(method, "stepIn") == 0 || strcmp(method, "next") == 0)
    {
        return debugger_handle_step(dbg);
    }
    if (strcmp(method, "setCurrentStep") == 0)
    {
        return debugger_handle_set_current_step(dbg, params);
    }
    if (strcmp(method, "setCurrentAnt") == 0)
    {
        return debugger_handle_set_current_ant(dbg, params);
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