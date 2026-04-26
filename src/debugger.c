#include "debugger.h"
#include "debugger_private.h"
#include "simulation.h"
#include "parser.h"
#include "json_rpc.h"
#include "utils.h"

void 
debugger_init(Debugger* debugger)
{
    debugger->sim_speed = 1; 
    debugger->stop_sim = false;
    debugger->pause_sim = false;
    debugger->exit_debugger = false;

    debugger->program_file_path = NULL; 
    debugger->pending_notif = NULL;
    debugger->sim = NULL; 

    sem_init(&debugger->pause_semaphore, 0, 0);
}


void
debugger_cleanup(Debugger* debugger)
{
    sem_destroy(&debugger->pause_semaphore);
    if (debugger->sim != NULL) 
    {
        simulation_delete(debugger->sim); 
        debugger->sim = NULL;
    }
    if (debugger->pending_notif) cJSON_free(debugger->pending_notif);
    if (debugger->program_file_path) free(debugger->program_file_path);
}

static
void
debugger_send_update(Debugger* dbg)
{
    cJSON* notif = cJSON_Parse("{\n"
        "\"type\": \"event\",\n"
        "\"event\": \"gridContent\"\n"
    "}");
    assert(notif != NULL);
    cJSON* sim_state = simulation_to_json(dbg->sim);
    cJSON* notif_body = cJSON_AddObjectToObject(notif, "body");
    cJSON_AddItemToObject(notif_body, "simulation", sim_state);
    const char* sim_speed_str = dbg->sim_speed == 1 ? "x1" : 
        dbg->sim_speed == 2 ? "x2" : 
        dbg->sim_speed == 5 ? "x5" : 
        dbg->sim_speed == 10 ? "x10" : 
        dbg->sim_speed == -1 ? "xMax" : 
        "x1";
    cJSON_AddStringToObject(notif_body, "speed", sim_speed_str);
    send_packet(notif, false);
}

void debugger_pause_sim(Debugger* dbg)
{
    cJSON* pause_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(pause_notif, "type", "event"); 
    cJSON_AddStringToObject(pause_notif, "event", "stopped");
    cJSON* notif_body = cJSON_AddObjectToObject(pause_notif, "body");
    cJSON_AddStringToObject(notif_body, "reason", "pause");
    cJSON_AddBoolToObject(notif_body, "allThreadsStopped", true);
    cJSON_AddNumberToObject(notif_body, "threadId", 0);
    send_packet(pause_notif, true);
    sem_wait(&dbg->pause_semaphore);
}

void* 
simulation_runner(void* arg) 
{
    Debugger* dbg = (Debugger*) arg;
    time_t last_sent_update; 
    time(&last_sent_update);
    debugger_send_update(arg);
    for (size_t i = 0; i < 2000; i++)
    {
        simulation_run_step(dbg->sim);
        if (dbg->stop_sim)
        {
            break;
        }
        if (dbg->sim_speed < 0)
        {
            time_t now; 
            time(&now); 
            if (now - last_sent_update > 0.1)
            {
                last_sent_update = now;
                debugger_send_update(dbg);
            }
        }
        else if (simulation_get_step_number(dbg->sim) % dbg->sim_speed == 0)
        {
            time_t now;
            time(&now);
            double diff_time = now - last_sent_update;
            last_sent_update = now;
            debugger_send_update(dbg);
            if (diff_time < 0.1)
            {
                usleep((0.1 - diff_time) * 1000 * 1000);
            }
        }
        if (dbg->pause_sim)
        {
            debugger_pause_sim(dbg);
        }
    }

    if (!dbg->stop_sim)
    {
        debugger_send_update(dbg);
        dbg->pause_sim = true; 
        debugger_pause_sim(dbg);
    }

    simulation_delete(dbg->sim);
    dbg->sim = NULL;

    cJSON* terminated_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(terminated_notif, "type", "event"); 
    cJSON_AddStringToObject(terminated_notif, "event", "terminated");
    send_packet(terminated_notif, true);
    return NULL;
}

static 
cJSON* handle_initialize(Debugger* /*dbg*/)
{
    //Return the server capabilities
    cJSON* capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(capabilities, "supportsTerminateRequest", true);
    //cJSON_AddBoolToObject(capabilities, "supportsRestartRequest", true);
    cJSON_AddBoolToObject(capabilities, "supportsInstructionBreakpoints", true);
    cJSON_AddBoolToObject(capabilities, "supportsConfigurationDoneRequest", true);
    return capabilities; 
}

