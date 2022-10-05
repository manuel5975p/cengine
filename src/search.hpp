#include "position.hpp"
#include "evaluation.hpp"
#include <omp.h>
#include <vector>
#include <tuple>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
struct hash_move_pair{
    hash_int phash;
    complete_move c;
    hash_move_pair() : phash(0), c(SQ_NONE, SQ_NONE, NO_PIECE, NO_PIECE){

    }
};
struct hash_map{
    std::vector<hash_move_pair> data;
    hash_map(size_t size_in_bytes) : data(size_in_bytes / sizeof(hash_move_pair)){
        
    }
    hash_move_pair& operator[](const Position& pos){
        size_t hash = pos.hash();
        hash_move_pair& ref = data[hash % data.size()];
        if(ref.c.from != SQ_NONE){
            
        }
        //OOF case
        return data[0];
    }
};
struct search_state{
    int depth;
    std::unordered_map<hash_int, std::tuple<int, int, short>> map;
    complete_move bestmove;
    uint64_t count;
    search_state() : map(1 << 20), bestmove(Square(0),Square(0), W_KING, NO_PIECE), count(0){

    }
};

int negamax(Position& pos, int depth, int alpha, int beta, search_state& state){
    state.count++;
    
    if(pos.spec_mem.since_capture >= 50){std::cout << "abort eval" << std::endl;return 0;}
    if(depth <= -30){
        return evaluate(pos);
    }
    
    stackvector<complete_move, 256> Zugliste = pos.generate_legal(pos.at_move);
    bool hash_hit_but_shallower = false;
    if(state.map.contains(pos.quickhash())){
        auto it = state.map.find(pos.quickhash());
        if(std::get<0>(it->second) >= depth){
            //std::cout << "hashhit " << depth << std::endl;
            return std::get<1>(it->second);
        }
        else{
            hash_hit_but_shallower = true;
        }
    }
    if(Zugliste.size() == 0){
        if(pos.check(pos.at_move)){
            return -123456;
        }
        else
            return 0;
    }

    if (depth <= 0){
        bool atcheck = pos.check(pos.at_move);
        if(!atcheck)
        for(auto it = Zugliste.begin();it < Zugliste.end();){
            state.count++;
	    	if(it->captured_piece == NO_PIECE){ //TODO: en passant
                Position pclone(pos);
                bool ch = pclone.apply_move_checked(*it);
                if(!ch)std::terminate();
                if(!pclone.check(pclone.at_move)){
	    		    std::swap(*it, *(Zugliste.end() - 1));
	    		    Zugliste.pop_back();
                }
                else{
                    //std::cout << it->to_string() << "\n";
                    //std::cout << pclone.to_string() << "\n\n";
                    it++;
                }
	    	}
	    	else{
	    		it++;
            }
	    }
        if(Zugliste.size() > 0){
            //if(!atcheck){
                int stand_pat = evaluate(pos);
                if(stand_pat >= beta){
                    return stand_pat;
                }
                if(alpha < stand_pat){
                    alpha = stand_pat;
                }
            //}
            int maxWert = alpha;
            for (auto& move : Zugliste) {
                Position pclone(pos);
                bool ch = pclone.apply_move_checked(move);
                if(!ch)std::terminate();
                int wert = -negamax(pclone, depth - 1, -beta, -maxWert, state);
                //pos.revert_move_checked(move);
                if(depth == state.depth){
                    std::cout << move.to_string() << ": " << wert << "\n"; 
                }
                if (wert > maxWert) {
                    maxWert = wert;
                    if (maxWert >= beta){
                        state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(move.from) << 8 | short(move.to));
                        return maxWert;
                    }
                }
            }
            return maxWert;
        }
        else
            return evaluate(pos);
    }
    int maxWert = alpha;
    std::vector<std::pair<complete_move, int>> guesses(Zugliste.size());
    short previous_best_move = -1;
    if(hash_hit_but_shallower)
        previous_best_move = std::get<2>(state.map.find(pos.quickhash())->second);
    for(size_t i = 0;i < Zugliste.size();i++){
        Position pclone(pos);
        bool ch = pclone.apply_move_checked(Zugliste[i]);
        int wert;
        auto nit = state.map.find(pclone.quickhash());
            if((nit) != state.map.end()){
                wert = std::get<1>(nit->second);
            }
        else
            wert = -evaluate(pclone);
        guesses[i] = std::make_pair(Zugliste[i], wert);
        if(hash_hit_but_shallower){
            if(Zugliste[i].from == previous_best_move >> 8 && Zugliste[i].to == previous_best_move & 255){
                guesses[i].second += 1000;
            }
        }
    }
    std::sort(guesses.begin(), guesses.end(), [](const std::pair<complete_move, int>& a1, const std::pair<complete_move, int>& a2){
        return a1.second > a2.second;
    });

    for (auto& [move, wart] : guesses) {
        Position pclone(pos);
        bool ch = pclone.apply_move_checked(move);
        if(!ch)std::terminate();
        int wert = -negamax(pclone, depth - 1, -beta, -maxWert, state);
        //pos.revert_move_checked(move);
        if(depth == state.depth){
            std::cout << move.to_string() << ": " << wert << "\n"; 
        }
        if (wert > maxWert) {
            maxWert = wert;
            if (depth == state.depth){
                state.bestmove = move;
            }
            if (maxWert >= beta){
                state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(move.from) << 8 | short(move.to));
                return maxWert;
            }
        }
    }
    return maxWert;
}