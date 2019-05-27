#ifndef IO_HPP_INCLUDED
#define IO_HPP_INCLUDED
#include "types.hpp"
#include <string>
#include <cstddef>
#include <cassert>
#include <iostream>
std::string bb_to_string(Bitboard x){
	std::string ret(72, 0);
	std::size_t pos = 0;
	for(std::size_t i = 64;i >= 8;i -= 8){
		for(std::size_t j = 1;j <= 8;j++){
			ret[pos++] = !!(x & (1ULL << (i - j))) + '0';
		}
		ret[pos++] = '\n';
	}
	return ret;
}
void print(Bitboard x, std::ostream& out = std::cout){
	out << bb_to_string(x);
}
std::string operator|(const std::string& a, const std::string& b){
	assert(a.size() == b.size() && "Combining strings with different lengths.");
	std::string ret(a.size(), '0');
	for(std::size_t i = 0;i < a.size();i++){
		assert((a[i] == '0' || b[i] == '0') && );
		if(a[i] != '0')ret[i] = a[i];
		else if(b[i] != '0'){ret[i] = b[i];}
	}
	return ret;
}

#endif //IO_HPP_INCLUDED
