#include <iostream>
#include "types.hpp"
#include "bitboard.hpp"
#include "position.hpp"
#include "move.hpp"
#include "io.hpp"
int main(){
	Bitboards::init();
	Position p;
	std::vector<complete_move> moves = p.generate_trivial(WHITE);
	for(auto& a : moves){
		std::cout << a.to_string() << std::endl;
	}
}

