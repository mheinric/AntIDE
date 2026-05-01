#pragma once
#include "utils.h"
#include "simulation.h"
#include <cJSON/cJSON.h>

typedef enum {
    DBG_START,
    DBG_FAST_SIM,
    DBG_RUN,
    DBG_STEP,
    DBG_STEP_OUT,
    DBG_PAUSE, 
    DBG_STOP,
    DBG_EXIT,
} DebuggerState;

typedef struct {
    int sim_speed;
    DebuggerState state;

    char* program_file_path;

    pthread_mutex_t sim_mutex;
    Simulation* sim;
    pthread_t sim_thread;
    sem_t pause_semaphore;
    size_t last_stop_ant;
    size_t target_step_nb;

} Debugger;

void 
debugger_init(Debugger* debugger);

void
debugger_cleanup(Debugger* debugger);

bool
debugger_init_simulation(Debugger* debugger);

void* 
debugger_simulation_runner(void* arg);

void 
debugger_send_pause_notif(const char* reason, size_t ant_id);
