#include "position.hpp"
#include "io.hpp"
Bitboard Position::get(Piece p)const{
	return piece_boards[compress_piece(p)];
}
Bitboard& Position::get(Piece p){
	return piece_boards[compress_piece(p)];
}
std::string Position::to_string()const{
	std::string ret = bb_to_string(get(W_PAWN), W_PAWN);
	for(int i = 1;i < 12;i++){
		ret = ret | bb_to_string(piece_boards[i], piece_types[i]);
	}
	return ret;
}