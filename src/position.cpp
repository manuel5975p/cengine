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
std::vector<complete_move> Position::generate_trivial(Color color) const{
	Bitboard occupied = 0;//this->occupied();
	Bitboard attackable = ~get(color);
	std::vector<complete_move> moves;
	moves.reserve(120);
	if(color == WHITE){
		for(Piece p : white_pieces){
			Bitboard pboard = this->get(p);
			
			while(pboard){
				Bitboard single_piece = pboard & -pboard;
				Bitboard attacks = attacks_bb<WHITE>(get_type(p), lsb(single_piece), occupied);
				print(attacks);
				attacks &= attackable;
				while(attacks){
					Bitboard attack_bit = attacks & -attacks;
					moves.emplace_back(attack_bit | single_piece, p);
					attacks ^= attack_bit;
				}
				pboard ^= single_piece;
			}
		}
	}
	else if(color == BLACK){
		
	}
	return moves;
}