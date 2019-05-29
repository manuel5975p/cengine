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
	stackvector<complete_move, 218> moves = p.generate_trivial(WHITE);
	for(auto& a : moves){
		std::cout << a.to_string() << std::endl;
	}
	std::cout << sizeof(moves) << "\n";
}

