#include "scheduler.cpp"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "des.h"
#include "scheduler.h"

using namespace std;

int quantum = 10000;
int maxprios = 4;
int CURRENT_TIME = 0;

bool vTrack = false;
string svalue;
string inputfile;
string rfile;
string schedulerName;

DES* des;
vector<Process*> total_processes;
int num_processes = 0;
vector<int> randvals;
SCHEDULER* THE_SCHEDULER;

int ofs = 0;
int randCount;

int time_cpubusy = 0;
int time_iobusy = 0;
int IO_num = 0;
int time_io_start = 0;
int finishtime = 0;

void read_rfile() {
    ifstream read_rfile;
    read_rfile.open(rfile);

    if (!read_rfile.is_open()) {
        cout << "Warning: cannot open " << rfile << ". Exit!" << endl;
        exit(1);
    }

    string number;
    getline(read_rfile, number);
    randCount = stoi(number);
    for (int i = 0; i < randCount; i++) {
        getline(read_rfile, number);
        randvals.push_back(stoi(number));
    }

    read_rfile.close();
}

int myrandom(int burst) {
    ofs %= randCount - 1;
    int ans = 1 + (randvals[ofs] % burst);
    ++ofs;
    return ans;
}

void create_process(int PID_, int AT_, int TC_, int CB_, int IO_, int PRIO_) {
    Process* currentProcess = new Process(PID_, AT_, TC_, CB_, IO_, PRIO_);
    total_processes.push_back(currentProcess);

    EVENT* processEvent = new EVENT(currentProcess, TRANS_TO_READY);
    processEvent->evtTimeStamp = AT_;
    processEvent->old_state = STATE_CREATED;
    des->put_event(processEvent);
}

void read_input() {
    ifstream inputFile;
    inputFile.open(inputfile);

    if (!inputFile.is_open()) {
        cout << "Warning: cannot open " << inputfile << ". Exit!" << endl;
        exit(1);
    }

    int processIdCounter = 0;
    int AT_, TC_, CB_, IO_;
    char buffer[100];

    while (inputFile.getline(buffer, sizeof(buffer))) {
        sscanf(buffer, "%d %d %d %d", &AT_, &TC_, &CB_, &IO_);
        int PRIO_ = myrandom(maxprios);

        create_process(processIdCounter++, AT_, TC_, CB_, IO_, PRIO_);
    }

    num_processes = total_processes.size();
}

int get_cpu_burst(Process* proc) {
    if (proc->cb > 0) {
        return proc->cb;
    }

    int tmp_burst = myrandom(proc->CB);

    if (tmp_burst >= proc->rem) {
        return proc->rem;
    }
    else return tmp_burst;
}

void Summary() {
    cout << schedulerName << endl;
    int sumTT = 0;
    int sumCW = 0;
    for (Process* proc : total_processes) {
        printf("%04d: %4d %4d %4d %4d %1d | ", proc->PID, proc->AT, proc->TC, proc->CB, proc->IO, proc->static_prio);
        printf("%5d %5d %5d %5d\n", proc->FT, proc->TT, proc->IT, proc->CW);
        sumTT += proc->TT;
        sumCW += proc->CW;
    }

    double avgTT = sumTT / (double)num_processes;
    double avgCW = sumCW / (double)num_processes;

    double cpu_util = 100.0 * (time_cpubusy / (double)finishtime);
    double io_util = 100.0 * (time_iobusy / (double)finishtime);
    double throughput = 100.0 * (num_processes / (double)finishtime);

    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
        finishtime,
        cpu_util,
        io_util,
        avgTT,
        avgCW,
        throughput
    );
}


