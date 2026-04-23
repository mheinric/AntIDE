#include "lsp.h"
#include "utils.h"
#include "parser.h"
#include "json_rpc.h"
#include <cJSON/cJSON.h>

bool NEED_EXIT = false;

cJSON* 
handle_initialize(const cJSON* /*params*/)
{
    const char* server_info_str = "{ \"name\": \"antlsp\", \"version\" : \"0.1.0\" }";
    const char* capabilities_str = "{ \"textDocumentSync\": 1 }";

    cJSON* resp = cJSON_CreateObject(); 
    cJSON* server_capablities = cJSON_Parse(capabilities_str);
    cJSON_AddItemToObject(resp, "capabilities", server_capablities);
    cJSON* serverInfo = cJSON_Parse(server_info_str);
    cJSON_AddItemToObject(resp, "serverInfo", serverInfo);
    return resp;
}

cJSON*
handle_shutdown()
{
    print_debug("antide SHUTDOWN");
    return cJSON_CreateNull();
}

cJSON* 
handle_request(const char* method, const cJSON* params)
{
    if (strcmp(method, "initialize") == 0)
    {
        return handle_initialize(params);
    }
    if (strcmp(method, "shutdown") == 0)
    {
        return handle_shutdown();
    }
    print_debug("No handler for request:");
    print_debug(method);
    return NULL;
}

void
handle_initialized_notif(const cJSON* /*params*/)
{
    print_debug("antide lsp INITIALIZED");
} 

void handle_exit_notif()
{
    print_debug("antide lsp CLOSING");
    NEED_EXIT = true;
}

void handle_set_trace_notif(const cJSON* /*params*/)
{
    //TODO: something
}

void handle_document_change(const cJSON* params)
{
    //Extract the content of the document from the params
    cJSON* document = cJSON_GetObjectItem(params, "textDocument");

    const char* doc_uri = cJSON_GetStringValue(
        cJSON_GetObjectItem(document, "uri")
        );

    const char* document_content = NULL; 
    if (cJSON_HasObjectItem(document, "text"))
    {
        document_content = cJSON_GetStringValue(
            cJSON_GetObjectItem(document, "text")
        );
    }
    else
    {
        document_content = cJSON_GetStringValue(
            cJSON_GetObjectItem(
                cJSON_GetArrayItem(
                    cJSON_GetObjectItem(params,"contentChanges"), 
                0),
            "text"));
    }

    
    if (document_content == NULL || doc_uri == NULL)
    {
        print_debug("Could not extract document content");
        return; 
    }

    //Parse the content to get a list of errors.
    ParseResult result = parse_program_from_string(document_content);
    
    //Convert the list of errors into json
    cJSON* diagnostics = cJSON_CreateArray(); 
    for (size_t i = 0; i < parse_result_nb_errors(&result); i++)
    {
        ParseError err = parse_result_get_error(&result, i);
        cJSON* diag_object = cJSON_CreateObject(); 
        cJSON_AddNumberToObject(diag_object, "severity", 1);
        cJSON_AddStringToObject(diag_object, "message", err.message);
        cJSON* range_object = cJSON_AddObjectToObject(diag_object, "range");
        cJSON* start = cJSON_AddObjectToObject(range_object, "start");
        cJSON* end = cJSON_AddObjectToObject(range_object, "end");
        cJSON_AddNumberToObject(start, "line", err.line - 1); 
        cJSON_AddNumberToObject(start, "character", 0); 
        cJSON_AddNumberToObject(end, "line", err.line); 
        cJSON_AddNumberToObject(end, "character", 0); 
        cJSON_AddItemToArray(diagnostics, diag_object);
    }

    parse_result_cleanup(&result);
    
    //Send a new notification with the errors
    
    cJSON* notif = cJSON_CreateObject();
    cJSON* notif_params = cJSON_AddObjectToObject(notif, "params");

    cJSON_AddStringToObject(notif, "method", "textDocument/publishDiagnostics");

    cJSON_AddStringToObject(notif_params, "uri", doc_uri);
    cJSON_AddItemToObject(notif_params, "diagnostics", diagnostics);

    send_packet(notif);
}

void
handle_notification(const char* method, const cJSON* params)
{
    if (strcmp(method, "initialized") == 0)
    {
        return handle_initialized_notif(params);
    }
    if (strcmp(method, "exit") == 0)
    {
        return handle_exit_notif();
    }
    if (strcmp(method, "$/setTrace") == 0)
    {
        return handle_set_trace_notif(params);
    }
    if (strcmp(method, "textDocument/didOpen") == 0 || 
        strcmp(method, "textDocument/didSave") == 0 || 
        strcmp(method, "textDocument/didChange") == 0)
    {
        return handle_document_change(params);
    }
    print_debug("No handler for notification:");
    print_debug(method);
}

typedef struct {
    cJSON* id; 
    char* method_name; 
    cJSON* params;
} Packet;

Packet* 
packet_create(cJSON* json_packet)
{
    Packet* packet = calloc(1, sizeof(packet));
    packet->id = cJSON_DetachItemFromObject(json_packet, "id");

    const char* method_name = cJSON_GetStringValue(cJSON_GetObjectItem(json_packet, "method"));
    if (method_name != NULL)
    {
        packet->method_name = strdup(method_name);
    }
    packet->params = cJSON_DetachItemFromObject(json_packet, "params");
    cJSON_free(json_packet);
    return packet;
}

void
packet_free(Packet* packet)
{
    if (packet->id) cJSON_free(packet->id);
    if (packet->method_name) free(packet->method_name);
    if (packet->params) cJSON_free(packet->params);
    free(packet);
}

void
run_lsp(void)
{
    init_logging("/tmp/antlsp.log");
    print_debug("antide lsp STARTED");

    while (!NEED_EXIT) {
        cJSON* packet_json = wait_for_message();
        if (packet_json == NULL)
        {
            print_debug("No packet received, closing");
            break;
        }
        print_debug("Received packet");
        print_debug_packet(packet_json);
        Packet* packet = packet_create(packet_json);

        if (packet->method_name == NULL)
        {
            print_debug("Received packet without a method, skipping");
            //TODO: instead of skip we should send back an error maybe?
            goto loop_iter_end;
        }

        if (packet->id == NULL)
        {
            //We received a notification
            handle_notification(packet->method_name, packet->params);
        }
        else 
        {
            // We received a request that expects an answer.
            cJSON* responseData = handle_request(packet->method_name, packet->params); 
            cJSON* resp = cJSON_CreateObject(); 
            cJSON_AddItemToObject(resp, "id", packet->id);
            packet->id = NULL; 
            if (resp != NULL)
            {
                cJSON_AddItemToObject(resp, "result", responseData);
            }
            else 
            {
                cJSON_AddStringToObject(resp, "error", "Internal error");
            }
            print_debug("Sending response");
            print_debug_packet(resp);
            send_packet(resp);
        }

loop_iter_end: 
        if (packet != NULL) packet_free(packet);
    }

    print_debug("antide lsp CLOSED");
    cleanup_logging();
}