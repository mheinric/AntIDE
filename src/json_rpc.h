#pragma once
#include <cJSON/cJSON.h>

void
init_logging(const char* log_file); 

void 
cleanup_logging();

void 
print_debug(const char* message);

void 
print_debug_packet(const cJSON* packet);

void 
send_packet(cJSON* packet);

cJSON*
wait_for_message();