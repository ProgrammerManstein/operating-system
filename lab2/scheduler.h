#pragma once
#include <iostream>
#include <string>
#include <stack>
#include <queue>
#include <list>

using namespace std;

class Process {
public:
    int PID;
    int AT;
    int TC;
    int CB;
    int IO;

    int IT = 0;
    int FT;
    int TT;
    int CW = 0;

    int static_prio;
    int dynamic_prio;
    int t_state_start;
    int t_state_end;

    int cb = 0;
    int io = 0;
    int rem;

    Process(int pid,int at, int tc, int cb, int io, int prio ) {
        AT = t_state_start = at;
        TC = rem = tc;
        CB = cb;
        IO = io;
        PID = pid;
        static_prio = prio;
        dynamic_prio = static_prio - 1;
    }

    void termiate(int currentTime) {
        TT = currentTime - AT;
        FT = currentTime;
    }
};

class SCHEDULER {
public:
    virtual void add_process(Process* p) = 0;
    virtual Process* get_next_process() = 0;
    virtual bool test_preempt(Process* p, int curtime) = 0; // false but for ¡®E¡¯
    virtual void printSched() = 0; // Debug
};