#include "evaluation.hpp"
struct positional_evaluator{
    Bitboard bb;
    int weight;
    Bitboard upside_down;
    constexpr positional_evaluator(Bitboard b, int w) : bb(b), weight(w), upside_down(flipVertical(bb)){}
    template<typename... T>
    constexpr positional_evaluator(int w, T... ts) : bb(((Bitboard(1) << ts) | ...)), weight(w), upside_down(flipVertical(bb)){}
    int operator()(Bitboard b)const noexcept{
        return weight * popcount(b & bb);
    }
    template<Color c>
    int eval(Bitboard b)const noexcept{
        if constexpr(c == WHITE){
            return weight * popcount(b & bb);
        }
        return weight * popcount(b & upside_down);
    }
};
constexpr positional_evaluator center (10, 26, 27, 28, 29, 34, 35, 36, 37);
constexpr positional_evaluator advanced_center (30, 42, 43, 44, 45, 50, 51, 52, 53);
constexpr positional_evaluator knights(20, 26, 27, 28, 29, 33, 34, 35, 36, 37, 38, 41, 42 ,43, 44, 45, 46);
constexpr positional_evaluator kings(40, 0, 1, 2, 6, 7);
constexpr positional_evaluator rooks(-30, 0, 7);
constexpr positional_evaluator knights_border(rank_bb(RANK_1) | rank_bb(RANK_8) | file_bb(FILE_A) | file_bb(FILE_H), -10);
constexpr int passed_scores[8] = {0, 30, 40, 50, 70, 100, 200, 0};
template<Piece pi>
inline stackvector<Bitboard, 10> attacks_by_piece(const Position& pos, Bitboard occ){
	stackvector<Bitboard, 10> ret;
	Bitboard piece_occ = pos.get<pi>();
	Bitloop(piece_occ){
		Square sq = lsb(piece_occ);
		Bitboard atks = attacks_bb<get_type(pi)>(sq, piece_occ);
		ret.push_back(atks);
	}
	return ret;
}
template<Color we>
int king_safety(const Position& pos, Bitboard occ){
	constexpr Color them = ~we;
	//Bitboard tknights = pos.get<make_piece(them, KNIGHT)>();
	//Bitboard tbishops = pos.get<make_piece(them, BISHOP)>();
	//Bitboard trooks = pos.get<make_piece  (them, ROOK)>();
	//Bitboard tqueens = pos.get<make_piece (them, QUEEN)>();
	Bitboard our_king_zone = KingTwoMoves[lsb(pos.get<make_piece(we, KING)>())];
	Bitboard problematic_attacks = 0;
	unsigned collisions = 0;
	stackvector<Bitboard, 10> patks = attacks_by_piece<make_piece(them, KNIGHT)>(pos, occ);
	size_t index = 0;
	for(Bitboard atk : patks){
		Bitboard problematic_slice = atk & our_king_zone;
		//std::cout << patks.size() << "\n";
		collisions += popcount(problematic_attacks & problematic_slice);
		problematic_attacks |= problematic_slice;
	}
	patks = attacks_by_piece<make_piece(them, BISHOP)>(pos, occ);
	for(Bitboard atk : patks){
		Bitboard problematic_slice = atk & our_king_zone;
		collisions += popcount(problematic_attacks & problematic_slice);
		problematic_attacks |= problematic_slice;
	}
	patks = attacks_by_piece<make_piece(them, ROOK)>(pos, occ);
	for(Bitboard atk : patks){
		Bitboard problematic_slice = atk & our_king_zone;
		collisions += popcount(problematic_attacks & problematic_slice);
		problematic_attacks |= problematic_slice;
	}
	patks = attacks_by_piece<make_piece(them, QUEEN)>(pos, occ);
	for(Bitboard atk : patks){
		Bitboard problematic_slice = atk & our_king_zone;
		collisions += popcount(problematic_attacks & problematic_slice);
		problematic_attacks |= problematic_slice;
	}
	int atk_malus = popcount(problematic_attacks);
	int multi_atk_malus = 10 * collisions;
	//std::cout << atk_malus << "\n";
	return -3 * multi_atk_malus;
}
int evaluate(const Position& pos){
    int popcnts[12];
    Bitboard wocc = pos.get(WHITE);
    Bitboard bocc = pos.get(BLACK);
    Bitboard occ = wocc | bocc;
    constexpr int weights[12] = {100,310,330,500,900,0,-100,-310,-330,-500,-900,0};
    for(size_t i = 0;i < 12;i++){
        popcnts[i] = popcount(pos.piece_boards[i]);
    }
    int w_minus_b_mat = 0;
    for(size_t i = 0;i < 12;i++){
        w_minus_b_mat += popcnts[i] * weights[i];
    }
    int wcenter = center.eval<WHITE>(pos.get<W_PAWN>()) + advanced_center.eval<WHITE>(pos.get<W_PAWN>());
    int bcenter = center.eval<BLACK>(pos.get<B_PAWN>()) + advanced_center.eval<BLACK>(pos.get<B_PAWN>());

    w_minus_b_mat += wcenter - bcenter;
    int wpawn_protectivity = popcount(pawn_attacks_bb<WHITE>(pos.piece_boards[0]) & wocc);
    int bpawn_protectivity = popcount(pawn_attacks_bb<BLACK>(pos.piece_boards[6]) & bocc);
    w_minus_b_mat += (wpawn_protectivity - bpawn_protectivity) * 10;
    
    int wknightspos = knights.eval<WHITE>(pos.get<W_KNIGHT>()) + knights_border.eval<WHITE>(pos.get<W_KNIGHT>());
    int bknightspos = knights.eval<BLACK>(pos.get<B_KNIGHT>()) + knights_border.eval<BLACK>(pos.get<B_KNIGHT>());
    w_minus_b_mat += wknightspos - bknightspos;

    int wkings = kings(pos.get<W_KING>());
    int bkings = kings.eval<BLACK>(pos.get<B_KING>());
    //int wrooks = rooks(pos.get<W_ROOK>());
    //int brooks = rooks(flipVertical(pos.get<B_ROOK>()));
    w_minus_b_mat += wkings - bkings;
    //w_minus_b_mat += wrooks - brooks;
    //return pos.at_move == WHITE ? w_minus_b_mat : -w_minus_b_mat;

    w_minus_b_mat += (popcount(pos.get<W_BISHOP>()) > 1 ? 40 : 0);
    w_minus_b_mat -= (popcount(pos.get<B_BISHOP>()) > 1 ? 40 : 0);
    biterator biter(0);
    int wmobbonus = 0;
    int bmobbonus = 0;
    Bitboard w_kingfield = KingTwoMoves[lsb(pos.get<W_KING>())];
    Bitboard b_kingfield = KingTwoMoves[lsb(pos.get<B_KING>())];
    biter = biterator(pos.get<W_ROOK>());
    while(*biter){
        Bitboard ataks = attacks_bb<ROOK>(lsb(*biter), occ);
        wmobbonus += popcount(ataks);
        wmobbonus += 5 * popcount(ataks & b_kingfield);
        ++biter;
    }
    biter = biterator(pos.get<W_BISHOP>());
    while(*biter){
        Bitboard ataks = attacks_bb<BISHOP>(lsb(*biter), occ);
        wmobbonus += popcount(ataks);
        wmobbonus += 5 * popcount(ataks & b_kingfield);
        ++biter;
    }
    biter = biterator(pos.get<B_ROOK>());
    while(*biter){
        Bitboard ataks = attacks_bb<ROOK>(lsb(*biter), occ);
        bmobbonus += popcount(ataks);
        wmobbonus += 5 * popcount(ataks & w_kingfield);
        ++biter;
    }
    biter = biterator(pos.get<B_BISHOP>());
    while(*biter){
        Bitboard ataks = attacks_bb<BISHOP>(lsb(*biter), occ);
        bmobbonus += popcount(ataks);
        wmobbonus += 5 * popcount(ataks & w_kingfield);
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
            bpassed += passed_scores[7 - lsb(*biter) / 8];
        }
        ++biter;
    }
    int w_outposts = 0, b_outposts = 0;

    Bitboard pc = pos.get<W_KNIGHT>();
    Bitloop(pc){
        Square psq = lsb(pc);
        Bitboard passed_field = outpost_span(WHITE, psq);
        if((passed_field & pos.get<B_PAWN>()) == 0){
            w_outposts += 40;
        }
        Bitboard pseudos = PseudoAttacks[KNIGHT][psq];
        pseudos &= ~(pawn_attacks_bb<BLACK>(pos.get<B_PAWN>()));
        w_outposts += 5 * popcount(pseudos);
    }
    pc = pos.get<B_KNIGHT>();
    Bitloop(pc){
        Square psq = lsb(pc);
        Bitboard passed_field = outpost_span(BLACK, psq);
        if((passed_field & pos.get<W_PAWN>()) == 0){
            b_outposts += 40;
        }
        Bitboard pseudos = PseudoAttacks[KNIGHT][psq];
        pseudos &= ~(pawn_attacks_bb<WHITE>(pos.get<W_PAWN>()));
        b_outposts += 5 * popcount(pseudos);
    }
	int w_kingsafety = king_safety<WHITE>(pos, occ);
	int b_kingsafety = king_safety<BLACK>(pos, occ);

    w_minus_b_mat += (w_kingsafety - b_kingsafety);
    w_minus_b_mat += (wpassed - bpassed);
    w_minus_b_mat += (w_outposts - b_outposts);
    if(pos.at_move == BLACK)return -w_minus_b_mat;
    return w_minus_b_mat;
}