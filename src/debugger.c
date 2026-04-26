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
debugger_simulation_runner(void* arg) 
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