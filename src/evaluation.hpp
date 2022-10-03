#include "position.hpp"
#include "types.hpp"
#include <xoshiro.hpp>
#include <boost/algorithm/string.hpp>
static uint64_t evalcount(0);
int evaluate(const Position& pos){
    evalcount++;
    unsigned popcnts[12];
    constexpr int weights[12] = {100,310,330,500,900,0,-100,-310,-330,-500,-900,0};
    for(size_t i = 0;i < 12;i++){
        popcnts[i] = popcount(pos.piece_boards[i]);
    }
    int w_minus_b_mat = 0;
    for(size_t i = 0;i < 12;i++){
        w_minus_b_mat += popcnts[i] * weights[i];
    }
    /*if(pos.at_move == WHITE){
        Bitboard occ = 0;
        for(auto& bb : pos.piece_boards){
            if(occ & bb){
                print(bb);
                assert("Fucced position" && false);
            }
            occ |= bb;
        }
        std::string _fen = pos.fen();
        //std::cout << _fen << std::endl;
        std::vector<std::string> result;
        boost::split(result, _fen, boost::is_any_of(" "));
        if(result[0].find('k') == std::string::npos){
            //std::cerr << _fen << std::endl;
            std::terminate();
        }
        if(result[0].find('K') == std::string::npos){
            //std::cerr << _fen << std::endl;
            std::terminate();
        }

    }*/
    return w_minus_b_mat;
}