static 
cJSON* 
handle_disconnect(Debugger* dbg)
{
    dbg->exit_debugger = true;
    sem_destroy(&dbg->pause_semaphore);
    return cJSON_CreateObject();
}

static
cJSON*
handle_set_breakpoints(Debugger* /*dbg*/, const cJSON* /*params*/)
{
    return cJSON_CreateObject();
}

static
cJSON*
handle_set_simulation_speed(Debugger* dbg, const cJSON* params);

static 
cJSON*
handle_launch(Debugger* dbg, const cJSON* params)
{
    dbg->program_file_path = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(params, "program")));
    if (dbg->program_file_path == NULL)
    {
        print_debug("handle_launch: No filename field in input params");
        goto launch_error;
    }
    ParseResult result = parse_program_from_file(dbg->program_file_path);
    if (parse_result_nb_errors(&result) > 0)
    {
        print_debug("handle_launch: Failed to parse program");
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
        cJSON* resp = handle_set_simulation_speed(dbg, sim_speed_arg);
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

static 
cJSON*
handle_configuration_done(Debugger* dbg)
{
    //TODO: this is temporary for testing
    pthread_create(&dbg->sim_thread, NULL, simulation_runner, dbg);

    return cJSON_CreateObject();
}

static
cJSON*
handle_list_threads(Debugger* dbg)
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

static 
cJSON* 
handle_terminate(Debugger* dbg, const cJSON* /*params*/)
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

static 
cJSON*
handle_pause(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->pause_sim = true;
    return cJSON_CreateNull();
}

static
cJSON*
handle_get_stack_trace(Debugger* dbg, const cJSON* params)
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

static 
cJSON* 
handle_get_scope(Debugger* dbg, const cJSON* params)
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
debugger_add_int_variable(cJSON* var_array, const char* var_name, int32_t var_value)
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
debugger_add_string_variable(cJSON* var_array, const char* var_name, const char* var_value)
{
    cJSON* var_data = cJSON_CreateObject(); 
    cJSON_AddItemToArray(var_array, var_data);
    cJSON_AddStringToObject(var_data, "name", var_name);
    cJSON_AddStringToObject(var_data, "value", var_value);
    cJSON_AddNumberToObject(var_data, "variablesReference", 0);
}

static 
cJSON*
debugger_get_registers(Debugger* dbg, size_t ant_id)
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
    debugger_add_int_variable(var_array, "pc", ant->pc);

    //The other registers
    for (int i = 0; i < 8; i++)
    {
        char reg_name[3];
        reg_name[0] = 'r';
        reg_name[1] = '0' + i;
        reg_name[2] = 0;
        debugger_add_int_variable(var_array, reg_name, ant->registers[i]);
    }
    return resp;
}

static 
cJSON*
debugger_get_state(Debugger* dbg, size_t ant_id)
{
    if (ant_id >= simulation_get_nb_ants(dbg->sim))
    {
        print_debug("Error: Ant id out of range");
        return NULL;
    }
    Ant* ant = simulation_get_ant(dbg->sim, ant_id);
    cJSON* resp = cJSON_CreateObject(); 
    cJSON* var_array = cJSON_AddArrayToObject(resp, "variables");

    debugger_add_int_variable(var_array, "id", (int32_t) ant->id);
    char pos_str[MAX_DIGITS_32BITS * 2 + 10];
    snprintf(pos_str, sizeof(pos_str), "x = %d, y = %d", (int32_t) ant->position.x, (int32_t) ant->position.y);
    debugger_add_string_variable(var_array, "position", pos_str);
    debugger_add_int_variable(var_array, "carrying", ant->carrying_food);
    debugger_add_string_variable(var_array, "tag", simulation_get_tag_name(dbg->sim, ant->tag));

    return resp;
}


static 
cJSON* 
handle_get_variables(Debugger* dbg, const cJSON* params)
{
    const size_t var_refs = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "variablesReference"));
    
    if (var_refs <= simulation_get_nb_ants(dbg->sim))
    {
        //Note: -1 because variablesReference start counting at 1.
        return debugger_get_registers(dbg, var_refs - 1);
    }
    else
    {
        return debugger_get_state(dbg, var_refs - simulation_get_nb_ants(dbg->sim) - 1);
    }
}

static
cJSON*
handle_continue(Debugger* dbg, const cJSON* /*params*/)
{
    dbg->pause_sim = false; 
    sem_post(&dbg->pause_semaphore);
    return cJSON_CreateNull();
}

static 
cJSON*
handle_set_simulation_speed(Debugger* dbg, const cJSON* params)
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

