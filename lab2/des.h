#pragma once
#include "scheduler.h"
#include <iostream>
#include <list>
#include <string>

using namespace std;

typedef enum { STATE_CREATED, TRANS_TO_READY, TRANS_TO_RUNNING, TRANS_TO_BLOCK, TRANS_TO_PREEMPT, TRANS_TO_TERMINATE } STATES;
typedef enum { CPU, IO, NA } INFOTYPE;

ostream& operator << (ostream& s, STATES tt) { // In order to print enum
	const string stateType[] = { "CREATED", "READY", "RUNNG", "BLOCK", "PREEMPT", "TERMINATE" }; // RUNNG? Keep the same with refoutv
	return s << stateType[tt];
}

class EVENT {
public:

    EVENT() {}

    EVENT(Process* p) {
        evtProcess = p;
    }

    EVENT(Process* p, STATES s) {
        new_state = s;
        evtProcess = p;
    }

    EVENT(int e, Process* p) {
        evtTimeStamp = e;
        evtProcess = p;
    }

    STATES new_state;
    STATES old_state;
    Process* evtProcess;
    int evtTimeStamp;

    void baseTrack(int current, int timeInPrevState, INFOTYPE type) {
        cout << current << " " << evtProcess->PID << " " << timeInPrevState << ": ";
        cout << old_state << " -> " << new_state;

        int info = type;
        switch (info) {
        case CPU:
        {
            cout << " cb=" << evtProcess->cb << " rem=" << evtProcess->rem << " prio=" << evtProcess->dynamic_prio << endl;
        }
        break;
        case IO:
        {
            cout << "  ib=" << evtProcess->io << " rem=" << evtProcess->rem << endl;
        }
        break;
        default:
        {
            cout << endl;
        }
        break;
        }
    }

    void terminateTrack(int c, int t) {
        cout << c << " " << evtProcess->PID << " " << t << ": Done" << endl;
    }

};

class DES {
public:
	list<EVENT*> event_queue;

    void put_event(EVENT* evt) {
        if (event_queue.empty()) {
            event_queue.insert(event_queue.begin(), evt);
        }
        else {
            auto it = event_queue.begin();
            while (it != event_queue.end() ) {
                if ((*it)->evtTimeStamp > evt->evtTimeStamp) {
                    break;
                }
                ++it;
            }
            event_queue.insert(it, evt);
        }
    }

    void rm_event(Process* proc) {
        auto it = event_queue.begin();
        while (it != event_queue.end()) {
            if ((*it)->evtProcess->PID == proc->PID) {
                it = event_queue.erase(it);
            }
            else it++;
        }
    }


    EVENT* get_event() {
        if (event_queue.empty()) {
            return nullptr;
        }
        else {
            auto current_event = event_queue.front();
            event_queue.pop_front();
            return current_event;
        }
    }

    int get_next_event_time() {
        if (event_queue.empty()) {
            return -1;
        }
        else {
            return event_queue.front()->evtTimeStamp;
        }
    }

    bool checkPreemptTime(Process* p, int curtime) {
        if (event_queue.empty()) return false;
        else {
            auto it = event_queue.begin();
            for (; it != event_queue.end(); it++) {
                if ((*it)->evtTimeStamp == curtime && p->PID == (*it)->evtProcess->PID)
                    return true;
            }
            return false;
        }
    }

    void printEvent() {
        if (event_queue.empty()) cout << "event_queue is empty." << endl;
        else {
            for (auto it = event_queue.begin(); it != event_queue.end(); ++it) {
                cout << "event->process: " << (*it)->evtProcess->PID << endl;
            }
        }
    }

};