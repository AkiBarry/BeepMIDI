#include <cstdlib>
#include <iostream>
#include <string>
#include <Windows.h>

#include "Util.hpp"

#include <fstream>

void error_and_exit(const std::string message)
{
	std::cout << message << "\n";
	Sleep(10 * 1000);
	exit(0);
}

bool comp_char_array(const char* a, const char* b, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		if (a[i] != b[i])
			return false;
	}

	return true;
}

uint32_t char_arr_to_int(const char * a, size_t len)
{
	uint32_t res = 0;
	
	for (size_t i = 0; i < len; ++i)
	{
		res = (res << 8) | (0xFF & (uint32_t)a[i]);
	}
	
	return res;
}

int get_var_len_int(std::ifstream& input_file, int& cur_len)
{	
	int res = 0;
	int cur_byte;
	do {
		cur_byte = input_file.get();
		cur_len++;

		res = (res << 7) | (cur_byte & 0x7F);
	} while (cur_byte > 0x80);
	return res;
}
