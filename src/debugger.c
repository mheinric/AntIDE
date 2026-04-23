#include "debugger.h"
#include "json_rpc.h"
#include "utils.h"

static bool EXIT_DEBUGGER = false;

static cJSON* PENDING_NOTIF = NULL;

static 
cJSON* handle_initialize()
{
    //Return the server capabilities
    cJSON* capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(capabilities, "supportsTerminateRequest", true);
    cJSON_AddBoolToObject(capabilities, "supportsRestartRequest", true);
    cJSON_AddBoolToObject(capabilities, "supportsInstructionBreakpoints", true);
    cJSON_AddBoolToObject(capabilities, "supportsConfigurationDoneRequest", true);
    return capabilities; 
}

static 
cJSON* 
handle_disconnect()
{
    EXIT_DEBUGGER = true;
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
handle_launch(const cJSON* /*params*/)
{
    //Prepare the Initialized notification that will be sent after the response has been sent.
    PENDING_NOTIF = cJSON_CreateObject(); 
    cJSON_AddStringToObject(PENDING_NOTIF, "type", "event");
    cJSON_AddStringToObject(PENDING_NOTIF, "event", "initialized");

    return cJSON_CreateObject();
}

static 
cJSON*
handle_configuration_done()
{
    return cJSON_CreateObject();
}

static
cJSON*
handle_list_threads()
{
    cJSON* resp = cJSON_CreateObject();
    cJSON* thread_list = cJSON_AddArrayToObject(resp, "threads");
    cJSON* thread_item = cJSON_CreateObject();
    cJSON_AddItemToArray(thread_list, thread_item); 
    cJSON_AddNumberToObject(thread_item, "id", 0);
    return resp;
}

static 
cJSON* 
handle_terminate(const cJSON* /*params*/)
{
    // Stop the simulation, but don't stop the debugger
    // Note: there is a 'restart' param if we want to restart the simulation from the start that needs to be accounted for
    PENDING_NOTIF = cJSON_CreateObject(); 
    cJSON_AddStringToObject(PENDING_NOTIF, "type", "event"); 
    cJSON_AddStringToObject(PENDING_NOTIF, "event", "terminated");
    return cJSON_CreateNull();
}

static 
cJSON*
handle_pause(const cJSON* /*params*/)
{
    PENDING_NOTIF = cJSON_CreateObject(); 
    cJSON_AddStringToObject(PENDING_NOTIF, "type", "event"); 
    cJSON_AddStringToObject(PENDING_NOTIF, "event", "stopped");
    cJSON* notif_body = cJSON_AddObjectToObject(PENDING_NOTIF, "body");
    cJSON_AddStringToObject(notif_body, "reason", "pause");
    cJSON_AddBoolToObject(notif_body, "allThreadsStopped", true);
    cJSON_AddNumberToObject(notif_body, "threadId", 0);

    return cJSON_CreateNull();
}

static
cJSON*
handle_get_stack_trace(const cJSON* /*params*/)
{
    cJSON* result = cJSON_CreateObject(); 
    cJSON_AddNumberToObject(result, "totalFrames", 1);
    cJSON* stackFrames = cJSON_AddArrayToObject(result, "stackFrames");
    cJSON* frame = cJSON_CreateObject(); 
    cJSON_AddItemToArray(stackFrames, frame);
    cJSON_AddNumberToObject(frame, "id", 0);
    cJSON_AddStringToObject(frame, "name", "main");
    cJSON_AddNumberToObject(frame, "line", 0);
    cJSON_AddNumberToObject(frame, "column", 0);
    return result;
}

static
cJSON*
handle_continue(const cJSON* /*params*/)
{
    //TODO: this is temporary for testing
    PENDING_NOTIF = cJSON_Parse("{\n"
        "\"type\": \"event\",\n"
        "\"event\": \"gridContent\",\n"
        "\"body\": {}\n"
    "}");
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
            send_packet(resp);
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
            send_packet(PENDING_NOTIF); 
            PENDING_NOTIF = NULL;
        }

loop_iter_end:
        if (message) cJSON_free(message);
    }

    print_debug("antide dbg CLOSED");
    cleanup_logging();
}