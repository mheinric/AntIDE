#include "json_rpc.h"
#include "utils.h"

static FILE* DEBUG_FILE = NULL;

void 
init_logging(const char *log_file)
{
    DEBUG_FILE = fopen(log_file, "a");
}

void 
cleanup_logging()
{
    fclose(DEBUG_FILE);
    DEBUG_FILE = NULL;
}

void 
print_debug(const char *message)
{
    if(DEBUG_FILE == NULL) return;
    fprintf(DEBUG_FILE, "%s\n", message);
    fflush(DEBUG_FILE);
}

void 
print_debug_packet(const cJSON* packet)
{
    if(DEBUG_FILE == NULL) return;
    char* data = cJSON_Print(packet); 
    fprintf(DEBUG_FILE, "%s\n", data); 
    free(data);
    fflush(DEBUG_FILE);
}

void 
send_packet(cJSON* packet)
{
    print_debug_packet(packet);
    char* content = cJSON_Print(packet); 
    free(packet); 
    const size_t length = strlen(content); 
    fprintf(stdout, "Content-Length: %zd\r\n\r\n", length); 
    fprintf(stdout, "%s", content);
    fflush(stdout);
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
    print_debug("Received packet");
    print_debug_packet(packet);
    return packet;
}