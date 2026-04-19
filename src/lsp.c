#include "lsp.h"
#include "utils.h"
#include <cJSON/cJSON.h>

static FILE* DEBUG_FILE = NULL; 
bool NEED_EXIT = false;

void 
print_debug(const char* message)
{
    fprintf(DEBUG_FILE, "%s\n", message);
    fflush(DEBUG_FILE);
}

void print_debug_packet(const cJSON* packet)
{
    char* data = cJSON_Print(packet); 
    fprintf(DEBUG_FILE, "%s\n", data); 
    free(data);
    fflush(DEBUG_FILE);
}

cJSON*
wait_for_message()
{
    cJSON *packet = NULL;
    char* buffer = NULL;
    size_t payload_length;
    int scanf_result = fscanf(stdin, "Content-Length: %ld\r\n\r\n", &payload_length);
    if (scanf_result == 0 || scanf_result == EOF)
    {
        print_debug("Failed to parse content length");
        goto end;
    } 
    if (payload_length > 10000000)
    {
        print_debug("Message too long");
        goto end;
    }

    buffer = malloc((payload_length + 1) * sizeof(char)); // +1 for the null byte at the end of the string
    if (fgets(buffer, payload_length + 1, stdin) != buffer)
    {
        print_debug("Failed to read the input");
        goto end;
    }
    if (strlen(buffer) != payload_length)
    {
        print_debug("Unexpected packet size");
        fprintf(DEBUG_FILE, "%zd instead of %zd\n", strlen(buffer), payload_length);
        goto end;
    }
    packet = cJSON_Parse(buffer);
    if (packet == NULL)
    {
        print_debug("Failed to parse json data"); 
        goto end;
    }
end:
    free(buffer);
    return packet;
}

void 
send_packet(cJSON* packet)
{
    char* content = cJSON_Print(packet); 
    free(packet); 
    const size_t length = strlen(content); 
    fprintf(stdout, "Content-Length: %zd\r\n\r\n", length); 
    fprintf(stdout, "%s", content);
    fflush(stdout);
}

cJSON* 
handle_initialize(const cJSON* /*params*/)
{
    const char* SERVER_INFO = "{ \"name\": \"antlsp\", \"version\" : \"0.1.0\" }";
    cJSON* resp = cJSON_CreateObject(); 
    cJSON* server_capablities = cJSON_AddObjectToObject(resp, "capabilities");
    cJSON_AddBoolToObject(server_capablities, "documentHighlightProvider", true);
    cJSON* serverInfo = cJSON_Parse(SERVER_INFO);
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
    DEBUG_FILE = fopen("/tmp/antlsp.log", "a");
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
    fclose(DEBUG_FILE);
}