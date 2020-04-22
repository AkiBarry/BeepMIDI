#pragma once

void error_and_exit(const std::string message);

bool comp_char_array(const char* a, const char* b, size_t len);

uint32_t char_arr_to_int(const char* a, size_t len);

int get_var_len_int(std::ifstream& input_file, int& cur_len);