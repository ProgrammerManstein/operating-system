#include "scheduler.h"
#include <iostream>
#include <string>
#include <stack>
#include <queue>
#include <list>

using namespace std;

class FCFS: public SCHEDULER
{
public:
    queue<Process*> READY_QUEUE;

    void add_process(Process* p) {
        READY_QUEUE.push(p);
    }

    Process* get_next_process() {
        if (READY_QUEUE.empty()) {
            return nullptr;
        }
        Process* p = READY_QUEUE.front();
        READY_QUEUE.pop();
        return p;
    }

    bool test_preempttest_preempt(Process* p, int curtime) {
        return false;
    }

    void printSched() {
        queue<Process*> print_queue = READY_QUEUE;
        cout << "------------------------" << endl;
        if (print_queue.empty()) cout << "SCHEDULER is empty" << endl;
        while (!print_queue.empty()) {
            Process* tmp = print_queue.front();
            cout << "PID: " << tmp->PID << ": AT, CB, IO: " << tmp->AT << " " << tmp->CB << " " << tmp->IO << endl;
            cout << "cpu_burst, io_busrt: " << tmp->cb << " " << tmp->io << endl;
            cout << "state_ts, future_ts: " << tmp->t_state_start << " " << tmp->t_state_end << endl;
            print_queue.pop();
        }
        cout << "------------------------" << endl;
    }

    bool test_preempt(Process* p, int curtime) {
        return false;
    }

private:

};

class LCFS : public SCHEDULER {
public:
    stack<Process*> READY_QUEUE;

    void add_process(Process* proc) {
        READY_QUEUE.push(proc);
    }
    Process* get_next_process() {
        if (READY_QUEUE.empty()) {
            return nullptr;
        }
        Process* proc = READY_QUEUE.top();
        READY_QUEUE.pop();
        return proc;
    }
    bool test_preempt(Process* p, int curtime) {
        return false;
    }
    void printSched() {};
};

class SRTF : public SCHEDULER {
public:
    list<Process*> READY_QUEUE;

    void add_process(Process* proc) {
        if (READY_QUEUE.empty()) {
            READY_QUEUE.insert(READY_QUEUE.begin(), proc);
        }
        else {
            auto it = READY_QUEUE.begin();
            while ( it != READY_QUEUE.end()) {
                if ((*it)->rem > proc->rem) {                    
                    break; 
                }
                it++;
            }
            READY_QUEUE.insert(it, proc);
        }
    }
    Process* get_next_process() {
        if (READY_QUEUE.empty()) {
            return nullptr;
        }

        Process* proc = READY_QUEUE.front();
        READY_QUEUE.pop_front();
        return proc;
    }
    bool test_preempt(Process* p, int curtime) {
        return false;
    }
    void printSched() {}
};

class ROUNDROBIN : public SCHEDULER {
public:
    queue<Process*> READY_QUEUE;

    void add_process(Process* proc) {
        READY_QUEUE.push(proc);
    }
    Process* get_next_process() {
        if (READY_QUEUE.empty()) {
            return nullptr;
        }
        Process* proc = READY_QUEUE.front();
        READY_QUEUE.pop();
        return proc;
    }
    bool test_preempt(Process* p, int curtime) {
        return false;
    }
    void printSched() {}
};

class PRIO : public SCHEDULER {
public:
    int quantum;
    int maxprios;

    queue<Process*>* activeQ;
    queue<Process*>* expiredQ;

    PRIO(int quantum_, int maxprios_) : SCHEDULER() {
        quantum = quantum_;
        maxprios = maxprios_;
        activeQ = new queue<Process*>[maxprios_];
        expiredQ = new queue<Process*>[maxprios_];
    }

    void add_process(Process* proc) {
        if (proc->dynamic_prio < 0) { 
            proc->dynamic_prio = proc->static_prio - 1;
            expiredQ[proc->dynamic_prio].push(proc);
        }
        else { 
            activeQ[proc->dynamic_prio].push(proc);
        }
    }
    Process* get_next_process() {
        pair<int, int> queueCounts = countQueue();
        if (queueCounts.first == 0) {
            swap(activeQ, expiredQ);
            swap(queueCounts.first, queueCounts.second);
        }
            
        Process* proc = nullptr;
        if (queueCounts.first > 0) {
            for (int i = maxprios - 1; i >= 0; i--) {
                if (!activeQ[i].empty()) {
                    proc = activeQ[i].front();
                    activeQ[i].pop();
                    break;
                }
            }
        }
        return proc;
    }
    bool test_preempt(Process* proc, int curtime) {
        return proc->t_state_start == curtime;
    }
    void printSched() {}

    pair<int, int> countQueue() {
        int countActiveQ = 0;
        int countExpiredQ = 0;
        for (int i = 0; i < maxprios; i++) {
            countActiveQ += activeQ[i].size();
            countExpiredQ += expiredQ[i].size();
        }
        return make_pair(countActiveQ, countExpiredQ);
    }
};