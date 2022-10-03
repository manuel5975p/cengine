#include <iostream>
#include <chrono>
#include "types.hpp"
#include "bitboard.hpp"
#include "position.hpp"
#include "move.hpp"
#include "io.hpp"
#include "stackvector.hpp"
#include "search.hpp"
#include <fstream>
uint64_t nanoTime(){
	using namespace std;
	using namespace std::chrono;
	return duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
}
int misn(){
	Bitboards::init();
	std::ifstream ifstr("../../chess_learn/batsch/merged3.txt");
	std::ofstream ofstr("../../chess_learn/batsch/merged_boards3.txt");
	std::string line;
	while(std::getline(ifstr, line)){
		size_t cpos = line.find(',');
		std::string st1 = line.substr(0, cpos);
		std::string st2 = line.substr(cpos + 1, std::string::npos);
		Position p(st1);
		if(p.at_move == WHITE){
			ofstr << ~p.occupied()   << '|';
			ofstr << p.get(B_ROOK)   << '|';
			ofstr << p.get(B_KNIGHT) << '|';
			ofstr << p.get(B_BISHOP) << '|';
			ofstr << p.get(B_QUEEN)  << '|';
			ofstr << p.get(B_KING)   << '|';
			ofstr << p.get(B_PAWN)   << '|';
			ofstr << p.get(W_ROOK)   << '|';
			ofstr << p.get(W_KNIGHT) << '|';
			ofstr << p.get(W_BISHOP) << '|';
			ofstr << p.get(W_QUEEN)  << '|';
			ofstr << p.get(W_KING)   << '|';
			ofstr << p.get(W_PAWN)   << ',';
			ofstr << st2 << "\n";
		}
	}
	return 0;
}
int main(){
	Bitboards::init();
	Position p("rnbqkbnr/pppp1ppp/8/4p3/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 1");
	//std::cout << int(p.spec_mem.ep) << "\n";
	Bitboard ataks(0);
	for(int i = 0;i < 64;i++){
		ataks |= (uint64_t(p.under_attack_for(BLACK, Square(i))) << i);
	}
	search_state state;
	state.count = 0;
	negamax(p, 4, -1000000, 1000000, state);
	std::cout << state.count << "\n";
	std::cout << state.bestmove.to_string() << "\n";
	//return 0;
	//print(ataks);
	
	//p.get(W_PAWN) |= 1;
	//std::cout << '\n' << p.to_string() << "\n\n";
	//neural_net net("/home/manuel/Documents/chess_learn");
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
	std::string str;
	auto leg = p.generate_legal(p.at_move);
	for(auto& it : leg){
		std::cout << it.to_string() << ", ";
	}
	std::cout << std::endl;
	while(std::getline(std::cin, str)){
		complete_move cmm(p.piece_boards, str);
		auto x = p.generate_legal(p.at_move);
		//std::cout << cmm.to_string() << "\n";
		for(size_t i = 0;i < x.size();i++){
			//std::cout << x[i].to_string() << "\n";
		}
		//std::cout << "\n";
		if(std::find(x.begin(), x.end(), cmm) == x.end()){
			std::cout << "Illegal move!" << std::endl;
			//std::cout << x.end() - x.begin() << "\n";
		}

		else if(!p.apply_move_checked(cmm)){
			std::cout << "Illegal move!2" << std::endl;
		}
		else{
			std::cout << p.to_string() << "\n" << std::endl;
			//std::cout << p.to_one_hot_repr() << "\n";
			//std::cout << net(p.to_one_hot_repr()) << "\n";
		}
		
	}
	
	//std::cout << evalcount << "\n";
	//std::cout << state.bestmove.to_string() << "\n";
	//std::cout << sizeof(hash_move_pair) << "\n";
	//std::cout << (p.fen()) << "\n";
	//p.apply_move(moves[0]);
	//std::cout << p.to_string() << "\n";
	//std::cout << sizeof(Piece) << "\n";
	//std::cout << 20000000.0 / (t2 - t1) << "\n" << optinhib << "\n";
}

