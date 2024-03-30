#pragma once
#include "DateStructure.h"
#include <vector>
using namespace std;

extern vector<frame_t*> frame_table;
extern vector<int> randvals;

class Pager {
public:
	virtual frame_t* select_victim_frame(int inst_no) = 0;
};

class FIFO :public Pager {
private:
	frame_t* hand;
	int index;
public:
	FIFO() {
		index = 0;
	}

	frame_t* select_victim_frame(int inst_no) {
		hand = frame_table[index];
		index = (++index) % (frame_table.size());
		return hand;
	}
};

class CLOCK :public Pager {
private:
	frame_t* hand;
	int index;
public:
	CLOCK() {
		index = 0;
	}

	frame_t* select_victim_frame(int inst_no) {
		hand = frame_table[index];
		
		while (true) {
			pte_t* pte = hand->pte;
			if (pte->PRESENT) {
				if (pte->REFERENCED) {
					pte->REFERENCED = 0;
					index++;
					index %= frame_table.size();
					hand = frame_table[index];
				}
					
				else {
					index++;
					index %= frame_table.size();
					return hand;
				}
			}
		}
	}
};

class NRU :public Pager {
private:
	frame_t* hand;
	int index;

	int last;
public:
	NRU() {
		index = 0;
		last = 0;		
	}



	frame_t* select_victim_frame(int inst_no) {
		
		vector<frame_t*> Class;

		for (int i = 0; i < 4; i++) {
			Class.push_back(nullptr);
		}

		hand = frame_table[index];

		for (int i = 0; i < frame_table.size(); i++) {
			hand = frame_table[(i + index) % frame_table.size()];
			int R = hand->pte->REFERENCED;
			int M = hand->pte->MODIFIED;

			if (R == 0 && M == 0 && Class[0] == nullptr) {
				Class[0] = hand;
			}
			else if (R == 0 && M == 1 && Class[1] == nullptr) {
				Class[1] = hand;
			}
			else if (R == 1 && M == 0 && Class[2] == nullptr) {
				Class[2] = hand;
			}
			else if (R == 1 && M == 1 && Class.at(3) == nullptr) {
				Class[3] = hand;
			}
		}

		
		if (inst_no - last >= 48) {
			last = inst_no;
			for (int i = 0; i < frame_table.size(); i++) {
				frame_table[i]->pte->REFERENCED = false;
			}
		}

		for (int i = 0; i < 4; i++) {
			if (Class[i]) {
				hand = Class[i];
				break;
			}
		}

		index = (hand->fid + 1) % frame_table.size();
		return hand;
	}
};

class AGE :public Pager {
private:
	frame_t* hand;
	int index;

public:
	AGE() {
		index = 0;
	}

	frame_t* select_victim_frame(int inst_no) {
		for (int i = 0; i < frame_table.size(); i++) {
			
			hand = frame_table[(i + index) % frame_table.size()];

			if (hand->pte) {
				hand->age = hand->age >> 1;

				if (hand->pte->REFERENCED == 1) {
					hand->age = (hand->age | 0x80000000);
				}
				hand->pte->REFERENCED = false;
			}			
		}

		unsigned int min = 0xffffffff;
		int min_index;

		for (int i = 0; i < frame_table.size(); i++) {
			frame_t *frame = frame_table[(i + index) % frame_table.size()];

			if (frame->age < min) {
				min = frame->age;
				hand = frame;
				min_index = (i + index) % frame_table.size();
			}
		}
		index = (min_index + 1) % frame_table.size();
		return hand;
	}
};

class WORK_SET :public Pager {
private:
	frame_t* hand;
	int index;
	int TUA;

public:
	WORK_SET() {
		index = 0;
		TUA = 49;
	}

	frame_t* select_victim_frame(int inst_no) {
		
		int max=-1;
		bool found=false;
		int select_index;

		for (int i = 0; i < frame_table.size(); i++) {
			frame_t *frame = frame_table[(i + index) % frame_table.size()];

			if (frame->pte->REFERENCED) {
				frame->pte->REFERENCED = 0;
				frame->last_referenced = inst_no;
			}

			int time = inst_no - frame->last_referenced;
			if (time > TUA && !found) {
				hand = frame;
				found = true;
				select_index = index + i % frame_table.size();
				break;
			}
			else if (time > max && !found) {
				max = time;
				hand = frame;
				select_index = index + i % frame_table.size();
			}

		}
		index = (select_index + 1) % frame_table.size();
		return hand;
	}


};


class RANDOM : public Pager {
private:
	int index = 0;
	vector<int> randoms;

public:
	RANDOM() {
		randoms = randvals;
	}

	int generateRandom(int size) {
		index %= randoms.size();
		++index;
		return (randoms[index - 1] % size);
	}

	frame_t* select_victim_frame(int inst_no) {
		int randNum = generateRandom(frame_table.size());
		return frame_table[randNum];
	}

};