#ifndef IO_HPP_INCLUDED
#define IO_HPP_INCLUDED
#include "types.hpp"
#include <string>
#include <cstddef>
#include <cassert>
#include <iostream>
template<Piece p> inline char pieceChar   (){return 'X';}
template<> inline char pieceChar<W_PAWN>  (){return 'P';}
template<> inline char pieceChar<W_ROOK>  (){return 'R';}
template<> inline char pieceChar<W_KNIGHT>(){return 'N';}
template<> inline char pieceChar<W_BISHOP>(){return 'B';}
template<> inline char pieceChar<W_QUEEN> (){return 'Q';}
template<> inline char pieceChar<W_KING>  (){return 'K';}
template<> inline char pieceChar<B_PAWN>  (){return 'p';}
template<> inline char pieceChar<B_ROOK>  (){return 'r';}
template<> inline char pieceChar<B_KNIGHT>(){return 'n';}
template<> inline char pieceChar<B_BISHOP>(){return 'b';}
template<> inline char pieceChar<B_QUEEN> (){return 'q';}
template<> inline char pieceChar<B_KING>  (){return 'k';}
inline constexpr Piece charPiece(char x){
	switch(x){
		case 'P':return W_PAWN  ;     break;
		case 'R':return W_ROOK  ;     break;
		case 'N':return W_KNIGHT;     break;
		case 'B':return W_BISHOP;     break;
		case 'Q':return W_QUEEN ;     break;
		case 'K':return W_KING  ;     break;
		case 'p':return B_PAWN  ;     break;
		case 'r':return B_ROOK  ;     break;
		case 'n':return B_KNIGHT;     break;
		case 'b':return B_BISHOP;     break;
		case 'q':return B_QUEEN ;     break;
		case 'k':return B_KING  ;     break;
	default      :return NO_PIECE;     break;
	}
}
inline constexpr char pieceChar(Piece p){
	switch(p){
	case W_PAWN  :return 'P';break;
	case W_ROOK  :return 'R';break;
	case W_KNIGHT:return 'N';break;
	case W_BISHOP:return 'B';break;
	case W_QUEEN :return 'Q';break;
	case W_KING  :return 'K';break;
	case B_PAWN  :return 'p';break;
	case B_ROOK  :return 'r';break;
	case B_KNIGHT:return 'n';break;
	case B_BISHOP:return 'b';break;
	case B_QUEEN :return 'q';break;
	case B_KING  :return 'k';break;
	default      :return 'X';break;
	}
}
inline std::string square_to_string(Square sq){
	std::string ret(2,2);
	int square(sq);
	ret[0] = 'a' + square % 8;
	ret[1] = '0' + square / 8 + 1;
	return ret;
}
inline Square string_to_square(const std::string& str){
	std::string ret(2,2);
	int file, rank;
	file = str[0] - 'a';
	rank = str[1] - '0' - 1;
	return Square(rank * 8 + file);
}
inline std::string bb_to_string(Bitboard x, Piece p = NO_PIECE){
	std::string ret(72, 0);
	std::size_t pos = 0;
	for(std::size_t i = 56;i <= 56;i -= 8){
		for(std::size_t j = 0;j <= 7;j++){
			ret[pos++] = !!(x & (1ULL << (i + j))) ? pieceChar(p) : '.';
		}
		ret[pos++] = '\n';
	}
	return ret;
}
inline void print(Bitboard x, std::ostream& out = std::cout){
	out << bb_to_string(x);
}
inline std::string operator|(const std::string& a, const std::string& b){
	assert(a.size() == b.size() && "Combining strings with different lengths.");
	std::string ret(a.size(), '.');
	for(std::size_t i = 0;i < a.size();i++){
		assert((a[i] == '.' || b[i] == '.' || (a[i] == '\n' && b[i] == '\n')) && "Occupancies coincide");
		if(a[i] != '.')ret[i] = a[i];
		else if(b[i] != '.'){ret[i] = b[i];}
	}
	return ret;
}

#endif //IO_HPP_INCLUDED
