#include "position.hpp"
#include "io.hpp"
#include "bitboard.hpp"
std::string Position::to_string()const{
	std::string ret = bb_to_string(get(W_PAWN), W_PAWN);
	for(int i = 1;i < 12;i++){
		ret = ret | bb_to_string(piece_boards[i], pieces[i]);
	}
	return ret;
}
Bitboard Position::get(Color c)const{
	Bitboard x(0);
	if(c == WHITE){
		for(Piece p : white_pieces){
			x |= this->get(p);
		}
	}
	else if(c == BLACK){
		for(Piece p : black_pieces){
			x |= this->get(p);
		}
	}
	return x;
}
Bitboard Position::occupied()const{
	Bitboard x(0);
	for(Bitboard b : piece_boards){
		x |= b;
	}
	return x;
}
stackvector<complete_move, 256> Position::generate_trivial(Color color) const{
	Bitboard occupied = this->occupied();
	Bitboard ours = get(color);
	Bitboard theirs = get(opp(color));
	stackvector<complete_move, 256> moves;
	if(color == WHITE){
		Bitboard pboard = this->get(W_PAWN);
		while(pboard){
			Bitboard single_piece = pboard & -pboard;
			Bitboard attacks = pawn_attacks<WHITE>(lsb(single_piece), occupied, theirs);
			while(attacks){
				Bitboard attack_bit = attacks & -attacks;
				moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), W_PAWN));
				attacks ^= attack_bit;
			}
			pboard ^= single_piece;
		}
		for(size_t i = 1; i < white_pieces.size();i++){
			Piece p = white_pieces[i];
			Bitboard pboard = this->get(p);
			
			while(pboard){
				Bitboard single_piece = pboard & -pboard;
				Bitboard attacks = attacks_bb(get_type(p), lsb(single_piece), occupied);
				attacks &= (~ours);
				while(attacks){
					Bitboard attack_bit = attacks & -attacks;
					moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), p));
					attacks ^= attack_bit;
				}
				pboard ^= single_piece;
			}
		}
	}
	else if(color == BLACK){
		Bitboard pboard = this->get(B_PAWN);
		while(pboard){
			Bitboard single_piece = pboard & -pboard;
			Bitboard attacks = pawn_attacks<BLACK>(lsb(single_piece), occupied, theirs);
			while(attacks){
				Bitboard attack_bit = attacks & -attacks;
				moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), B_PAWN));
				attacks ^= attack_bit;
			}
			pboard ^= single_piece;
		}
		for(size_t i = 1; i < black_pieces.size();i++){
			Piece p = black_pieces[i];
			Bitboard pboard = this->get(p);
			while(pboard){
				Bitboard single_piece = pboard & -pboard;
				Bitboard attacks = attacks_bb(get_type(p), lsb(single_piece), occupied);
				attacks &= (~ours);
				while(attacks){
					Bitboard attack_bit = attacks & -attacks;
					moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), p));
					attacks ^= attack_bit;
				}
				pboard ^= single_piece;
			}
		}
	}
	return moves;
}
void Position::apply_move(const complete_move& move){
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
}
void Position::revert_move(const complete_move& move){
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
}
void Position::revert_move_checked(const complete_move& move){
	assert(color_of(move.moving_piece) != this->at_move);
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	at_move = ~at_move;
}
void Position::apply_move_checked(const complete_move& move){
	assert(color_of(move.moving_piece) == this->at_move);
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	assert(!check(at_move));
	at_move = ~at_move;
}
bool Position::check(Color c)const{
	Bitboard king_bit;
	Bitboard enemy_rooks;
	Bitboard enemy_bishops;
	Bitboard enemy_knights;
	Bitboard enemy_queens;
	Bitboard enemy_kings;
	Bitboard enemy_pawns;

	if(c == BLACK){
		king_bit = get(B_KING);
		enemy_rooks   = get(W_ROOK);
		enemy_bishops = get(W_BISHOP);
		enemy_knights = get(W_KNIGHT);
		enemy_queens  = get(W_QUEEN);
		enemy_kings   = get(W_KING);
		enemy_pawns   = get(W_PAWN);
	}
	else{
		king_bit = get(W_KING);
		enemy_rooks   = get(B_ROOK);
		enemy_bishops = get(B_BISHOP);
		enemy_knights = get(B_KNIGHT);
		enemy_queens  = get(B_QUEEN);
		enemy_kings   = get(B_KING);
		enemy_pawns   = get(B_PAWN);
	}
	Bitboard occ = this->occupied();
	Square king_pos = lsb(king_bit);
	if(attacks_bb<ROOK>(king_pos, occ) & (enemy_rooks | enemy_queens)){
		return true;
	}
	if(attacks_bb<BISHOP>(king_pos, occ) & (enemy_bishops | enemy_queens)){
		return true;
	}
	if(attacks_bb<KING>(king_pos, occ) & (enemy_kings | enemy_queens)){
		return true;
	}
	if(attacks_bb<KNIGHT>(king_pos, occ) & enemy_knights){
		return true;
	}
	if(c == WHITE){
		if(shift<NORTH_WEST>(king_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<NORTH_EAST>(king_bit) & (enemy_pawns)){
			return true;
		}
	}
	else{
		if(shift<SOUTH_WEST>(king_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<SOUTH_EAST>(king_bit) & (enemy_pawns)){
			return true;
		}
	}
	return false;
}
stackvector<complete_move, 256> Position::generate_legal(Color c)const{
	stackvector<complete_move, 256> ret = generate_trivial(c);
	Position p(*this);
	
	for(auto it = ret.begin();it != ret.end();){
		complete_move cmm = *it;
		p.apply_move(cmm);
		if(p.check(c)){
			std::swap(*it, *(ret.end() - 1));
			ret.pop_back();
		}
		else{
			it++;
		}
		p.revert_move(cmm);
	}
	return ret;
}