void simulation() {
    EVENT* evt;
    bool CALL_SCHEDULER = false;
    Process* CURRENT_RUNNING_PROCESS = nullptr;

    while ((evt = des->get_event()) != NULL) {
        Process* p = evt->evtProcess;
        CURRENT_TIME = evt->evtTimeStamp;
        int transition = evt->new_state;
        int timeInPrevState = CURRENT_TIME - p->t_state_start;

        switch (transition) {
        case TRANS_TO_READY:
        {

            CALL_SCHEDULER = true;
            p->t_state_start = CURRENT_TIME;

            if (vTrack) {
                if (evt->old_state == TRANS_TO_RUNNING) {
                    evt->baseTrack(CURRENT_TIME, timeInPrevState, CPU);
                }
                else {
                    evt->baseTrack(CURRENT_TIME, timeInPrevState, NA);
                }
            }

            if (evt->old_state == TRANS_TO_BLOCK) {
                p->IT += timeInPrevState;

                --IO_num;
                if (IO_num == 0)
                    time_iobusy += CURRENT_TIME - time_io_start;
            }
            else if (evt->old_state == TRANS_TO_RUNNING && (svalue.at(0) == 'P' || svalue.at(0) == 'E')) { // update priority of PRIO/PREPRIO
                (p->dynamic_prio)--;
            }
          
            THE_SCHEDULER->add_process(p);

            if (svalue.at(0) == 'E') {
                if (evt->old_state == TRANS_TO_BLOCK || evt->old_state == STATE_CREATED) {
                    bool preemptNeeded = false;
                    int flag = 0;

                    if (CURRENT_RUNNING_PROCESS != nullptr && p->dynamic_prio > CURRENT_RUNNING_PROCESS->dynamic_prio) {
                        flag = 1;
                        if (!des->checkPreemptTime(CURRENT_RUNNING_PROCESS, CURRENT_TIME)) {
                            preemptNeeded = true;
                        }
                    }

                    if (vTrack) {
                        if (CURRENT_RUNNING_PROCESS != nullptr) {

                            cout << "---> PRIO preemption " << CURRENT_RUNNING_PROCESS->PID << " by " << p->PID << " ? " << flag;
                            cout << " TS=" << CURRENT_RUNNING_PROCESS->t_state_end << " now=" << CURRENT_TIME;

                            cout << ") --> " << (preemptNeeded ? "YES" : "NO") << "\n";
                        }

                        if (preemptNeeded) {
                            if (evt->old_state == TRANS_TO_RUNNING) {
                                evt->baseTrack(CURRENT_TIME, timeInPrevState, CPU);
                            }
                            else {
                                evt->baseTrack(CURRENT_TIME, timeInPrevState, NA);
                            }
                        }
                    }

                    if (preemptNeeded) {
                        int transferTime = CURRENT_RUNNING_PROCESS->t_state_end - CURRENT_TIME;

                        CURRENT_RUNNING_PROCESS->t_state_end = CURRENT_TIME;
                        CURRENT_RUNNING_PROCESS->cb += transferTime;
                        CURRENT_RUNNING_PROCESS->rem += transferTime;

                        time_cpubusy -= transferTime;

                        des->rm_event(CURRENT_RUNNING_PROCESS);

                        EVENT* preemptEvt = new EVENT(CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT);
                        preemptEvt->evtTimeStamp = CURRENT_TIME;
                        des->put_event(preemptEvt);
                        CURRENT_RUNNING_PROCESS = nullptr;
                    }
                }
            }

            if (CURRENT_RUNNING_PROCESS != nullptr && p->PID == CURRENT_RUNNING_PROCESS->PID) {
                CURRENT_RUNNING_PROCESS = nullptr;
            }
        }
        break;
        case TRANS_TO_RUNNING:
        {
            int cpuBurst = get_cpu_burst(p);
            p->cb = cpuBurst;
            int runTime = (cpuBurst >= quantum) ? quantum : cpuBurst;

            p->CW += CURRENT_TIME - p->t_state_start;
            p->t_state_start = CURRENT_TIME;

            if (vTrack) {
                evt->baseTrack(CURRENT_TIME, timeInPrevState, CPU);
            }

            p->t_state_end = CURRENT_TIME + runTime;
            p->cb -= runTime;
            p->rem -= runTime;
            time_cpubusy += runTime;

            EVENT* new_event = new EVENT(CURRENT_TIME + runTime, p);
            new_event->old_state = TRANS_TO_RUNNING;

            if (p->rem > 0) {
                new_event->new_state = (p->cb != 0) ? TRANS_TO_READY : TRANS_TO_BLOCK;
            }
            else {
                new_event->new_state = TRANS_TO_TERMINATE;
            }

            des->put_event(new_event);
        }
        break;	
        case TRANS_TO_BLOCK:
        {
            CALL_SCHEDULER = true;
            CURRENT_RUNNING_PROCESS = nullptr;

            p->t_state_start = CURRENT_TIME;
            int ioBurst = myrandom(p->IO);
            p->io = ioBurst;

            if (vTrack) {
                evt->baseTrack(CURRENT_TIME, timeInPrevState, IO);
            }

            EVENT* new_event = new EVENT(p, TRANS_TO_READY);
            new_event->evtTimeStamp = CURRENT_TIME + ioBurst;
            new_event->old_state = TRANS_TO_BLOCK;
            des->put_event(new_event);

            p->dynamic_prio = p->static_prio - 1;

            if (IO_num == 0) {
                time_io_start = CURRENT_TIME;
            }
            IO_num++;
        }
        break;
        case TRANS_TO_PREEMPT:
        {
            CURRENT_RUNNING_PROCESS = nullptr;
            CALL_SCHEDULER = true;

            p->t_state_start = CURRENT_TIME;
            p->dynamic_prio--;
            THE_SCHEDULER->add_process(p);
        }
        break;
        case TRANS_TO_TERMINATE:
        {
            if (vTrack) {
                evt->terminateTrack(CURRENT_TIME, timeInPrevState);
            }
            CURRENT_RUNNING_PROCESS = nullptr;
            CALL_SCHEDULER = true;
            p->termiate(CURRENT_TIME);
        }
        default:
            break;
        }

        if (CALL_SCHEDULER) {
            if (des->get_next_event_time() == CURRENT_TIME) {
                continue; 
            }

            CALL_SCHEDULER = false; 

            if (CURRENT_RUNNING_PROCESS == nullptr) {
                CURRENT_RUNNING_PROCESS = THE_SCHEDULER->get_next_process();
                if (CURRENT_RUNNING_PROCESS == nullptr) {
                    continue;
                }

                EVENT* new_event = new EVENT(CURRENT_RUNNING_PROCESS, TRANS_TO_RUNNING);
                new_event->evtTimeStamp = CURRENT_TIME;
                new_event->old_state = TRANS_TO_READY;
                des->put_event(new_event);
            }

        }
        delete evt;
        evt = nullptr;
    }

    finishtime = CURRENT_TIME;
}


