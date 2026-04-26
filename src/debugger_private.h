#pragma once
#include "utils.h"
#include "simulation.h"
#include <cJSON/cJSON.h>

typedef struct {
    int sim_speed;
    bool stop_sim;
    bool pause_sim;
    bool exit_debugger;

    char* program_file_path;
    cJSON* pending_notif;
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
debugger_send_pause_notif(Debugger* dbg);
