#include "position.hpp"
#include "evaluation.hpp"
#include <omp.h>
#include <vector>
#include <cstdint>
struct pos_move_pair{
    Position pos;
    complete_move c;
    pos_move_pair() : pos(0), c(SQ_NONE, SQ_NONE, NO_PIECE){

    }
};
struct hash_map{
    std::vector<pos_move_pair> data;
    hash_map(size_t size_in_bytes) : data(size_in_bytes / sizeof(pos_move_pair)){
        
    }
    pos_move_pair& operator[](const Position& pos){
        size_t hash = pos.hash();
        pos_move_pair& ref = data[hash % data.size()];
        if(ref.c.from == SQ_NONE){
            data[hash % data.size()]
        }
    }
};
struct search_state{
    hash_map map;
    complete_move bestmove;
    search_state() : map(1 << 20), bestmove(Square(0),Square(0),W_KING){

    }
};

int negamax(Position& pos, int depth, int alpha, int beta, search_state& state){

    if (depth == 0)
        return evaluate(pos);
    int maxWert = alpha;
    auto Zugliste = pos.generate_legal(pos.at_move);
    for (auto& move : Zugliste) {
        //std::string tinfo = std::string("Thread ") + std::to_string(omp_get_thread_num()) + "\n";
        //std::cout << tinfo;
        pos.apply_move_checked(move);
        int wert = -negamax(pos, depth-1, -beta, -maxWert, state);
        pos.revert_move_checked(move);
        if (wert > maxWert) {
            maxWert = wert;
            
            if (depth == 8){
                
                state.bestmove = move;
            }
            if (maxWert >= beta)
                break;
        }
    }
    return maxWert;
}