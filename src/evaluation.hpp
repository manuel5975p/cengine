#include "position.hpp"
#include "types.hpp"
#include <xoshiro.hpp>
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
    return w_minus_b_mat;
}