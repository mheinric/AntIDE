#include "debugger.h"
#include "debugger_private.h"
#include "debugger_handlers.h"
#include "simulation.h"
#include "parser.h"
#include "json_rpc.h"
#include "utils.h"

#define UPDATE_WAIT_TIME 0.05

void 
debugger_init(Debugger* debugger)
{
    debugger->sim_speed = 1; 
    debugger->state = DBG_START;

    debugger->program_file_path = NULL; 
    debugger->map_data = NULL; 
    debugger->map_settings = map_settings_create_default(42);
    debugger->sim = NULL; 

    pthread_mutex_init(&debugger->sim_mutex, NULL);
    sem_init(&debugger->pause_semaphore, 0, 0);
    debugger->last_stop_ant = 0;
    debugger->target_step_nb = 2000;
}


void
debugger_cleanup(Debugger* debugger)
{
    pthread_join(debugger->sim_thread, NULL);
    pthread_mutex_destroy(&debugger->sim_mutex);
    sem_destroy(&debugger->pause_semaphore);
    if (debugger->sim != NULL) 
    {
        simulation_delete(debugger->sim); 
        debugger->sim = NULL;
    }
    if (debugger->program_file_path) free(debugger->program_file_path);
    if (debugger->map_data) free(debugger->map_data);
}

bool 
debugger_init_simulation(Debugger *dbg)
{
    if (dbg->sim != NULL) 
    {
        simulation_delete(dbg->sim);
        dbg->sim = NULL;   
    }
    ParseResult result = parse_program_from_file(dbg->program_file_path);
    if (parse_result_nb_errors(&result) > 0)
    {
        print_debug("debugger_init_simulation: Failed to parse program");
        return false;
    }
    Program prog; 
    program_init_move(&prog, &result.program);
    parse_result_cleanup(&result);
    GridMap map; 
    if (dbg->map_data != NULL)
    {
        grid_map_init_from_data(&map, dbg->map_data);
    }
    else
    {
        grid_map_init(&map, dbg->map_settings);
    }
    dbg->sim = simulation_create(simulation_settings_create_default(42), prog, map);
    return true;
}

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
    cJSON_AddNumberToObject(notif_body, "last_stop_ant", debugger_is_paused(dbg) ? (int) dbg->last_stop_ant : -1);
    
    send_packet(notif, false);
}

void 
debugger_send_pause_notif(const char* reason, size_t ant_id)
{
    cJSON* pause_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(pause_notif, "type", "event"); 
    cJSON_AddStringToObject(pause_notif, "event", "stopped");
    cJSON* notif_body = cJSON_AddObjectToObject(pause_notif, "body");
    cJSON_AddStringToObject(notif_body, "reason", reason);
    cJSON_AddBoolToObject(notif_body, "allThreadsStopped", true);
    cJSON_AddNumberToObject(notif_body, "threadId", ant_id);
    send_packet(pause_notif, true);
}

