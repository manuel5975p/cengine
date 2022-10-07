#ifndef POSITION_HPP_INCLUDED
#define POSITION_HPP_INCLUDED
#include "types.hpp"
#include "move.hpp"
#include "stackvector.hpp"
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
//#include <Eigen/Dense>
#include <xoshiro.hpp>
using hash_int = std::uint64_t;
struct zobrist_table{
	std::array<std::array<hash_int, 64>, 12> values;
	std::array<hash_int, 4> castling_values;
	zobrist_table(){
		xoshiro_256 gen(~69420u);
		std::uniform_int_distribution<hash_int> dis(0, std::numeric_limits<hash_int>::max());
		for(size_t i = 0;i < 12;i++){
			for(size_t j = 0;j < 64;j++){
				values[i][j] = dis(gen);
			}
		}
		for(size_t i = 0;i < 4;i++){
			castling_values[i] = dis(gen);
		}
	}
};
extern zobrist_table global_zobrist_table;
struct special_members{
	CastlingRight cr;
	std::uint8_t since_capture;
	hash_int hash;
	Square ep;
	special_members() : cr(ANY_CASTLING), since_capture(0), hash(0), ep(SQ_NONE){}
	bool operator==(const special_members& other)const{
		return cr == other.cr && since_capture == other.since_capture && hash == other.hash && ep == other.ep;
	}
};
struct Position{
	std::array<Bitboard, 12> piece_boards = {};
	Color at_move;
	special_members spec_mem;
	Position(Bitboard b){
		std::fill(piece_boards.begin(), piece_boards.end(), b);
	}
	Position(const std::string& fen);
	Position(){
		get(W_PAWN) = 0xff00;
		get(W_ROOK) = (1ULL | 1ULL << 7);
		get(W_KNIGHT) = (1ULL << 1| 1ULL << 6);
		get(W_BISHOP) = (1ULL << 2| 1ULL << 5);
		get(W_KING) = (1ULL << 4);
		get(W_QUEEN) = (1ULL << 3);
		
		get(B_PAWN) = 0xff000000000000;
		get(B_ROOK) = (1ULL << 63 | 1ULL << 56);
		get(B_KNIGHT) = (1ULL << 57 | 1ULL << 62);
		get(B_BISHOP) = (1ULL << 58 | 1ULL << 61);
		get(B_KING) = (1ULL << 60);
		get(B_QUEEN) = (1ULL << 59);
		at_move = WHITE;
		spec_mem.hash = this->hash();
	}
	bool operator==(const Position& other)const{
		bool a = std::equal(other.piece_boards.begin(), other.piece_boards.end(), piece_boards.begin());
		return a && (spec_mem == other.spec_mem);
	}
	template<Piece p>
	Bitboard get()const{
		return piece_boards[compress_piece<p>()];
	}
	template<Piece p>
	Bitboard& get(){
		return piece_boards[compress_piece<p>()];
	}
	const Bitboard& get(Piece p)const{
		return piece_boards[compress_piece(p)];
	}
	hash_int hash()const{
		hash_int h(0);
		for(const auto& piece : pieces){
			Bitboard bb = get(piece);
			biterator it(bb);
			while(*it){
				Bitboard singlebit = *it;
				h ^= global_zobrist_table.values[compress_piece(piece)][lsb(singlebit)];
				it++;
			}
		}
		biterator biter(spec_mem.cr);
		while(*biter){
			h ^= global_zobrist_table.castling_values[lsb(*biter)];
			biter++;
		}
		return h;
	}
	Bitboard& get(Piece p){
		return piece_boards[compress_piece(p)];
	}

	const Bitboard& get(Color c, PieceType p)const{
		return piece_boards[compress_piece(Piece(p + (c << 3)))];
	}
	Bitboard& get(Color c, PieceType p){
		return piece_boards[compress_piece(Piece(p + (c << 3)))];
	}
	Bitboard get(Color c)const;
	
	std::string to_string()const;
	stackvector<complete_move, 256> generate_trivial(Color c)const;
	stackvector<complete_move, 256> generate_legal(Color c)const;
	std::vector<Position> generate_all_successors()const;
	//Eigen::VectorXf to_one_hot_repr()const;
	bool check(Color c)const;
	bool under_attack_for(Color c, Square s)const;
	void apply_move(const complete_move& move);
	void revert_move(const complete_move& move);
	bool revert_move_checked(const complete_move& move);
	bool apply_move_checked(const complete_move& move);
	Bitboard occupied()const;
	std::string fen()const;
	hash_int quickhash()const{
		assert(spec_mem.hash == hash());
		return spec_mem.hash ^ hash_int(at_move);
	}
};
#endif //POSITION_HPP_INCLUDED
