module;
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>


export module nestestloader;
import NESCPU_state;
export int hexStrtoInt(std::string str) {
	std::stringstream ss;
	ss << std::hex << str;
	int retval;
	ss >> retval;
	return retval;
}

export class NEStestloader {
	std::vector<NESCPU_state> statesequence;

	NESCPU_state convert_to_state(std::string in_line) {
		NESCPU_state retval;
		std::stringstream ss;
		std::string PC = in_line.substr(0, 4);
		std::string A = in_line.substr(50, 2);
		std::string X = in_line.substr(55, 2);
		std::string Y = in_line.substr(60, 2);
		std::string P = in_line.substr(65, 2);
		std::string SP = in_line.substr(71, 2);
		std::string cycle = in_line.substr(78,3);
		retval.PC = hexStrtoInt(PC);
		retval.A = hexStrtoInt(A);
		retval.X = hexStrtoInt(X);
		retval.Y = hexStrtoInt(Y);
		retval.P = hexStrtoInt(P);
		retval.S = hexStrtoInt(SP);
		retval.test_cycle = atoi(cycle.c_str());
		
		return retval;
	}
public:


	void Init() {
		statesequence = std::vector<NESCPU_state>();
		std::ifstream nestestopener;
		nestestopener.open(R"(C:\Users\Josh\source\NESEmulator\notes\nestest.log)", std::ios_base::binary);
		std::ostringstream sstream;
		sstream << nestestopener.rdbuf();
		std::string whole_file = sstream.str();
		std::vector<char> building_line;
		int current_pos = 0;
		
		int lines = 0;
		while (whole_file[current_pos] != 0) {
			if (whole_file[current_pos] != '\n') {
				building_line.push_back(whole_file[current_pos]);
			}
			else {
				statesequence.push_back(convert_to_state(std::string(building_line.begin(), building_line.end())));
				building_line.clear();
				lines++;
			}
			current_pos++;
		}
	}

	NESCPU_state GetStateSequence(int index) {
		return NESCPU_state(statesequence[index]);
	}
};