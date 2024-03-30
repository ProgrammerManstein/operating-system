#pragma once
#include <vector>
#include <array>
#include <iostream>

#define max_pte 64


struct pte_t {
    unsigned int PRESENT : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int MODIFIED : 1;
    unsigned int REFERENCED : 1;
    unsigned int PAGEDOUT : 1;
    unsigned int frame_index : 7;
    unsigned int isFILE_MAPPED : 1;
    unsigned int isVMA : 1;

    pte_t()
    {
        PRESENT = 0;
        WRITE_PROTECT = 0;
        MODIFIED = 0;
        MODIFIED = 0;
        REFERENCED = 0;
        PAGEDOUT = 0;
        frame_index = 0;
        isFILE_MAPPED = 0;
        isVMA = 0;
    }
};

struct VMA {
    int start_vpage;
    int end_vpage;
    bool write_protected;
    bool file_mapped;
};


struct Statistics {
    unsigned long unmaps;
    unsigned long maps;
    unsigned long ins;
    unsigned long outs;
    unsigned long fins;
    unsigned long fouts;
    unsigned long zeros;
    unsigned long segv;
    unsigned long segprot;

    Statistics() :
        unmaps(0),
        maps(0),
        ins(0),
        outs(0),
        fins(0),
        fouts(0),
        zeros(0),
        segv(0),
        segprot(0)
    {}
};

class Process {
public:
    int pid;
    std::vector<VMA> vma_table;
    std::array<pte_t*, max_pte> page_table;
    Statistics statistic;
};

struct frame_t {
    int fid;
    int vpage;
    pte_t* pte;
    Process* proc;
    unsigned int age;
    unsigned long last_referenced;
};


struct instruction {
    char instType;
    int instVal;
};


