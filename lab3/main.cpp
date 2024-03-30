#include "Pager.h"
#include "DateStructure.h"
#include "handler.cpp"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <queue>
#include <deque>
#include <unistd.h>

using namespace std;

string inputfile;
string r_file;

deque<frame_t*> free_list;
vector<frame_t*> frame_table;

queue<instruction*> ins_list;
vector<Process*> proc_list;

vector<int> randvals;

int MAX_FRAMES = 128;

bool oooohOption = false;
bool pagetableOption = false;
bool frameTableOption = false;
bool processStatistics = false;

int proc_n = -1;
int ctx_switches = 0;
int process_exits = 0;
int cost = 0;
int countInst = 0;


char operation;
int vpage;

handler* handle;

void read_rfile() {
	ifstream read_rfile;
	read_rfile.open(r_file);

	if (!read_rfile.is_open()) {
		cout << "Warning: cannot open " << r_file << ". Exit!" << endl;
		exit(1);
	}

	string number;
	getline(read_rfile, number);
	int randCount = stoi(number);
	for (int i = 0; i < randCount; i++) {
		getline(read_rfile, number);
		randvals.push_back(stoi(number));
	}

	read_rfile.close();
}

void read_inputfile(string inputfile) {
	ifstream input;
	input.open(inputfile);

	if (!input.is_open()) {
		cout << "Cannot open input file. Exit!" << endl;
		exit(1);
	}

	int pid = 0;
	int vma_n = 0;
	string line;
	while (getline(input, line) && !input.eof()) {
		if (line[0] != '#') {
			stringstream stream;
			stream << line;
			if (line[0] > '0' && line[0] < '9') {
				if (proc_n < 0) {
					stream >> proc_n;
				}
				else if (vma_n == 0) {
					stream >> vma_n;

					Process* proc = new Process();
					proc->pid = pid;
					pid++;
					for (int i = 0; i < vma_n; i++) {
						getline(input, line);
						VMA vma;
						stream.clear();
						stream << line;
						stream >> vma.start_vpage >> vma.end_vpage >> vma.write_protected >> vma.file_mapped;
						proc->vma_table.push_back(vma);

					}
					proc_list.push_back(proc);
					vma_n = 0;
				}
			}
			else {
				instruction* ins = new instruction;
				stream >> ins->instType >> ins->instVal;
				ins_list.push(ins);
			}


		}
	}
	input.close();
}

instruction* get_next_instruction(char& operation, int& vpage) {
	instruction* ins;
	if (ins_list.empty()) {
		return nullptr;
	}
	else {
		ins = ins_list.front();
		ins_list.pop();
		operation = ins->instType;
		vpage = ins->instVal;
		return ins;
	}
}

void Summary () {
	if (processStatistics) {
		for (int i = 0; i < proc_list.size(); i++) {
			Statistics* stats = &proc_list[i]->statistic;
			printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
				proc_list[i]->pid,
				stats->unmaps, stats->maps, stats->ins, stats->outs,
				stats->fins, stats->fouts, stats->zeros,
				stats->segv, stats->segprot);
			cost += stats->maps * 350 + stats->unmaps * 410 + stats->ins * 3200 + stats->outs * 2750 + stats->fins * 2350 + stats->fouts * 2800 + stats->zeros * 150 + stats->segv * 440 + stats->segprot * 410;
		}
		printf("TOTALCOST %lu %lu %lu %lu %lu\n",
			countInst, ctx_switches, process_exits, cost, sizeof(pte_t));

	}
    
}

void simulation() {
	Process* current_process = nullptr;
	while (get_next_instruction(operation, vpage)) {
		countInst++;
		// handle special case of ¡°c¡± and ¡°e¡± instruction

		if (oooohOption) { printf("%lu: ==> %c %d\n", countInst - 1, operation, vpage); }

		if (operation == 'c') {
			current_process = proc_list[vpage];
			cost += 130;
			handle->contextSwitch(current_process);
			ctx_switches++;
		}
		else if (operation == 'e') {
			process_exits++;
			cost += 1230;

			handle->exit(proc_list[vpage]);
		}


		// now the real instructions for read and write
		else if (operation == 'r' || operation == 'w') {
			cost++;
			pte_t* pte = current_process->page_table[vpage];
			if (!pte->PRESENT) {

				// this in reality generates the page fault exception and now you execute
				// verify this is actually a valid page in a vma if not raise error and next inst

				//-> figure out if/what to do with old frame if it was mapped
				// see general outline in MM-slides under Lab3 header and writeup below
				// see whether and how to bring in the content of the access page.
				if (pte->isVMA) {
					frame_t* new_frame = handle->get_frame();
					handle->UNMAP(new_frame);
					pte->REFERENCED = 0;
					pte->MODIFIED = 0;
					handle->MAP(current_process, new_frame, pte, countInst, vpage);
				}
				else {
					if (oooohOption) { std::cout << " SEGV" << std::endl; }
					current_process->statistic.segv++;
				}
				
			}
			handle->update_pte(current_process, operation, pte);
		}

	}
	handle->printPageTable(pagetableOption, proc_list);
	handle->printFrameTable(frameTableOption);
	Summary();
}


int main(int argc, char** argv) {
	int c;
	string method;
	string options="";

	while ((c = getopt(argc, argv, "f::a::o::")) != -1) {
		switch (c)
		{
		case 'f':
			MAX_FRAMES = stoi(optarg);
			break;
		case 'a':
			method = optarg;
			break;
		case 'o':
			options = optarg;
			break;
		case '?':
			if (optopt == 'f' || optopt == 'a' || optopt == 'o')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr,
					"Unknown option character `\\x%x'.\n",
					optopt);
			std::exit(1);
		default:
			abort();
		}
	}

	if (!options.empty()) {
		if (options.find('O') != string::npos) {
			oooohOption = true;
		}
		if (options.find('P') != string::npos) {
			pagetableOption = true;
		}
		if (options.find('F') != string::npos) {
			frameTableOption = true;
		}
		if (options.find('S') != string::npos) {
			processStatistics = true;
		}
	}

	inputfile = string(argv[argc - 2]);
	r_file = string(argv[argc - 1]);

	read_inputfile(inputfile);
	read_rfile();

	handle = new handler(method, MAX_FRAMES);

	simulation();

	/*inputfile = "C:/Users/27666/Downloads/lab3_assign/lab3_assign/lab3_assign/in8";
	read_inputfile(inputfile);
	r_file = "C:/Users/27666/Downloads/lab3_assign/lab3_assign/lab3_assign/rfile";
	read_rfile();
	string opt = "r";
	handle = new handler(opt, MAX_FRAMES);
	simulation();*/
}