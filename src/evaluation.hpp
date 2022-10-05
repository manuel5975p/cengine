#include "position.hpp"
#include "types.hpp"
#include <xoshiro.hpp>
#include <boost/algorithm/string.hpp>
#include <array>
struct positional_evaluator{
    Bitboard bb;
    int weight;
    constexpr positional_evaluator(Bitboard b, int w) : bb(b), weight(w){}
    template<typename... T>
    constexpr positional_evaluator(int w, T... ts) : bb(((Bitboard(1) << ts) | ...)), weight(w){}
    int operator()(Bitboard b)const noexcept{
        return weight * popcount(b & bb);
    }
};
constexpr positional_evaluator center (10, 26, 27, 28, 29, 34, 35, 36, 37);
constexpr positional_evaluator knights(20, 26, 27, 28, 29, 33, 34, 35, 36, 37, 38, 41, 42 ,43, 44, 45, 46);
int evaluate(const Position& pos){
    int popcnts[12];
    constexpr int weights[12] = {100,310,330,500,900,0,-100,-310,-330,-500,-900,0};
    for(size_t i = 0;i < 12;i++){
        popcnts[i] = popcount(pos.piece_boards[i]);
    }
    int w_minus_b_mat = 0;
    for(size_t i = 0;i < 12;i++){
        w_minus_b_mat += popcnts[i] * weights[i];
    }
    int wcenter = center(pos.piece_boards[0]);
    int bcenter = center(pos.piece_boards[6]);
    w_minus_b_mat += wcenter - bcenter;
    int wpawn_protectivity = popcount(pawn_attacks_bb<WHITE>(pos.piece_boards[0]) & pos.get(WHITE));
    int bpawn_protectivity = popcount(pawn_attacks_bb<BLACK>(pos.piece_boards[6]) & pos.get(BLACK));
    w_minus_b_mat += (wpawn_protectivity - bpawn_protectivity) * 10;
    
    int wknightspos = knights(pos.get<W_KNIGHT>());
    int bknightspos = knights(flipVertical(pos.get<B_KNIGHT>()));
    w_minus_b_mat += wknightspos - bknightspos;
    
    if(pos.at_move == BLACK)return -w_minus_b_mat;
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