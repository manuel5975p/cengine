#include <iostream>
#include "types.hpp"
#include "bitboard.hpp"
#include "position.hpp"
#include "move.hpp"
#include "io.hpp"
#include "stackvector.hpp"
int main(){
	Bitboards::init();
	Position p;
	uint64_t optinhib = 0;
	for(uint64_t i = 0;i < 10000000;i++){
		stackvector<complete_move, 218> moves = p.generate_trivial(WHITE);
		optinhib += moves[0].xor_mask;
		p.piece_boards[0] |= 512;
	}
	stackvector<complete_move, 218> moves = p.generate_trivial(WHITE);
	for(auto& a : moves){
		std::cout << a.to_string() << std::endl;
	}
	std::cout << optinhib << "\n";
}

