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
#include <chrono>
#include <sstream>
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
	std::ofstream whatwasinput("/home/manuel/cengine_log.txt", std::ios::app);
	whatwasinput << "\n\n=========================\n\n" << std::flush;
	std::string str;
	Position p;
	std::cout << "HolzerichEngine by Holzerich" << std::endl;
	search_state state;
	while(std::getline(std::cin, str)){
		whatwasinput << str << std::endl;
		std::istringstream isstr(str);
		std::string command;
		isstr >> command;
		if(command == "quit"){
			break;
		}
		if(command == "plei"){
			state.depth = 4;
			state.count = 0;
			negamax(p,  state.depth, -100000, 100000, state);
			state.depth = 6;
			state.count = 0;
			negamax(p,  state.depth, -100000, 100000, state);
			p.apply_move_checked(state.bestmove);
			std::cout << p.to_string() << "\n";
		}
		if(command == "position"){
			std::string StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
			std::string fen;
			std::string token;
			isstr >> token;
			if (token == "startpos"){
    		    fen = StartFEN;
    		    isstr >> token;
    		}
			else if(token == "fen"){
				//std::cout << "Fen inegrotzt" << std::endl;
				while (isstr >> token && token != "moves")
            		fen += token + " ";
			}
			p = Position(fen);
			while(isstr >> token){
				bool attempt = p.apply_move_checked(complete_move(p.piece_boards, token));
				if(!attempt){
					std::terminate();
				}
			}
			//std::cout << p.to_string() << "\n";
			//std::cout << int(p.spec_mem.cr) << "\n";
		}
		else if(command == "go"){
			state.map.clear();
			state.depth = 2;
			state.count = 0;
			std::string bm;
			auto nau = std::chrono::high_resolution_clock::now();
			int sgor = negamax(p,  state.depth, -100000, 100000, state);
			bm = square_to_string(state.bestmove.from) + square_to_string(state.bestmove.to);
			int nps = state.count * 1000000000 / (std::chrono::high_resolution_clock::now() - nau).count();
			std::cout << "info depth " << state.depth << " seldepth " << state.depth + 10 << " multipv 1 score cp " << sgor << " nodes " << state.count << " nps " << nps << " hashfull 63 tbhits 0 time 101 pv " + bm << std::endl;
			state.depth = 4;
			state.count = 0;
			sgor = negamax(p,  state.depth, -100000, 100000, state);
			bm = square_to_string(state.bestmove.from) + square_to_string(state.bestmove.to);
			nps = state.count * 1000000000 / (std::chrono::high_resolution_clock::now() - nau).count();
			std::cout << "info depth " << state.depth << " seldepth " << state.depth + 10 << " multipv 1 score cp " << sgor << " nodes " << state.count << " nps " << nps << " hashfull 63 tbhits 0 time 101 pv " + bm << std::endl;
			if((std::chrono::high_resolution_clock::now() - nau).count() < 2000000000){
				state.depth = 7;
				std::vector<Position> succ = p.generate_all_successors();
				auto leg = p.generate_legal(p.at_move);
				std::vector<search_state> states(succ.size());
				for(size_t i = 0;i < states.size();i++){
					states[i].map = state.map;
				}
				std::vector<int> scores(succ.size());
				state.count = 0;
				#pragma omp parallel for
				for(size_t i = 0;i < succ.size();i++){
					scores[i] = -negamax(succ[i], state.depth, -100000, 100000, states[i]);
				}
				for(size_t i = 0;i < scores.size();i++){
					std::cout << leg[i].to_short_notation() << ": " << scores[i] << std::endl;
				}
				auto maxit = std::max_element(scores.begin(), scores.end());
				complete_move total_bescht = leg[maxit - scores.begin()];
				state.bestmove = total_bescht;
				bm = square_to_string(state.bestmove.from) + square_to_string(state.bestmove.to);
				nps = state.count * 1000000000 / (std::chrono::high_resolution_clock::now() - nau).count();
				std::cout << "info depth " << state.depth << " seldepth " << state.depth + 10 << " multipv 1 score cp " << sgor << " nodes " << state.count << " nps " << nps << " hashfull 63 tbhits 0 time 101 pv " + bm << std::endl;
				for(size_t i = 0;i < succ.size();i++){
					for(auto& [k, v] : states[i].map){
						if(state.map.find(k) == state.map.end()){
							state.map[k] = v;
						}
						else if(std::get<0>(v) > std::get<0>(state.map.find(k)->second)){
							state.map[k] = v;
						}
					}
				}
				state.map[p.quickhash()] = std::make_tuple(state.depth + 1, *maxit, short(total_bescht.from) << 8 | short(total_bescht.to));
				states.clear();
			}
			std::vector<complete_move> pv;
			Position pvpos(p);
			while(true){
				auto it = state.map.find(pvpos.quickhash());
				if(it == state.map.end()){
					break;
				}
				short sm = std::get<2>(it->second);
				pv.push_back(complete_move(pvpos.piece_boards, Square(sm >> 8), Square(sm & 255)));
				pvpos.apply_move_checked(pv.back());
			}
			std::string pvstring;
			for(auto& cm : pv){
				pvstring += cm.to_short_notation();
				pvstring += ' ';
			}
			std::cout << "info depth " << state.depth << " seldepth " << state.depth + 10 << " multipv 1 score cp " << sgor << " nodes " << state.count << " nps " << nps << " hashfull 63 tbhits 0 time 101 pv " + pvstring << std::endl;
			std::cout << "bestmove ";
			std::cout << square_to_string(state.bestmove.from);
			std::cout << square_to_string(state.bestmove.to);
			std::cout << std::endl;
		}
		else if(command == "uci"){
			std::cout << "id name HolzerichEngine 1" << std::endl;
			std::cout << "id author flaschenholz" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if(command == "isready"){
			std::cout << "readyok" << std::endl;
		}
	}
}
int maisn(){
	Bitboards::init();
	Position p("rnbqkbnr/3ppppp/8/2p5/4P3/p1PP1PP1/1B5P/RN1QKBNR b KQkq - 0 9");
	//p.apply_move_checked(complete_move(p.piece_boards, "a1b1"));
	//std::cout << int(p.spec_mem.ep) << "\n";
	//Bitboard ataks(0);
	//for(int i = 0;i < 64;i++){
	//	ataks |= (uint64_t(p.under_attack_for(BLACK, Square(i))) << i);
	//}
	search_state state;
	state.depth = 4;
	state.count = 0;
	negamax(p, state.depth, -1000000, 1000000, state);
	//std::cout << state.count << "\n";
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
	//stackvector<complete_move, 256> moves = p.generate_legal(p.at_move);
	//for(auto& a : moves){
	//	std::cout << a.to_string() << std::endl;
	//}
	//std::cout << moves.size() << "\n";
	std::string str;
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
	return 0;
}

