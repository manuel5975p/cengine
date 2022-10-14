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
#include <immintrin.h>
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
#define Bitloop(X) for(;X; X = _blsr_u64(X))
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
	bool sane()const{
		Bitboard accum = 0;
		for(size_t i = 0;i < 12;i++){
			if(piece_boards[i] & accum){
				return false;
			}
			accum |= piece_boards[i];
		}
		return true;
	}
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
		return spec_mem.hash ^ hash_int(at_move) ^ (hash_int(spec_mem.ep) << 17);
	}
	stackvector<turbomove, 256> generate_new(Color c){
		if(c == WHITE){
			return generate_new<WHITE>();
		}
		return generate_new<BLACK>();
	}
	stackvector<turbomove, 256> generate_loud(Color c){
		if(c == WHITE){
			return generate_loud<WHITE>();
		}
		return generate_loud<BLACK>();
	}
	template<Color we>
	stackvector<turbomove, 256> generate_loud()const;
	template<Color we>
	stackvector<turbomove, 256> generate_new()const;
	
	void apply_move(const turbomove& tm){
		piece_boards[tm.index1] ^= tm.bb1;
		if(tm.index2 < 12)
			piece_boards[tm.index2] ^= tm.bb2;
		//if(false)
		{
			Bitboard b1 = tm.bb1;
			Bitboard b2 = tm.bb2;
			Bitloop(b1){
				spec_mem.hash ^= global_zobrist_table.values[tm.index1][int(lsb(b1))];
			}
			if(tm.index2 < 12)
			Bitloop(b2){
				spec_mem.hash ^= global_zobrist_table.values[tm.index2][int(lsb(b2))];
			}
		}
		constexpr Bitboard castling_interesting = 
		(1ull << SQ_A1) | (1ull << SQ_E1) | (1ull << SQ_H1) | (1ull << SQ_A8) | (1ull << SQ_E8) | (1ull << SQ_H8);
		if((tm.bb1 | tm.bb2) & castling_interesting)
			{ //Castling schisdreck
			for(Bitboard b : {tm.bb1, tm.bb2}){
				if(b & 1ull << SQ_A1){
					spec_mem.hash ^= ((spec_mem.cr & WHITE_OOO) ? global_zobrist_table.castling_values[1] : 0);
					spec_mem.cr &= ~WHITE_OOO;
				}
				if(b & 1ull << SQ_H1){
					spec_mem.hash ^= ((spec_mem.cr & WHITE_OO) ? global_zobrist_table.castling_values[0] : 0);
					spec_mem.cr &= ~WHITE_OO;
				}
				if(b & 1ull << SQ_E1){
					spec_mem.hash ^= ((spec_mem.cr & WHITE_OO) ? global_zobrist_table.castling_values[0] : 0);
					spec_mem.hash ^= ((spec_mem.cr & WHITE_OOO) ? global_zobrist_table.castling_values[1] : 0);
					spec_mem.cr &= ~WHITE_CASTLING;
				}
			}
			for(Bitboard b : {tm.bb1, tm.bb2}){
				if(b & 1ull << SQ_A8){
					spec_mem.hash ^= ((spec_mem.cr & BLACK_OOO) ? global_zobrist_table.castling_values[3] : 0);
					spec_mem.cr &= ~BLACK_OOO;
				}
				if(b & 1ull << SQ_H8){
					spec_mem.hash ^= ((spec_mem.cr & BLACK_OO) ? global_zobrist_table.castling_values[2] : 0);
					spec_mem.cr &= ~BLACK_OO;
				}
				if(b & 1ull << SQ_E8){
					spec_mem.hash ^= ((spec_mem.cr & BLACK_OOO) ? global_zobrist_table.castling_values[3] : 0);
					spec_mem.hash ^= ((spec_mem.cr & BLACK_OO) ? global_zobrist_table.castling_values[2] : 0);
					spec_mem.cr &= ~BLACK_CASTLING;
				}
			}
			if((tm.flags >> 5) & 3){
				auto rigts = spec_mem.cr;
				if(at_move == WHITE){
					if(tm.flags >> 5 & 1){
						spec_mem.cr &= ~WHITE_OO;
					}
					if(tm.flags >> 6 & 1){
						spec_mem.cr &= ~WHITE_OOO;
					}
					if(tm.index1 == 5){
						spec_mem.cr &= ~WHITE_OO;
						spec_mem.cr &= ~WHITE_OOO;
					}
				}
				else{
					if(tm.flags >> 5 & 1){
						spec_mem.cr &= ~BLACK_OO;
					}
					if(tm.flags >> 6 & 1){
						spec_mem.cr &= ~BLACK_OOO;
					}
					if(tm.index1 == 11){
						spec_mem.cr &= ~BLACK_OO;
						spec_mem.cr &= ~BLACK_OOO;
					}
				}
				if(rigts != spec_mem.cr){
					std::cout << "ALAAAARM" << std::endl;
					std::abort();
				}
			}
			
		}
		spec_mem.ep = SQ_NONE;
		constexpr Bitboard pawnstarts = rank_bb(RANK_2) | rank_bb(RANK_7);
		if((tm.bb1 & pawnstarts) && (tm.index1 == 0 || tm.index1 == 6)){
			if(lsb(_blsr_u64(tm.bb1)) - lsb(tm.bb1) == 16){
				spec_mem.ep = Square((lsb(_blsr_u64(tm.bb1)) + lsb(tm.bb1)) / 2);
			}
		}
		if(tm.flags >> 2 & 7){
			//std::cout << "doing promotion" << std::endl;
			//std::cout << "Square " << lsb(_blsr_u64(tm.bb1)) << std::endl;
			Bitboard promotion_mask = ((tm.flags & 2) ? (tm.bb1 ^ _blsr_u64(tm.bb1)) : _blsr_u64(tm.bb1));
			int index = tm.flags >> 2 & 7;
			int realindex = 5 - index + (at_move == BLACK ? 6 : 0);

			piece_boards[realindex] ^= promotion_mask;
			piece_boards[at_move == BLACK ? 6 : 0] ^= promotion_mask;
			
			spec_mem.hash ^= global_zobrist_table.values[realindex][lsb(tm.bb2)];
			spec_mem.hash ^= global_zobrist_table.values[at_move == BLACK ? 6 : 0][lsb(tm.bb2)];
		}
		at_move = ~at_move;
		//if(hash() != spec_mem.hash)std::terminate();
		//Bitboard b1 = tm.bb1;
		//Bitloop(b1){
		//	spec_mem.hash ^= zobrist_table[tm.index1][lsb(b1)];
		//}
	}
	void undo_move(const turbomove& tm){
		at_move = ~at_move;
		piece_boards[tm.index1] ^= tm.bb1;
		if(tm.index2 < 12)
			piece_boards[tm.index2] ^= tm.bb2;
		if(tm.flags >> 2 & 7){
			Bitboard promotion_mask = ((tm.flags & 2) ? (tm.bb1 ^ _blsr_u64(tm.bb1)) : _blsr_u64(tm.bb1));
			int index = tm.flags >> 2 & 7;
			int realindex = 5 - index + (at_move == BLACK ? 6 : 0);
			piece_boards[realindex] ^= promotion_mask;
			piece_boards[at_move == BLACK ? 6 : 0] ^= promotion_mask;
		}
		
	}
};
extern template stackvector<turbomove, 256> Position::generate_loud<WHITE>()const;
extern template stackvector<turbomove, 256> Position::generate_loud<BLACK>()const;
extern template stackvector<turbomove, 256> Position::generate_new<WHITE>()const;
extern template stackvector<turbomove, 256> Position::generate_new<BLACK>()const;
template<typename iterator, typename func>
void sort_based_on(iterator begin, iterator end, func f){
    using T = typename std::iterator_traits<iterator>::value_type;
    using func_type = decltype(f(std::declval<T>()));
    std::vector<std::pair<T, func_type>> buf(end - begin);
    size_t index = 0;
    for(auto it = begin;it != end;it++){
        buf[index++] = {*it, f(*it)};
    }
    std::sort(buf.begin(), buf.end(), [](const std::pair<T, func_type>& a, const std::pair<T, func_type>& b){
        return a.second > b.second;
    });
    index = 0;
    for(auto it = begin;it != end;it++){
        *it = buf[index++].first;
    }
}
#endif //POSITION_HPP_INCLUDED
