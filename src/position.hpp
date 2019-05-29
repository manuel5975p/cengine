#ifndef POSITION_HPP_INCLUDED
#define POSITION_HPP_INCLUDED
#include "types.hpp"
#include <string>
template<Piece p> constexpr size_t compress_piece()  {return -1;}
template<> constexpr size_t compress_piece<W_PAWN>  (){return 0;}
template<> constexpr size_t compress_piece<W_KNIGHT>  (){return 1;}
template<> constexpr size_t compress_piece<W_BISHOP>(){return 2;}
template<> constexpr size_t compress_piece<W_ROOK>(){return 3;}
template<> constexpr size_t compress_piece<W_QUEEN> (){return 4;}
template<> constexpr size_t compress_piece<W_KING>  (){return 5;}
template<> constexpr size_t compress_piece<B_PAWN>  (){return 6;}
template<> constexpr size_t compress_piece<B_KNIGHT>  (){return 7;}
template<> constexpr size_t compress_piece<B_BISHOP>(){return 8;}
template<> constexpr size_t compress_piece<B_ROOK>(){return 9;}
template<> constexpr size_t compress_piece<B_QUEEN> (){return 10;}
template<> constexpr size_t compress_piece<B_KING>  (){return 11;}
constexpr static size_t compress_piece(Piece p){
	switch(p){
		case W_PAWN    :return 0 ;     break;
		case W_KNIGHT  :return 1 ;     break;
		case W_BISHOP  :return 2 ;     break;
		case W_ROOK    :return 3 ;     break;
		case W_QUEEN   :return 4 ;     break;
		case W_KING    :return 5 ;     break;
		case B_PAWN    :return 6 ;     break;
		case B_KNIGHT  :return 7 ;     break;
		case B_BISHOP  :return 8 ;     break;
		case B_ROOK    :return 9 ;     break;
		case B_QUEEN   :return 10;     break;
		case B_KING    :return 11;     break;
		default        :return -1;     break;
	}
}
struct Position{
	std::array<Bitboard, 12> piece_boards = {};
	Position(){
		get(W_PAWN) = 0xff00;
		get(W_ROOK) = (1ULL | 1ULL << 7);
		get(W_KNIGHT) = (1ULL << 1| 1ULL << 6);
		get(W_BISHOP) = (1ULL << 2| 1ULL << 5);
		get(W_KING) = (1ULL << 3);
		get(W_QUEEN) = (1ULL << 4);
		
		get(B_PAWN) = 0xff000000000000;
		get(B_ROOK) = (1ULL << 63 | 1ULL << 56);
		get(B_KNIGHT) = (1ULL << 57 | 1ULL << 62);
		get(B_BISHOP) = (1ULL << 58 | 1ULL << 61);
		get(B_KING) = (1ULL << 59);
		get(B_QUEEN) = (1ULL << 60);
	}
	template<Piece p>
	Bitboard get()const{
		return piece_boards[compress_piece<p>()];
	}
	template<Piece p>
	Bitboard& get(){
		return piece_boards[compress_piece<p>()];
	}
	const Bitboard& get(Piece p)const;
	Bitboard& get(Piece p);
	std::string to_string()const;
	Bitboard generate_trivial(Color c)const;
	Bitboard occupied()const;
};
#endif //POSITION_HPP_INCLUDED
