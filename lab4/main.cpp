#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <queue>
#include <cmath>
#include <unistd.h>

#include "scheduler.h"

using namespace std;

vector<request*> instruction_list;

scheduler *sched;
request* active=nullptr;
int TIME = 1;
int head;
int direction=1;
int next_request = 0;

int total_waittime=0;
int total_turnaround = 0;
int tot_movement=0;
int max_waittime=0;
int time_io_was_busy = 0;

int last_head;

void readInputfile(string inputfile) {
	ifstream input;
	input.open(inputfile);
	
	int io_number=0;
	string line;


	while (getline(input, line)) {
		if (line[0] != '#') {
			stringstream stream;
			stream << line;
			request *rq = new request();
			io_number++;
			stream >> rq->issue >> rq->track;
			instruction_list.push_back(rq);
		}
	}

	input.close();
}

void Summary() {
	for (int i = 0; i < instruction_list.size();i++) {
		request* rq = instruction_list[i];
		printf("%5d: %5d %5d %5d\n", i, rq->issue, rq->start, rq->end);
	}

	double avg_turnaround = total_turnaround / double(instruction_list.size());
	double avg_waittime = total_waittime / double(instruction_list.size());
	double io_utilization = time_io_was_busy / double(TIME);

	printf("SUM: %d %d %.4lf %.2lf %.2lf %d\n",
		TIME, tot_movement,io_utilization, avg_turnaround, avg_waittime, max_waittime);
}

void simulation() {
	while (true) {
		
		//if a new I / O arrived at the system at this current time
				//¡ú add request to IO - queue
		if (next_request<instruction_list.size() && instruction_list[next_request]->issue == TIME) {
			sched->add_rq(instruction_list[next_request]);
			next_request++;
		}

		//if an IO is active and completed at this time
			//¡ú Compute relevant info and store in IO request for final summary
		if (active && active->track == head) {
			active->end = TIME;
			int wait = active->start - active->issue;
			total_waittime += wait;
			total_turnaround += active->end - active->issue;
			tot_movement += abs(head - last_head);
			last_head = head;
			active = nullptr;

			if (wait > max_waittime)
				max_waittime = wait;
		}

		//if no IO request active now
			//if requests are pending
				//¡ú Fetch the next request from IO - queue and start the new IO.
			//else if all IO from input file processed
				//¡ú exit simulation
		if (!active) {
			if (!sched->io_queue.empty()) {
				if (TIME == 19214) {
					int check = 1;
				}
				active = sched->get_next_rq();
				active->start = TIME;

				if (active->track == head) {
					continue;
				}
			
			}
			else if (sched->io_queue.empty()&&next_request>= instruction_list.size()) {
				break;
			}
		}

		//if an IO is active
			//¡ú Move the head by one unit in the direction its going(to simulate seek)Increment time by 1}
		if (active) {
			if (head < active->track) {
				direction=1;
			}
			else {
				direction = -1;
			}
			head += direction;
			time_io_was_busy++;
		}
		TIME++;
	}
}

int main(int argc, char** argv) {
	/*	
	string s = "C:/Users/27666/Downloads/lab4_assign/input9";
	readInputfile(s);
	sched = new FLOOK;
	simulation();
	Summary();*/
	string opt;
	int c;
	while ((c = getopt(argc, argv, "vqfs:")) != -1) {
		switch (c)
		{
		case 'v':
			break;
		case 'q':
			break;
		case 'f':
			break;
		case 's':
			opt = string(optarg);
			break;
		case '?':
			if (optopt == 's')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			std::exit(1);
		default:
			abort();
		}
	}

	char* parameter = argv[argc - 1];
	string inputfile = string(parameter);
	readInputfile(inputfile);

	if (opt == "N") sched = new FIFO;
	else if (opt == "S") sched = new SSTF;
	else if (opt == "L") sched = new LOOK;
	else if (opt == "C") sched = new CLOOK;
	else if (opt == "F") sched = new FLOOK;
	else {
		cout << "Wrong strategy. Exit!" << endl;
		std::exit(1);
	}

	simulation();
	Summary();

}
