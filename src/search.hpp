#include "position.hpp"
#include "evaluation.hpp"
#include <omp.h>
#include <vector>
#include <cstdint>
#include <unordered_set>
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
    hash_map map;
    std::unordered_set<hash_int> visited;
    complete_move bestmove;
    uint64_t count;
    search_state() : map(1 << 20), bestmove(Square(0),Square(0),W_KING, NO_PIECE), count(0){

    }
};

int negamax(Position& pos, int depth, int alpha, int beta, search_state& state){

    if (depth == 0){
        state.count++;
        return evaluate(pos);
    }
    int maxWert = alpha;
    stackvector<complete_move, 256> Zugliste = pos.generate_legal(pos.at_move);
    for (auto& move : Zugliste) {
        Position pclone(pos);
        //std::string tinfo = std::string("Thread ") + std::to_string(omp_get_thread_num()) + "\n";
        //std::cout << tinfo;
        /*if(move.captured_piece == W_KING){
            std::cerr << pos.to_string() << "\n";
            std::cerr << pos.check(WHITE) << std::endl;
            std::cerr << pieceChar(move.moving_piece) << std::endl;
            std::terminate();
        }*/

        pclone.apply_move_checked(move);
        int wert = -negamax(pclone, depth - 1, -beta, -maxWert, state);
        //pos.revert_move_checked(move);
        if (wert > maxWert) {
            maxWert = wert;
            if (depth == 4){
                state.bestmove = move;
            }
            if (maxWert >= beta)
                break;
        }
    }
    return maxWert;
}