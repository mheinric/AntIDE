#include "debugger.h"
#include "debugger_private.h"
#include "debugger_handlers.h"
#include "simulation.h"
#include "parser.h"
#include "json_rpc.h"
#include "utils.h"

void 
debugger_init(Debugger* debugger)
{
    debugger->sim_speed = 1; 
    debugger->state = DBG_START;

    debugger->program_file_path = NULL; 
    debugger->sim = NULL; 

    sem_init(&debugger->pause_semaphore, 0, 0);
    debugger->last_stop_ant = 0;
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
    time_t last_sent_update; 
    time(&last_sent_update);
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
                simulation_run_step(dbg->sim);
                if (simulation_stopped_on_breakpoint(dbg->sim))
                {
                    dbg->state = DBG_PAUSE;
                    dbg->last_stop_ant = simulation_get_next_running_ant(dbg->sim);
                    debugger_send_pause_notif("breakpoint", dbg->last_stop_ant);
                    debugger_send_update(dbg);
                    sem_wait(&dbg->pause_semaphore);
                }
                if (simulation_get_step_number(dbg->sim) >= 2000)
                {
                    dbg->state = DBG_PAUSE;
                }
                break;
            }
            case DBG_STEP:
            {
                bool sim_others = false;
                while(dbg->last_stop_ant != simulation_get_next_running_ant(dbg->sim) && 
                dbg->state == DBG_STEP)
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
                    //Run a single instruction, ignoring breakpoints
                    simulation_run_single_instruction(dbg->sim);
                    if (simulation_stopped_on_breakpoint(dbg->sim))
                    {
                        //We hit a breakpoint, so we actually have 
                        //to run the instruction again to actually execute it
                        simulation_run_single_instruction(dbg->sim);
                    }
                }
                debugger_send_pause_notif("step", dbg->last_stop_ant);
                debugger_send_update(dbg);
                sem_wait(&dbg->pause_semaphore);
                break;
            }
            case DBG_PAUSE:
            {
                debugger_send_pause_notif("pause", dbg->last_stop_ant);
                debugger_send_update(dbg);
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
        }
    }

    simulation_delete(dbg->sim);
    dbg->sim = NULL;

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
    print_debug("antide dbg CLOSED");
    cleanup_logging();
}