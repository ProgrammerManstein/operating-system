#include <iostream>
#include <vector>
#include <deque>
#include<limits.h>

using namespace std;

extern int last_head;
extern int direction;


struct request {
	int issue;
	int track;
	int start;
	int end;
	request(){}
};

class scheduler {
public:
	deque<request*> io_queue;
	virtual request* get_next_rq()=0;
	virtual void add_rq(request *r)=0;
};

class FIFO : public scheduler {
public:
	FIFO() {}

	request* get_next_rq() {
		request *r = io_queue.front();
		io_queue.pop_front();
		return r;
	}

	void add_rq(request *r) {
		io_queue.push_back(r);
	}

};

class SSTF : public scheduler {
public:
	SSTF() {}

	request* get_next_rq() {
		request* r=nullptr;
		int min = INT_MAX;
		int index;
		for (int i = 0; i < io_queue.size(); i++) {
			int distance = abs(io_queue[i]->track - last_head);
			if (distance < min) {
				index = i;
				min = distance;
				r = io_queue[i];
			}
		}
		io_queue.erase(io_queue.begin()+index);
		return r;
	}

	void add_rq(request* r) {
		io_queue.push_back(r);
	}

};

class LOOK : public scheduler {
public:
	LOOK() {}

	request* get_next_rq() {
		request* r = nullptr;
		int min_positive = INT_MAX;
		int min_negative = INT_MAX;
		int index_positive = -1;
		int index_negative = -1;
		for (int i = 0; i < io_queue.size(); i++) {
			int distance = io_queue[i]->track - last_head;
			if (distance < min_positive && distance >= 0) {
				index_positive = i;
				min_positive = distance;
			}
			if (abs(distance) < min_negative && distance <= 0) {
				index_negative = i;
				min_negative = abs(distance);
			}
		}
		
		if (direction > 0) {
			if (index_positive != -1) {
				r = io_queue[index_positive];
				io_queue.erase(io_queue.begin() + index_positive);
			}
			else {
				r = io_queue[index_negative];
				io_queue.erase(io_queue.begin() + index_negative);
			}
		}

		if (direction < 0) {
			if (index_negative != -1) {
				r = io_queue[index_negative];
				io_queue.erase(io_queue.begin() + index_negative);
			}
			else {
				r = io_queue[index_positive];
				io_queue.erase(io_queue.begin() + index_positive);
			}
		}

		
		return r;
	}

	void add_rq(request* r) {
		io_queue.push_back(r);
	}

};

class CLOOK : public scheduler {
public:
	CLOOK() {}

	request* get_next_rq() {
		request* r = nullptr;
		int min_positive = INT_MAX;
		int min_track = INT_MAX;
		int index_positive = -1;
		int index_min_track = -1;
		for (int i = 0; i < io_queue.size(); i++) {
			int distance = io_queue[i]->track - last_head;
			if (distance < min_positive && distance >= 0) {
				index_positive = i;
				min_positive = distance;
			}
			if (io_queue[i]->track < min_track) {
				index_min_track = i;
				min_track = io_queue[i]->track;
			}
		}

		if (index_positive != -1) {
			r = io_queue[index_positive];
			io_queue.erase(io_queue.begin() + index_positive);
		}
		else {
			r = io_queue[index_min_track];
			io_queue.erase(io_queue.begin() + index_min_track);
		}


		return r;
	}

	void add_rq(request* r) {
		io_queue.push_back(r);
	}

};

class FLOOK : public scheduler {

private:
	request* add_queue = nullptr;

public:
	FLOOK() {}

	request* get_next_rq() {
		request* begin = io_queue[0];
		if (begin == add_queue) {
			add_queue = nullptr;
		}

		request* r = nullptr;
		int min_positive = INT_MAX;
		int min_negative = INT_MAX;
		int index_positive = -1;
		int index_negative = -1;
		for (int i = 0; i < io_queue.size(); i++) {
			if (io_queue[i] == add_queue) {
				break;
			}
			int distance = io_queue[i]->track - last_head;
			if (distance < min_positive && distance >= 0) {
				index_positive = i;
				min_positive = distance;
			}
			if (abs(distance) < min_negative && distance <= 0) {
				index_negative = i;
				min_negative = abs(distance);
			}
		}

		if (direction > 0) {
			if (index_positive != -1) {
				r = io_queue[index_positive];
				io_queue.erase(io_queue.begin() + index_positive);
			}
			else {
				r = io_queue[index_negative];
				io_queue.erase(io_queue.begin() + index_negative);
			}
		}

		if (direction < 0) {
			if (index_negative != -1) {
				r = io_queue[index_negative];
				io_queue.erase(io_queue.begin() + index_negative);
			}
			else {
				r = io_queue[index_positive];
				io_queue.erase(io_queue.begin() + index_positive);
			}
		}


		return r;
	}

	void add_rq(request* r) {
		io_queue.push_back(r);
		if (add_queue == nullptr) {
			add_queue = r;
		}
	}

};