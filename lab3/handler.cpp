#pragma once
#include "DateStructure.h"
#include "Pager.h"
#include <vector>
#include <string>
#include <iostream>
#include <deque>
#include <iterator>

using namespace std;

#define max_pte 64

extern deque<frame_t*> free_list;
extern vector<frame_t*> frame_table;

extern int countInst;
extern int MAX_FRAMES;

extern bool oooohOption;

class handler {
private:

public:

	handler(string opt, int FT_size) {

		frame_t* fte;

		for (int i = 0; i < FT_size; i++) {
			fte = new frame_t();

			fte->fid = i;
			fte->pte = nullptr;
			fte->age = 0;
			fte->last_referenced = 0;

			frame_table.push_back(fte);
			free_list.push_back(fte);
		}

		switch (opt[0])
		{
		case 'f':
			THE_PAGER = new FIFO();
			break;
		case 'c':
			THE_PAGER = new CLOCK();
			break;
		case 'e':
			THE_PAGER = new NRU();
			break;
		case 'a':
			THE_PAGER = new AGE();
			break;
		case 'w':
			THE_PAGER = new WORK_SET();
			break;
		case 'r':
			THE_PAGER = new RANDOM();
			break;
		default:
			break;
		}
	}

	Pager* THE_PAGER;

	frame_t* allocate_frame_from_free_list() {
		if (free_list.size() > 0) {
			frame_t* fte = free_list.front();
			free_list.pop_front();

			return fte;
		}
		else
			return nullptr;
	}

	frame_t* get_frame() {
		frame_t* frame = allocate_frame_from_free_list();
		if (frame == nullptr)
			frame = THE_PAGER->select_victim_frame(countInst);
		return frame;
	}

	void initPageTable(Process* newProcess) {
		for (int i = 0; i < max_pte; i++) {
			pte_t* pte = new pte_t();
			newProcess->page_table[i] = pte;
		}
	}

	void contextSwitch(Process* newProcess) {
		if (newProcess->page_table[0] == nullptr) {
			initPageTable(newProcess);
		}

		for (int i = 0; i < newProcess->vma_table.size(); i++) {
			for (int j = 0; j < max_pte; j++) {
				if (j >= newProcess->vma_table[i].start_vpage && j <= newProcess->vma_table[i].end_vpage) {
					pte_t* pte = newProcess->page_table[j];
					pte->isVMA = 1;
					if (newProcess->vma_table[i].file_mapped) {
						pte->isFILE_MAPPED = 1;
					}
					else {
						pte->isFILE_MAPPED = 0;
					}
					if (newProcess->vma_table[i].write_protected) {
						pte->WRITE_PROTECT = 1;
					}
					else {
						pte->WRITE_PROTECT = 0;
					}

				}
			}
		}
	}



	void update_pte(Process* cur_proc, char instr, pte_t* pte) {
		if (instr == 'r') {
			pte->REFERENCED = 1;
		}
		else {
			pte->REFERENCED = 1;

			if (pte->WRITE_PROTECT) {
				cur_proc->statistic.segprot++;
				if (oooohOption) { std::cout << " SEGPROT" << std::endl; }
			}
			else {
				pte->MODIFIED = 1;
			}

		}
	}

	void MAP(Process* proc, frame_t* frame, pte_t* pte, int last, int vpage) {
		frame->proc = proc;
		frame->age = 0;
		frame->pte = pte;
		frame->last_referenced = last;
		frame->vpage = vpage;
		pte->PRESENT = true;
		pte->frame_index = frame->fid;

		if (pte->isFILE_MAPPED) {
			if (oooohOption) { cout << " FIN" << endl; }
			proc->statistic.fins++;
		}
		else if (pte->PAGEDOUT) {
			if (oooohOption) { cout << " IN" << endl; }
			proc->statistic.ins++;
		}
		else {
			if (oooohOption) { cout << " ZERO" << endl; }
			proc->statistic.zeros++;
		}
		if (oooohOption) { cout << " MAP " << frame->fid << endl; }
		proc->statistic.maps++;
	}


	void UNMAP(frame_t* frame) {
		pte_t* pte = frame->pte;
		Process* proc = frame->proc;
		if (pte) {
			if (oooohOption) { cout << " UNMAP " << frame->proc->pid << ":" << frame->vpage << endl; }
			pte->PRESENT = 0;
			proc->statistic.unmaps++;

			if (pte->MODIFIED) {
				if (pte->isFILE_MAPPED) {
					proc->statistic.fouts++;
					if (oooohOption) { std::cout << " FOUT" << std::endl; }
				}
				else {
					proc->statistic.outs++;
					if (oooohOption) { std::cout << " OUT" << std::endl; }
					pte->PAGEDOUT = 1;
				}
			}
		}
	}

	void exit(Process* proc) {

		if (oooohOption) { cout << "EXIT current process " << proc->pid << endl; }

		for (int i = 0; i < proc->page_table.size(); i++) {
			pte_t* pte = proc->page_table[i];

			if (pte->PRESENT) {
				free_list.push_back(frame_table[pte->frame_index]);
				pte->PRESENT = false;
				frame_table[pte->frame_index]->pte = nullptr;
				if (oooohOption) {
					cout << " UNMAP " << proc->pid << ":" << i << endl;
				}
				proc->statistic.unmaps++;

				if (pte->MODIFIED && pte->isFILE_MAPPED) {
					proc->statistic.fouts++;
					if (oooohOption) { cout << " FOUT" << endl; }
				}
			}
			pte->PAGEDOUT = 0;

		}
	}

	void printPageTable(bool pagetableOption, vector<Process*> processes) {
		if (pagetableOption) {
			for (int i = 0; i < processes.size(); i++) {
				cout << "PT[" << processes[i]->pid << "]:";

				for (int j = 0; j < processes[i]->page_table.size(); j++) {
					pte_t* pte = processes[i]->page_table[j];
					if (pte->PRESENT) {
						cout << " " << j << ":";

						if (pte->REFERENCED) cout << "R";
						else cout << "-";

						if (pte->MODIFIED) cout << "M";
						else cout << "-";

						if (pte->PAGEDOUT) cout << "S";
						else cout << "-";
					}
					else {
						if (pte->PAGEDOUT)
							cout << " #";
						else cout << " *";
					}
				}
				std::cout << std::endl;
			}
		}
	}

	void printFrameTable(bool frameTableOption) {
		if (frameTableOption) {
			cout << "FT:";
			
			for (int i = 0; i < frame_table.size(); i++) { 
				if (frame_table[i]->pte!=nullptr) {
					cout << " " << frame_table[i]->proc->pid << ":" << frame_table[i]->vpage;
				}
				else cout << " *";
			}

			cout << endl;
		}
	}

};