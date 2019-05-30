#include "position.hpp"
#include "io.hpp"
#include "bitboard.hpp"
const Bitboard& Position::get(Piece p)const{
	return piece_boards[compress_piece(p)];
}
Bitboard& Position::get(Piece p){
	return piece_boards[compress_piece(p)];
}
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
stackvector<complete_move, 80>Position::generate_trivial(Color color) const{
	Bitboard occupied = this->occupied();
	Bitboard ours = get(color);
	Bitboard theirs = get(opp(color));
	stackvector<complete_move, 80> moves;
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