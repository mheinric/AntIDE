#include "debugger.h"
#include "simulation.h"
#include "parser.h"
#include "json_rpc.h"
#include "utils.h"
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

static bool STOP_SIM = false;
static bool PAUSE_SIM = false;
static bool EXIT_DEBUGGER = false;

static char* PROGRAM_FILE_PATH = NULL;
static cJSON* PENDING_NOTIF = NULL;
static Simulation* SIM = NULL;
static pthread_t SIM_THREAD;
sem_t PAUSE_SIM_SEMAPHORE;

void
send_update()
{
    cJSON* notif = cJSON_Parse("{\n"
        "\"type\": \"event\",\n"
        "\"event\": \"gridContent\"\n"
    "}");
    assert(notif != NULL);
    cJSON* sim_state = simulation_to_json(SIM);
    cJSON_AddItemToObject(cJSON_AddObjectToObject(notif, "body"), "simulation", sim_state);
    send_packet(notif, false);
}

void* 
simulation_runner(void*) 
{
    for (size_t i = 0; i < 2000; i++)
    {
        simulation_run_step(SIM);
        if (STOP_SIM)
        {
            break;
        }
        send_update();
        usleep(100 * 1000);
        if (PAUSE_SIM)
        {
            cJSON* pause_notif = cJSON_CreateObject(); 
            cJSON_AddStringToObject(pause_notif, "type", "event"); 
            cJSON_AddStringToObject(pause_notif, "event", "stopped");
            cJSON* notif_body = cJSON_AddObjectToObject(pause_notif, "body");
            cJSON_AddStringToObject(notif_body, "reason", "pause");
            cJSON_AddBoolToObject(notif_body, "allThreadsStopped", true);
            cJSON_AddNumberToObject(notif_body, "threadId", 0);
            send_packet(pause_notif, true);
            sem_wait(&PAUSE_SIM_SEMAPHORE);
        }
    }
    simulation_delete(SIM);
    SIM = NULL;

    cJSON* terminated_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(terminated_notif, "type", "event"); 
    cJSON_AddStringToObject(terminated_notif, "event", "terminated");
    send_packet(terminated_notif, true);
    return NULL;
}

static 
cJSON* handle_initialize()
{
    sem_init(&PAUSE_SIM_SEMAPHORE, 0, 0);
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
handle_disconnect()
{
    EXIT_DEBUGGER = true;
    sem_destroy(&PAUSE_SIM_SEMAPHORE);
    return cJSON_CreateObject();
}

static
cJSON*
handle_set_breakpoints(const cJSON* /*params*/)
{
    return cJSON_CreateObject();
}

static 
cJSON*
handle_launch(const cJSON* params)
{
    PROGRAM_FILE_PATH = strdup(cJSON_GetStringValue(cJSON_GetObjectItem(params, "program")));
    if (PROGRAM_FILE_PATH == NULL)
    {
        print_debug("handle_launch: No filename field in input params");
        goto launch_error;
    }
    ParseResult result = parse_program_from_file(PROGRAM_FILE_PATH);
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
    SIM = simulation_create(simulation_settings_create_default(42), prog, map);

    //Prepare the Initialized notification that will be sent after the response has been sent.
    PENDING_NOTIF = cJSON_CreateObject(); 
    cJSON_AddStringToObject(PENDING_NOTIF, "type", "event");
    cJSON_AddStringToObject(PENDING_NOTIF, "event", "initialized");

    return cJSON_CreateObject();
launch_error:
    //TODO: we don't have any other way of reporting an error here.
    return NULL;
}

static 
cJSON*
handle_configuration_done()
{
    //TODO: this is temporary for testing
    pthread_create(&SIM_THREAD, NULL, simulation_runner, NULL);

    return cJSON_CreateObject();
}

static
cJSON*
handle_list_threads()
{
    cJSON* resp = cJSON_CreateObject();
    cJSON* thread_list = cJSON_AddArrayToObject(resp, "threads");
    for (size_t i = 0; i < simulation_get_nb_ants(SIM); i++)
    {
        cJSON* thread_item = cJSON_CreateObject();
        cJSON_AddItemToArray(thread_list, thread_item); 
        cJSON_AddNumberToObject(thread_item, "id", i);
        Ant* ant = simulation_get_ant(SIM, i);
        char thread_name[100];
        const char* tag = simulation_get_tag_name(SIM, ant->tag);
        snprintf(thread_name, 100, "%zd %s", i, tag);
        cJSON_AddStringToObject(thread_item, "name", thread_name);
    }
    return resp;
}

static 
cJSON* 
handle_terminate(const cJSON* /*params*/)
{
    // Stop the simulation, but don't stop the debugger
    // Note: there is a 'restart' param if we want to restart the simulation from the start that needs to be accounted for
    STOP_SIM = true;
    PAUSE_SIM = false; 
    sem_post(&PAUSE_SIM_SEMAPHORE);
    if (PROGRAM_FILE_PATH) free(PROGRAM_FILE_PATH);
    PROGRAM_FILE_PATH = NULL;
    return cJSON_CreateNull();
}

static 
cJSON*
handle_pause(const cJSON* /*params*/)
{
    PAUSE_SIM = true;
    return cJSON_CreateNull();
}

static
cJSON*
handle_get_stack_trace(const cJSON* params)
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
    cJSON_AddStringToObject(source_obj, "path", PROGRAM_FILE_PATH);

    return result;
}

