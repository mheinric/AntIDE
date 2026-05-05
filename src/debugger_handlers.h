#pragma once
#include "utils.h"
#include "debugger_private.h"
/// Handler methods for all the supported requests in the DAP protocol.

cJSON* 
debugger_handle_initialize(Debugger* dbg);

cJSON* 
debugger_handle_disconnect(Debugger* dbg);

cJSON*
debugger_handle_set_breakpoints(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_set_simulation_speed(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_launch(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_configuration_done(Debugger* dbg);

cJSON*
debugger_handle_list_threads(Debugger* dbg);

cJSON* 
debugger_handle_terminate(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_pause(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_get_stack_trace(Debugger* dbg, const cJSON* params);

cJSON* 
debugger_handle_get_scope(Debugger* dbg, const cJSON* params);

cJSON* 
debugger_handle_get_variables(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_continue(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_set_simulation_speed(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_step_out(Debugger* dbg);

cJSON* 
debugger_handle_step(Debugger* dbg);

cJSON*
debugger_handle_set_current_step(Debugger* dbg, const cJSON* params);

cJSON*
debugger_handle_set_current_ant(Debugger* dbg, const cJSON* params);

cJSON* 
debugger_handle_request(Debugger* dbg, const char* method, const cJSON* params);

void
debugger_handle_notification(Debugger* dbg, const char* method, const cJSON* params);
