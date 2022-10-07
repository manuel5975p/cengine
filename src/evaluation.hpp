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
constexpr positional_evaluator advanced_center (30, 42, 43, 44, 45, 50, 51, 52, 53);
constexpr positional_evaluator knights(20, 26, 27, 28, 29, 33, 34, 35, 36, 37, 38, 41, 42 ,43, 44, 45, 46);
constexpr positional_evaluator kings(40, 0, 1, 2, 6, 7);
constexpr positional_evaluator rooks(-30, 0, 7);
constexpr positional_evaluator knights_border(rank_bb(RANK_1) | rank_bb(RANK_8) | file_bb(FILE_A) | file_bb(FILE_H), -30);
constexpr int passed_scores[8] = {0, 30, 40, 50, 70, 100, 200, 0};
int evaluate(const Position& pos){
    int popcnts[12];
    Bitboard occ = pos.occupied();
    constexpr int weights[12] = {100,310,330,500,900,0,-100,-310,-330,-500,-900,0};
    for(size_t i = 0;i < 12;i++){
        popcnts[i] = popcount(pos.piece_boards[i]);
    }
    int w_minus_b_mat = 0;
    for(size_t i = 0;i < 12;i++){
        w_minus_b_mat += popcnts[i] * weights[i];
    }
    int wcenter = center(pos.get<W_PAWN>()) + advanced_center(pos.get<W_PAWN>());
    int bcenter = center(pos.get<B_PAWN>()) + advanced_center(flipVertical(pos.get<B_PAWN>()));

    w_minus_b_mat += wcenter - bcenter;
    int wpawn_protectivity = popcount(pawn_attacks_bb<WHITE>(pos.piece_boards[0]) & pos.get(WHITE));
    int bpawn_protectivity = popcount(pawn_attacks_bb<BLACK>(pos.piece_boards[6]) & pos.get(BLACK));
    w_minus_b_mat += (wpawn_protectivity - bpawn_protectivity) * 10;
    
    int wknightspos = knights(pos.get<W_KNIGHT>()) + knights_border(pos.get<W_KNIGHT>());
    int bknightspos = knights(flipVertical(pos.get<B_KNIGHT>())) + knights_border(pos.get<B_KNIGHT>());
    w_minus_b_mat += wknightspos - bknightspos;

    //int wkings = kings(pos.get<W_KING>());
    //int bkings = kings(flipVertical(pos.get<B_KING>()));
    //int wrooks = rooks(pos.get<W_ROOK>());
    //int brooks = rooks(flipVertical(pos.get<B_ROOK>()));
    //w_minus_b_mat += wkings - bkings;
    //w_minus_b_mat += wrooks - brooks;
    w_minus_b_mat += (popcount(pos.get<W_BISHOP>()) > 1 ? 40 : 0);
    w_minus_b_mat -= (popcount(pos.get<B_BISHOP>()) > 1 ? 40 : 0);

    biterator biter(0);
    int wmobbonus = 0;
    int bmobbonus = 0;
    biter = biterator(pos.get<W_ROOK>());
    while(*biter){
        wmobbonus += popcount(attacks_bb<ROOK>(lsb(*biter), occ));
        ++biter;
    }
    biter = biterator(pos.get<W_BISHOP>());
    while(*biter){
        wmobbonus += popcount(attacks_bb<BISHOP>(lsb(*biter), occ));
        ++biter;
    }
    biter = biterator(pos.get<B_ROOK>());
    while(*biter){
        bmobbonus += popcount(attacks_bb<ROOK>(lsb(*biter), occ));
        ++biter;
    }
    biter = biterator(pos.get<B_BISHOP>());
    while(*biter){
        bmobbonus += popcount(attacks_bb<BISHOP>(lsb(*biter), occ));
        ++biter;
    }
    w_minus_b_mat += (wmobbonus - bmobbonus) * 5;
    int wpassed = 0, bpassed = 0;
    biter = pos.get<W_PAWN>();
    while(*biter){
        Bitboard passed_field = passed_pawn_span(WHITE, lsb(*biter));
        passed_field &= pos.get<B_PAWN>();
        if(passed_field == 0){
            wpassed += passed_scores[lsb(*biter) / 8];
        }
        ++biter;
    }
    biter = pos.get<B_PAWN>();
    while(*biter){
        Bitboard passed_field = passed_pawn_span(BLACK, lsb(*biter));
        passed_field &= pos.get<W_PAWN>();
        if(passed_field == 0){
            bpassed += passed_scores[8 - lsb(*biter) / 8];
        }
        ++biter;
    }
    w_minus_b_mat += (wpassed - bpassed);
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