static 
cJSON* 
handle_get_scope(const cJSON* params)
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
    cJSON_AddNumberToObject(state_scope, "variablesReference", ant_id + 1 + simulation_get_nb_ants(SIM));
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
debugger_get_registers(size_t ant_id)
{
    if (ant_id >= simulation_get_nb_ants(SIM))
    {
        print_debug("Error: Ant id out of range");
        return NULL;
    }
    Ant* ant = simulation_get_ant(SIM, ant_id);
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
debugger_get_state(size_t ant_id)
{
    if (ant_id >= simulation_get_nb_ants(SIM))
    {
        print_debug("Error: Ant id out of range");
        return NULL;
    }
    Ant* ant = simulation_get_ant(SIM, ant_id);
    cJSON* resp = cJSON_CreateObject(); 
    cJSON* var_array = cJSON_AddArrayToObject(resp, "variables");

    debugger_add_int_variable(var_array, "id", (int32_t) ant->id);
    char pos_str[MAX_DIGITS_32BITS * 2 + 10];
    snprintf(pos_str, sizeof(pos_str), "x = %d, y = %d", (int32_t) ant->position.x, (int32_t) ant->position.y);
    debugger_add_string_variable(var_array, "position", pos_str);
    debugger_add_int_variable(var_array, "carrying", ant->carrying_food);
    debugger_add_string_variable(var_array, "tag", simulation_get_tag_name(SIM, ant->tag));

    return resp;
}


static 
cJSON* 
handle_get_variables(const cJSON* params)
{
    //Note: -1 because variablesReference start counting at 1.
    const size_t var_refs = cJSON_GetNumberValue(cJSON_GetObjectItem(params, "variablesReference"));

    if (var_refs <= simulation_get_nb_ants(SIM))
    {
        return debugger_get_registers(var_refs - 1);
    }
    else
    {
        return debugger_get_state(var_refs - simulation_get_nb_ants(SIM) - 1);
    }
}

static
cJSON*
handle_continue(const cJSON* /*params*/)
{
    PAUSE_SIM = false; 
    sem_post(&PAUSE_SIM_SEMAPHORE);
    return cJSON_CreateNull();
}

static
cJSON* 
handle_request(const char* method, const cJSON* params)
{
    if (strcmp(method, "initialize") == 0)
    {
        return handle_initialize();
    }
    if (strcmp(method, "disconnect") == 0)
    {
        return handle_disconnect();
    }
    if (strcmp(method, "setBreakpoints") == 0 || 
        strcmp(method, "setInstructionBreakpoints") == 0)
    {
        return handle_set_breakpoints(params);
    }
    if (strcmp(method, "launch") == 0)
    {
        return handle_launch(params);
    }
    if (strcmp(method, "configurationDone") == 0)
    {
        return handle_configuration_done();
    }
    if (strcmp(method, "threads") == 0)
    {
        return handle_list_threads();
    }
    if (strcmp(method, "terminate") == 0)
    {
        return handle_terminate(params);
    }
    if (strcmp(method, "pause") == 0)
    {
        return handle_pause(params);
    }
    if (strcmp(method, "stackTrace") == 0)
    {
        return handle_get_stack_trace(params);
    }
    if (strcmp(method, "scopes") == 0)
    {
        return handle_get_scope(params);
    }
    if (strcmp(method, "variables") == 0)
    {
        return handle_get_variables(params);
    }
    if (strcmp(method, "continue") == 0)
    {
        return handle_continue(params);
    }
    print_debug("No handler for request:");
    print_debug(method);
    return NULL;
}

static
void
handle_notification(const char* /*method*/, const cJSON* /*params*/)
{

}

void 
run_debugger(void)
{
    init_logging("/tmp/antdbg.log");
    print_debug("antide dbg STARTED");
    
    while (!EXIT_DEBUGGER)
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
            handle_notification(method_name, params);
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

            cJSON* response_body = handle_request(method_name, params);
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
        if (PENDING_NOTIF != NULL) 
        {
            print_debug("Sending pending event");
            send_packet(PENDING_NOTIF, true); 
            PENDING_NOTIF = NULL;
        }

loop_iter_end:
        if (message) cJSON_free(message);
    }

    print_debug("antide dbg CLOSED");
    cleanup_logging();
}