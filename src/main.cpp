#include <iostream>
#include <chrono>
#include "types.hpp"
#include "bitboard.hpp"
#include "position.hpp"
#include "move.hpp"
#include "io.hpp"
#include "stackvector.hpp"
#include "search.hpp"
uint64_t nanoTime(){
	using namespace std;
	using namespace std::chrono;
	return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}
int main(){
	Bitboards::init();
	Position p;
	//uint64_t optinhib = 0;
	/*auto t1 = nanoTime();
	for(uint64_t i = 0;i < 20000000;i++){
		stackvector<complete_move, 80> moves = p.generate_trivial(WHITE);
		optinhib += moves[0].xor_mask;
		p.piece_boards[0] |= 512;
	}
	auto t2 = nanoTime();*/
	//p.get(B_QUEEN) |= (1ull << SQ_H4);
	//std::cout << p.to_string() << "\n";
	//stackvector<complete_move, 256> moves = p.generate_legal(WHITE);
	//for(auto& a : moves){
	//	std::cout << a.to_string() << std::endl;
	//}
	//std::cout << moves.size() << "\n";
	search_state state;
	popcount(5);
	{
		//negamax(p, 8,-1000000,1000000,state);
	}
	std::cout << evalcount << "\n";
	std::cout << state.bestmove.to_string() << "\n";
	std::cout << sizeof(pos_move_pair);
	//p.apply_move(moves[0]);
	//std::cout << p.to_string() << "\n";
	//std::cout << sizeof(Piece) << "\n";
	//std::cout << 20000000.0 / (t2 - t1) << "\n" << optinhib << "\n";
}

