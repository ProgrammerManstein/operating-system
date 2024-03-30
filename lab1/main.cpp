#include<iostream>
#include<sstream>
#include<string>
#include<fstream>
#include<regex>
# include <map>
#include <iomanip>
# include "tokenizer.h"
using namespace std;

map<string, int> symbol_table;
map<int, int> module_table;

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

void _errorMsg(string name, int err_code) {

	switch (err_code) {
	case 0: {
		cout << " Error: Illegal module operand ; treated as module=0"  << endl;
		break;
	}	
	case 1: {
		cout << " Error: Absolute address exceeds machine size; zero used" <<  endl;
		break;
	}
	case 2: {
		cout << " Error: Relative address exceeds module size; relative zero used" <<  endl;
		break;
	}
	case 3: {
		cout << " Error: External operand exceeds length of uselist; treated as relative=0" <<  endl;
		break;
	}
	case 4: {
		 cout << " Error: " << name << " is not defined; zero used" <<  endl;
		break;
	}
	case 5: {
		 cout << " Error: This variable is multiple times defined; first value used" <<  endl;
		break;
	}
	case 6: {
		 cout << " Error: Illegal immediate operand; treated as 999" <<  endl;
		break;
	}
	case 7: {
		 cout << " Error: Illegal opcode; treated as 9999" <<  endl;
		break;
	}
	default: {
		 cout << " Error: Others" <<  endl;
	}
	}
}


void _warnMsg(int errcode, int module, string name, int offset, int size) {
	switch (errcode) {
	case 0:																			// rule 5
		cout << "Warning: Module " << module << ": " << name << " too big " << offset << " (max=" << size << ") assume zero relative\n";
		break;
	case 1:																			// rule 7
		cout << "Warning: Module " << module << ": " << name << " appeared in the uselist but was not actually used\n";
		break;
	case 2:																			// rule 4
		cout << "Warning: Module " << module << ": " << name << " was defined but never used\n";
		break;
	default:
		break;
	}
}

void printSymbolTable(map<string,int> m) {
	 cout << "Symbol Table" <<  endl;

	for (map< string, int>::iterator it = symbol_table.begin(); it != symbol_table.end(); it++) {
		 cout << (*it).first << "=" << (*it).second;
		 if (m.count((*it).first)) {
			 string a;
			 _errorMsg(a, m.at((*it).first));
		 }
		 else {
			 cout << endl;
		 }
	}
	 cout << endl;
}

void pass1(Tokenizer &t) {

	int module_n = 1;
	int module_addr = 0;
	map<string, int> err;

	while (!t.input.eof()) {
		int defcount = t.readInt();
		map<string, int> sym_table;
		
		if (t.input.eof()) {
			break;
		}

		
		module_table.insert({ module_n,module_addr });
		//cout << module_n << " " << module_addr << endl;

		if (defcount > 16) {
			_parseError(t.line_n, t.line_offset, 4);
			exit(0);
		}
		for (int i = 0; i < defcount; i++) {
			string sym = t.readSymbol();
			int r_addr = t.readInt();
			if (sym_table.count(sym) == 0 && symbol_table.count(sym) == 0)
			{
				sym_table.insert({ sym,r_addr });
			}
			else {
				err.insert({ sym, 5 });
			}
		}

		int usecount = t.readInt();
		if (usecount > 16) {
			_parseError(t.line_n, t.line_offset, 5);
			exit(0);
		}
		for (int i = 0; i < usecount; i++) {
			t.readSymbol();
		}

		int codecount = t.readInt();
		if (codecount +codecount > 512) {
			_parseError(t.line_n, t.line_offset, 6);
			exit(0);
		}
		for (int i = 0; i < codecount; i++) {
			t.readMARIE();
			t.readInt();
		}

		for (map<string, int>::iterator iter = sym_table.begin(); iter != sym_table.end();iter++) {
			if ((*iter).second > codecount) {
				_warnMsg(0, module_n, (*iter).first, (*iter).second, codecount - 1);
				(*iter).second = module_addr;
			}
			else {
				(*iter).second += module_addr; 
			}
			symbol_table.insert(*iter);
		}
		module_addr += codecount;
		module_n++;
	}
	printSymbolTable(err);
	t.input.close();
}