static
cJSON* 
handle_request(Debugger* dbg, const char* method, const cJSON* params)
{
    if (strcmp(method, "initialize") == 0)
    {
        return handle_initialize(dbg);
    }
    if (strcmp(method, "disconnect") == 0)
    {
        return handle_disconnect(dbg);
    }
    if (strcmp(method, "setBreakpoints") == 0 || 
        strcmp(method, "setInstructionBreakpoints") == 0)
    {
        return handle_set_breakpoints(dbg, params);
    }
    if (strcmp(method, "launch") == 0)
    {
        return handle_launch(dbg, params);
    }
    if (strcmp(method, "configurationDone") == 0)
    {
        return handle_configuration_done(dbg);
    }
    if (strcmp(method, "threads") == 0)
    {
        return handle_list_threads(dbg);
    }
    if (strcmp(method, "terminate") == 0)
    {
        return handle_terminate(dbg, params);
    }
    if (strcmp(method, "pause") == 0)
    {
        return handle_pause(dbg, params);
    }
    if (strcmp(method, "stackTrace") == 0)
    {
        return handle_get_stack_trace(dbg, params);
    }
    if (strcmp(method, "scopes") == 0)
    {
        return handle_get_scope(dbg, params);
    }
    if (strcmp(method, "variables") == 0)
    {
        return handle_get_variables(dbg, params);
    }
    if (strcmp(method, "continue") == 0)
    {
        return handle_continue(dbg, params);
    }
    if (strcmp(method, "setSimulationSpeed") == 0)
    {
        return handle_set_simulation_speed(dbg, params);
    }
    print_debug("No handler for request:");
    print_debug(method);
    return NULL;
}

static
void
handle_notification(Debugger* /*dbg*/, const char* method, const cJSON* /*params*/)
{
    print_debug("No handler for notification");
    print_debug(method);
}

void 
run_debugger(void)
{
    init_logging("/tmp/antdbg.log");
    print_debug("antide dbg STARTED");

    Debugger dbg; 
    debugger_init(&dbg);
    
    while (!dbg.exit_debugger)
    {
        cJSON* message = wait_for_message(); 
        if (message == NULL)
        {
            print_debug("No packet received, closing");
            break;
        }
        const char* message_type = cJSON_GetStringValue(cJSON_GetObjectItem(message, "type"));
        if (message_type == NULL)
        {
            print_debug("Could not retrieve message type");
            goto loop_iter_end;
        }

        const bool is_notif = strcmp(message_type, "event") == 0;
        const bool is_request = strcmp(message_type, "request") == 0;

        if (is_notif)
        {
            const char* method_name = cJSON_GetStringValue(cJSON_GetObjectItem(message, "event"));
            cJSON* params = cJSON_GetObjectItem(message, "body");
            if (method_name == NULL)
            {
                print_debug("Got a notification without 'event' field");
                goto loop_iter_end;
            }
            handle_notification(&dbg, method_name, params);
        }
        else if (is_request)
        {
            const char* method_name = cJSON_GetStringValue(cJSON_GetObjectItem(message, "command"));
            cJSON* params = cJSON_GetObjectItem(message, "arguments");
            if (method_name == NULL)
            {
                print_debug("Got a request without 'command' field");
                goto loop_iter_end;
            }

            cJSON* response_body = handle_request(&dbg, method_name, params);
            cJSON* resp = cJSON_CreateObject(); 
            cJSON_AddItemToObject(resp, "request_seq", cJSON_DetachItemFromObject(message, "seq"));
            cJSON_AddBoolToObject(resp, "success", response_body != NULL);
            cJSON_AddStringToObject(resp, "command", method_name);
            cJSON_AddStringToObject(resp, "type", "response");
            if (response_body != NULL)
            {
                cJSON_AddItemToObject(resp, "body", response_body);
            }
            else
            {
                cJSON_AddStringToObject(resp, "body", "Internal error");
            }
            print_debug("Sending response");
            send_packet(resp, true);
        }
        else 
        {
            print_debug("Unhandled message type, skipping");
            print_debug(message_type);
            goto loop_iter_end;
        }
        if (dbg.pending_notif != NULL) 
        {
            print_debug("Sending pending event");
            send_packet(dbg.pending_notif, true); 
            dbg.pending_notif = NULL;
        }

loop_iter_end:
        if (message) cJSON_free(message);
    }

    debugger_cleanup(&dbg);
    print_debug("antide dbg CLOSED");
    cleanup_logging();
}