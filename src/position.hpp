#ifndef POSITION_HPP_INCLUDED
#define POSITION_HPP_INCLUDED
#include "types.hpp"
#include <string>
template<Piece p> constexpr size_t compress_piece(){return -1;}
template<> constexpr size_t compress_piece<W_PAWN>  (){return 0;}
template<> constexpr size_t compress_piece<W_ROOK>  (){return 1;}
template<> constexpr size_t compress_piece<W_KNIGHT>(){return 2;}
template<> constexpr size_t compress_piece<W_BISHOP>(){return 3;}
template<> constexpr size_t compress_piece<W_QUEEN> (){return 4;}
template<> constexpr size_t compress_piece<W_KING>  (){return 5;}
template<> constexpr size_t compress_piece<B_PAWN>  (){return 6;}
template<> constexpr size_t compress_piece<B_ROOK>  (){return 7;}
template<> constexpr size_t compress_piece<B_KNIGHT>(){return 8;}
template<> constexpr size_t compress_piece<B_BISHOP>(){return 9;}
template<> constexpr size_t compress_piece<B_QUEEN> (){return 10;}
template<> constexpr size_t compress_piece<B_KING>  (){return 11;}
constexpr static size_t compress_piece(Piece p){
	switch(p){
		case W_PAWN    :return 0 ;     break;
		case W_ROOK    :return 1 ;     break;
		case W_KNIGHT  :return 2 ;     break;
		case W_BISHOP  :return 3 ;     break;
		case W_QUEEN   :return 4 ;     break;
		case W_KING    :return 5 ;     break;
		case B_PAWN    :return 6 ;     break;
		case B_ROOK    :return 7 ;     break;
		case B_KNIGHT  :return 8 ;     break;
		case B_BISHOP  :return 9 ;     break;
		case B_QUEEN   :return 10;     break;
		case B_KING    :return 11;     break;
		default        :return -1;     break;
	}
}
struct Position{
	Bitboard piece_boards[12];
	template<Piece p>
	Bitboard get()const{
		return piece_boards[compress_piece<p>()];
	}
	template<Piece p>
	Bitboard& get(){
		return piece_boards[compress_piece<p>()];
	}
	Bitboard get(Piece p)const;
	Bitboard& get(Piece p);
	std::string to_string()const;
};
#endif //POSITION_HPP_INCLUDED