void pass2(Tokenizer &t) {
	int module_addr;
	int module_n = 1;
	int instr_n = 0;
	int instrcount = 0;
	map<string, int> symbol_module;
	map<string, int> def_lst_used_record;

	map<std::string, int>::iterator it;
	for (it = symbol_table.begin(); it != symbol_table.end(); it++) {
		def_lst_used_record.insert({ (*it).first, 0 });
	}

	cout<< "Memory Map"<<endl;


	while (!t.input.eof()) {

		string def[16];
		string def_use[16];
		module_addr = module_table.at(module_n);
		

		int defcount = t.readInt();
		

		for (int i = 0; i < defcount; i++) {
			string symbol = t.readSymbol();
			t.readInt();
			symbol_module.insert({ symbol,module_n});
		}
		
		int usecount = t.readInt();
		for (int i = 0; i < usecount; i++) {
			string symbol = t.readSymbol();
			def[i] = symbol;
		}

		int codecount = t.readInt();

		for (int i = 0; i < codecount; i++) {

			string MARIE = t.readMARIE();

			int instr = t.readInt();
			int opcode = instr / 1000;
			int operand = instr % 1000;
			int err_code = -1;
			string p;
			if (opcode >= 10 && err_code == -1){
				instr = 9999;
				err_code=7;
			}
			else {

				if (MARIE == "M") {
					if (module_table.find(operand) == module_table.end()) {

						err_code = 0;
						instr = instr - operand + module_table.at(0);
					}
					else {
						instr = instr - operand + module_table.at(operand);
					}
				}

				if (MARIE == "I") {
					if (operand >= 900) {
						instr = instr - operand;
						operand = 999;
						err_code = 6;
						instr = instr + operand;
					}
				}

				if (MARIE == "E") {
					if (operand >= usecount) {
						err_code = 3;
						instr = instr - operand + module_addr;
					}
					else {
						string sym = def[operand];
						def_use[operand] = sym;
						if (symbol_table.count(sym) == 0) {
							p = sym;
							err_code = 4;

							instr -= operand;
						}
						else {
							def_lst_used_record.at(sym) = 1;
							instr = instr - operand + symbol_table.at(sym);
						}
					}
				}

				if (MARIE == "A") {
					if (operand >= 512) {
						err_code = 1;
						instr -= operand;
					}
				}

				if (MARIE == "R") {
					if (operand > codecount) {
						err_code = 2;
						instr = instr - operand + module_addr;
					}
					else {
						instr += module_addr;
					}
				}

			}

			

			cout << std::setfill('0') << std::setw(3) << instrcount++ << ": "<< std::setfill('0') << std::setw(4) << instr;
			if (err_code != -1)
				_errorMsg(p, err_code);
			else
				cout << endl;
		}
		for (int i = 0; i < 16; i++) {
			if (def[i] != def_use[i]) {
				_warnMsg(1, module_n, def[i], 0, 0);
			}
		}
		module_n++;
		if (module_table.find(module_n) == module_table.end()) {
			break;
		}
		
		
	}
	if (def_lst_used_record.size() > 0) {
			cout << std::endl;
		}
	for (it = def_lst_used_record.begin(); it != def_lst_used_record.end(); it++) {
			if ((*it).second == false) {
				cout << "Warning: Module " << symbol_module.at((*it).first) << ": " << (*it).first << " was defined but never used\n";
			}
		}
}

int main(int argc, char* argv[]) {

	string filename;
	map<string, int> symbol_table;

	if (argc == 2) {
		filename = argv[1];
	}
	else {
		std::cout << "Too many input files.";
		exit(0);
	}
	// pass one
	Tokenizer p1(filename);
	pass1(p1);
	// pass two
	Tokenizer p2(filename);
	pass2(p2);

	return 0;
}