#pragma once
#include "utils.h"
#include "simulation.h"
#include <cJSON/cJSON.h>

typedef enum {
    DBG_START,
    DBG_RUN,
    DBG_STEP,
    DBG_PAUSE, 
    DBG_STOP,
    DBG_EXIT,
} DebuggerState;

typedef struct {
    int sim_speed;
    DebuggerState state;

    char* program_file_path;
    Simulation* sim;
    pthread_t sim_thread;
    sem_t pause_semaphore;
    size_t last_stop_ant;

} Debugger;

void 
debugger_init(Debugger* debugger);

void
debugger_cleanup(Debugger* debugger);

void* 
debugger_simulation_runner(void* arg);

void 
debugger_send_pause_notif(Debugger* dbg, const char* reason);
