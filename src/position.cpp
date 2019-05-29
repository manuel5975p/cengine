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
Bitboard Position::occupied()const{
	Bitboard x(0);
	for(Bitboard b : piece_boards){
		x |= b;
	}
	return x;
}
Bitboard Position::generate_trivial(Color color) const{
	Bitboard occupied = this->occupied();
	
	if(color == WHITE){

	}
	else if(color == BLACK){
		
	}
	return 0;
}