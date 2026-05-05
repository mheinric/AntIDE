#pragma once
#include "utils.h"
/// Utility functions for programs communicating via json RPC over stdio.

void
init_logging(const char* log_file); 

void 
cleanup_logging();

void 
print_debug(const char* message);

void 
print_debug_packet(const cJSON* packet);

void
json_rpc_init();

void 
json_rpc_cleanup();

void 
send_packet(cJSON* packet, bool debug);

cJSON*
wait_for_message();