void create_scheduler(char scheduler_type, string& scheduler_arg) {
    switch (scheduler_type) {
        case 'F':
            schedulerName = "FCFS";
            THE_SCHEDULER = new FCFS();
            break;
        case 'L':
            schedulerName = "LCFS";
            THE_SCHEDULER = new LCFS();
            break;
        case 'S':
            schedulerName = "SRTF";
            THE_SCHEDULER = new SRTF();
            break;
        case 'R': {
            schedulerName = "RR ";
            sscanf(scheduler_arg.c_str(), "R%d", &quantum);
            schedulerName += to_string(quantum);
            THE_SCHEDULER = new ROUNDROBIN();
            break;
        }
        case 'P': {
            if (scheduler_arg.find(":") == string::npos) {
                sscanf(scheduler_arg.c_str(), "P%d", &quantum);
            }
            else {
                sscanf(scheduler_arg.c_str(), "P%d:%d", &quantum, &maxprios);
            }
            schedulerName = "PRIO " + to_string(quantum);
            THE_SCHEDULER = new PRIO(quantum, maxprios);
            break;
        }
        case 'E': {
            if (scheduler_arg.find(":") == string::npos) {
                sscanf(scheduler_arg.c_str(), "E%d", &quantum);
            }
            else {
                sscanf(scheduler_arg.c_str(), "E%d:%d", &quantum, &maxprios);
            }
            schedulerName = "PREPRIO " + to_string(quantum);
            THE_SCHEDULER = new PRIO(quantum, maxprios);
            break;
        }
        default:
            cout << "Unknown scheduler. Exit!" << endl;
            exit(-1);
    }
}

int main(int argc, char** argv) {
    int c;
    while ((c = getopt(argc, argv, "vs:")) != -1) {
        switch (c) {
        case 'v':
            vTrack = true;
            break;
        case 's':
            svalue = string(optarg);
            create_scheduler(svalue.at(0), svalue);
            break;
        case '?':
            if (optopt == 's')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x", optopt);
            exit(1);
        default:
            abort();
        }
    }

    des = new DES();
    char* inputfile_tmp = argv[argc - 2];
    char* rfile_tmp = argv[argc - 1];
    inputfile = string(inputfile_tmp);
    rfile = string(rfile_tmp);

    read_rfile();
    read_input();
    simulation();
    Summary();
    return 0;
}

/*int main(int argc, char** argv) {
    
    svalue = "R2";
    create_scheduler(svalue.at(0), svalue);

    des = new DES();
    string inputfile_tmp = "C:/Users/27666/Downloads/lab2_assign/lab2_assign/lab2_assign/input2";
    string rfile_tmp = "C:/Users/27666/Downloads/lab2_assign/lab2_assign/lab2_assign/rfile";
    inputfile = string(inputfile_tmp);
    rfile = string(rfile_tmp);

    read_rfile();
    read_input();
    simulation();
    Summary();
    return 0;
}*/