void* 
debugger_simulation_runner(void* arg) 
{
    Debugger* dbg = (Debugger*) arg;
    struct timespec last_sent_update; 
    timespec_get(&last_sent_update, TIME_UTC);
    debugger_send_update(arg);
    while (dbg->state != DBG_STOP && dbg->state != DBG_EXIT)
    {
        switch(dbg->state)
        {
            case DBG_START:
            {
                // Send a notif that the simulation thread has started
                cJSON* init_notif = cJSON_CreateObject(); 
                cJSON_AddStringToObject(init_notif, "type", "event");
                cJSON_AddStringToObject(init_notif, "event", "initialized");
                send_packet(init_notif, true);
                sem_wait(&dbg->pause_semaphore);
                continue;
            }
            case DBG_RUN:
            {
                pthread_mutex_lock(&dbg->sim_mutex);
                simulation_run_step(dbg->sim);
                if (simulation_stopped_on_breakpoint(dbg->sim))
                {
                    dbg->state = DBG_PAUSE;
                    dbg->last_stop_ant = simulation_get_next_running_ant(dbg->sim);
                    debugger_send_pause_notif("breakpoint", dbg->last_stop_ant);
                    debugger_send_update(dbg);
                    pthread_mutex_unlock(&dbg->sim_mutex);
                    sem_wait(&dbg->pause_semaphore);
                    pthread_mutex_lock(&dbg->sim_mutex);
                }
                if (simulation_get_step_number(dbg->sim) >= 2000)
                {
                    dbg->state = DBG_PAUSE;
                }
                pthread_mutex_unlock(&dbg->sim_mutex);
                break;
            }
            case DBG_FAST_SIM: 
            {
                pthread_mutex_lock(&dbg->sim_mutex);
                if (simulation_get_step_number(dbg->sim) > dbg->target_step_nb)
                {
                    debugger_init_simulation(dbg);
                }

                if (simulation_get_step_number(dbg->sim) == dbg->target_step_nb)
                {
                    dbg->state = DBG_PAUSE;
                }
                else
                {
                    simulation_run_step(dbg->sim);
                }
                pthread_mutex_unlock(&dbg->sim_mutex);
                break;
            }
            case DBG_STEP_OUT:
            case DBG_STEP:
            {
                pthread_mutex_lock(&dbg->sim_mutex);
                bool sim_others = false;
                while(dbg->last_stop_ant != simulation_get_next_running_ant(dbg->sim) && 
                (dbg->state == DBG_STEP || dbg->state == DBG_STEP_OUT))
                {
                    //If this is not the turn to the ant we are currently debugging, then we simulate the other ants.
                    sim_others = true;
                    simulation_run_single_instruction(dbg->sim);
                    if (simulation_stopped_on_breakpoint(dbg->sim))
                    {
                        //If one of the other ants hits a breakpoint, we stop the loop
                        break;
                    }
                }
                if (sim_others && simulation_stopped_on_breakpoint(dbg->sim))
                {
                    //One of the other ants hit a breakpoint in the loop above, so we don't simulate any additional instructions.
                    dbg->last_stop_ant = simulation_get_next_running_ant(dbg->sim);
                }
                else
                {
                    if (dbg->state == DBG_STEP)
                    {
                        //Run a single instruction, ignoring breakpoints
                        simulation_run_single_instruction(dbg->sim);
                        if (simulation_stopped_on_breakpoint(dbg->sim))
                        {
                            //We hit a breakpoint, so we actually have 
                            //to run the instruction again to actually execute it
                            simulation_run_single_instruction(dbg->sim);
                        }
                    }
                    else
                    {
                        //Run all the instructions for this ant, stopping on breakpoints
                        while (simulation_get_next_running_ant(dbg->sim) == dbg->last_stop_ant)
                        {
                            simulation_run_single_instruction(dbg->sim);
                            if (simulation_stopped_on_breakpoint(dbg->sim))
                            {
                                break;
                            }
                        }
                    }
                }
                debugger_send_pause_notif("step", dbg->last_stop_ant);
                debugger_send_update(dbg);
                pthread_mutex_unlock(&dbg->sim_mutex);
                sem_wait(&dbg->pause_semaphore);
                break;
            }
            case DBG_PAUSE:
            {
                pthread_mutex_lock(&dbg->sim_mutex);
                debugger_send_pause_notif("pause", dbg->last_stop_ant);
                debugger_send_update(dbg);
                pthread_mutex_unlock(&dbg->sim_mutex);
                sem_wait(&dbg->pause_semaphore);
                break;
            } 
            case DBG_STOP:
            case DBG_EXIT:
            {
                break;
            }
        }
        if (dbg->state == DBG_RUN)
        {
            if (dbg->sim_speed < 0)
            {
                struct timespec now; 
                timespec_get(&now, TIME_UTC); 
                if (timespec_diff(now, last_sent_update) > UPDATE_WAIT_TIME)
                {
                    last_sent_update = now;
                    pthread_mutex_lock(&dbg->sim_mutex);
                    debugger_send_update(dbg);
                    pthread_mutex_unlock(&dbg->sim_mutex);
                }
            }
            else if (simulation_get_step_number(dbg->sim) % dbg->sim_speed == 0)
            {
                struct timespec now;
                timespec_get(&now, TIME_UTC);
                double diff_time = timespec_diff(now, last_sent_update);
                last_sent_update = now;
                pthread_mutex_lock(&dbg->sim_mutex);
                debugger_send_update(dbg);
                pthread_mutex_unlock(&dbg->sim_mutex);
                if (diff_time < UPDATE_WAIT_TIME)
                {
                    usleep((UPDATE_WAIT_TIME - diff_time) * 1000 * 1000);
                }
            }
        }
    }

    pthread_mutex_lock(&dbg->sim_mutex);
    simulation_delete(dbg->sim);
    dbg->sim = NULL;
    pthread_mutex_unlock(&dbg->sim_mutex);

    cJSON* terminated_notif = cJSON_CreateObject(); 
    cJSON_AddStringToObject(terminated_notif, "type", "event"); 
    cJSON_AddStringToObject(terminated_notif, "event", "terminated");
    send_packet(terminated_notif, true);
    return NULL;
}

void 
run_debugger(void)
{
    init_logging("/tmp/antdbg.log");
    print_debug("antide dbg STARTED");
    json_rpc_init();

    Debugger dbg; 
    debugger_init(&dbg);
    
    while (dbg.state != DBG_EXIT)
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
            debugger_handle_notification(&dbg, method_name, params);
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

            cJSON* response_body = debugger_handle_request(&dbg, method_name, params);
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

loop_iter_end:
        if (message) cJSON_free(message);
    }

    debugger_cleanup(&dbg);
    json_rpc_cleanup();
    print_debug("antide dbg CLOSED");
    cleanup_logging();
}

bool 
debugger_is_paused(const Debugger* dbg)
{
    switch(dbg->state)
    {
        case DBG_STEP:
        case DBG_STEP_OUT:
        case DBG_PAUSE:
        {
            return true;
        }
        case DBG_START:
        case DBG_FAST_SIM:
        case DBG_RUN:
        case DBG_STOP:
        case DBG_EXIT:
        {
            return false;
        }
    }
    return false;
}