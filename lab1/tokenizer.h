#pragma once
#include<iostream>
#include<sstream>
#include<string>
#include<fstream>
#include<regex>

using namespace std;
class Tokenizer {

private:
	void _parseError(int linenum, int lineoffset, int errcode) {
		const char* errstr[] = {
			"NUM_EXPECTED",
			"SYM_EXPECTED",	
			"ADDR_EXPECTED",
			"SYM_TOO_LONG",
			"TOO_MANY_DEF_IN_MODULE",
			"TOO_MANY_USE_IN_MODULE",
			"TOO_MANY_INSTR",
		};
		printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, errstr[errcode]);
	}

public:
	ifstream input;
	char* token;
	char line[256];
	char* offset;
	string linestr;
	int line_n;
	int line_offset;
	char* p;
	const char delim[4] = { ' ',',','\t','\n'};
	char* context = NULL;

	Tokenizer(string filename) {
		input.open(filename);

		line_n = 0;
		line_offset = 0;
		p = nullptr;
	}

	~Tokenizer() {
		input.close();
	}

	

	void getToken() {

		if(context!=NULL)
			p = strtok_r(NULL, delim, &context);
		while (!input.eof() && p==NULL) {
			line_offset = linestr.length() + 1;
			if (getline(input, linestr)) {
				line_offset = linestr.length()+1;
				strcpy(line, linestr.c_str());
				p= strtok_r(line, delim, &context);
				if (p != NULL) {
					offset = line;
				}

				line_n++;
			}
			else {

				return;
			}	
			
		}
		if (p != NULL) {
				token = p;
				char target = token[0];
				;
				line_offset = token - offset+1;
			}
	}

	string readSymbol() {
		
		getToken();
		if (token) {
			string symbol = token;
			if (symbol.length() > 16) {
				_parseError(line_n, line_offset, 3);
				exit(0);
			}
			regex re("[a-zA-Z][a-zA-Z0-9]*");
			if (regex_match(symbol.begin(), symbol.end(), re)) {
				return symbol;
			}
			else {
				_parseError(line_n, line_offset, 1);
				exit(0);
			}
		}
		else {
			_parseError(line_n, line_offset, 1);
			exit(0);
		}
	}

	int readInt() {
		getToken();
		if (token) {
			string int_token = token;
			regex re("[0-9]+");
			if (regex_match(int_token.begin(), int_token.end(), re)) {
				return atoi(int_token.c_str());
			}
			else {
				_parseError(line_n, line_offset, 0);			
				exit(0);
			}
		
		}else {
			_parseError(line_n, line_offset, 0);
			exit(0);
		}
	}

	string readMARIE() {

		getToken();
		if (token) {
			string iear = token;
			if (iear == "M" || iear == "I" || iear == "E" || iear == "A" || iear == "R") {
				return iear;
			}
			else {
				_parseError(line_n, line_offset, 2);
				exit(0);
			}
		}
		else {
			_parseError(line_n, line_offset, 2);
			exit(0);
		}
	}
};