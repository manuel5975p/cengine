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
	stackvector<complete_move, 256> generate_new(Color c){

	}
	template<Color we>
	stackvector<turbomove, 256> generate_new(){
		stackvector<turbomove, 256> moves;
		Bitboard pinlines[64];
		std::fill(pinlines, pinlines + 64, ~0ull);
		Bitboard checklines = 0;
		constexpr Color them = ~we;
		unsigned checkcount = 0;
		Bitboard our_king_bitboard = piece_boards[compress_piece(make_piece(we, KING))];
		Square our_king_square = lsb(piece_boards[compress_piece(make_piece(we, KING))]);
		Bitboard our_pieces = get(we);
		Bitboard their_pieces = get(them);
		Bitboard occ = our_pieces | their_pieces;
		Bitboard occ_for_pseudo = occ ^ (1ull << our_king_square);
		Bitboard their_pseudo = 0;
		for(PieceType x : {BISHOP, ROOK, QUEEN, KNIGHT, KING}){
			Bitboard enemy_occ = get(them, x);
			Bitloop(enemy_occ){
				Square enemy_square = lsb(enemy_occ);
				Bitboard attacks = attacks_bb(x, enemy_square, occ_for_pseudo);
				their_pseudo |= attacks;
			}
		}
		their_pseudo |= pawn_attacks_bb<them>(get(them, PAWN));

		Piece piece_map[64] = {NO_PIECE};
		for(auto& pi : pieces){
			Bitboard pib = get(pi);
			while(pib){
				Square x = lsb(pib);
				piece_map[x] = pi;
				pib ^= (1ULL << x);
			}
		}
		for(PieceType x : {BISHOP, ROOK, QUEEN}){
			Piece enemy_slider = make_piece(them, x);
			Bitboard es_bb = get(enemy_slider);
			Bitloop(es_bb){
				Square single_square = lsb(es_bb);
				Bitboard attackline = 0;
				if(our_king_bitboard & PseudoAttacks[x][single_square])
					attackline = (LineBetween[single_square][our_king_square] & (1ull << single_square | PseudoAttacks[x][single_square]));
				//if(attackline){
				//	print(attackline);
				//}
				/*if(LineBetween[single_square][our_king_square]){
					attackline |= (1ull << single_square);
				}*/
				if((attackline & (their_pieces & ~_blsi_u64(es_bb))) == 0){
					if(popcount(attackline & our_pieces) == 1){
						pinlines[lsb(attackline & our_pieces)] ^= ~attackline;
						pinlines[lsb(attackline & our_pieces)] |= (1ull << single_square);
					}
					if(attackline && popcount(attackline & our_pieces) == 0){
						checklines |= attackline;
						checklines |= (1ull << single_square);
						checkcount++;
					}
				}
			}
		}
		
		checkcount += popcount(attacks_bb(KNIGHT, our_king_square, occ) & get(them, KNIGHT));
		checklines |= (attacks_bb(KNIGHT, our_king_square, occ) & get(them, KNIGHT));
		checkcount += popcount(pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN));
		checklines |= pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN);
		//print(checklines);
		//std::cout << checkcount << "\n";
		Bitboard checkmask_for_normal_figures;

		if(checkcount == 0)checkmask_for_normal_figures = ~0ull;
		else if(checkcount == 1){checkmask_for_normal_figures = checklines;}
		else if(checkcount == 2)checkmask_for_normal_figures = 0;
		//std::cout << our_king_square << "\n";
		if(pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN)){
			if(spec_mem.ep != SQ_NONE)
				checkmask_for_normal_figures |= square_bb(spec_mem.ep);
		}
		for(PieceType pt : piece_types){
			Piece ourpiece = make_piece(we, pt);
			Bitboard our_pieces_ofthis = get(ourpiece);
			if(checkcount >= 2 && pt != KING)continue;
			if(pt == PAWN){
				Bitloop(our_pieces_ofthis){
					const Bitboard our_piece_single = _blsi_u64(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = pawn_attacks<we>(our_piece_square, occ, their_pieces, spec_mem.ep);
					attacks &= (pinlines[our_piece_square]);
					attacks &= checkmask_for_normal_figures;
					attacks &= ~last_rank<we>();
					Bitloop(attacks){
						Bitboard singleattack = _blsi_u64(attacks);
						Bitboard their_piece_clearance = singleattack;
						Piece their_klonked_piece = NO_PIECE;
						if(their_piece_clearance & their_pieces){
							their_klonked_piece = piece_map[lsb(their_piece_clearance)];
						}
						else if(spec_mem.ep != SQ_NONE && !!(singleattack & (1ull << spec_mem.ep))){
							their_piece_clearance = shift<we == WHITE ? SOUTH : NORTH>(their_piece_clearance);
							//print(their_piece_clearance);
							their_klonked_piece = make_piece(~we, PAWN);
						}
						else{
							their_piece_clearance = 0;
						}
						moves.push_back(turbomove{(unsigned short)(compress_piece(ourpiece)), (unsigned short)(compress_piece(their_klonked_piece)), 0, our_piece_single | singleattack, their_klonked_piece == NO_PIECE ? 0 : their_piece_clearance});
						if(int(lsb(singleattack)) < int(our_piece_square)){
							moves.back().flags |= (1 << 1);
						}
					}
				}
			}
			else if(pt == BISHOP || pt == ROOK || pt == QUEEN || pt == KNIGHT){
				Bitloop(our_pieces_ofthis){
					const Bitboard our_piece_single = _blsi_u64(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = attacks_bb(pt, our_piece_square, occ);
					attacks &= ~our_pieces;
					attacks &= (pinlines[our_piece_square]);
					
					attacks &= checkmask_for_normal_figures;
					if(pt == ROOK && our_piece_square == SQ_H1){
						//print(attacks);
					}
					Bitloop(attacks){
						Bitboard singleattack = _blsi_u64(attacks);
						int castling_klonk = 0;
						if constexpr(we == WHITE){
							castling_klonk |= (Bitboard(our_piece_square == SQ_H1 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A1 ? 1 : 0) << 6);
						}
						else{
							castling_klonk |= (Bitboard(our_piece_square == SQ_H8 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A8 ? 1 : 0) << 6);
						}
						moves.push_back(turbomove{short(compress_piece(ourpiece)), short(compress_piece(piece_map[lsb(attacks)])), castling_klonk, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
						if(int(lsb(singleattack)) < int(our_piece_square)){
							moves.back().flags |= (1 << 1);
						}
					}
				}
			}
			else{
				Bitboard attacks = PseudoAttacks[KING][our_king_square];
				const Bitboard our_piece_single = 1ull << our_king_square;
				attacks &= ~our_pieces;
				attacks &= ~their_pseudo;
				Bitloop(attacks){
					Bitboard singleattack = _blsi_u64(attacks);
					moves.push_back(turbomove{short(compress_piece(ourpiece)), short(compress_piece(piece_map[lsb(attacks)])), 0, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
					if(int(lsb(singleattack)) < int(our_king_square)){
						moves.back().flags |= (1 << 1);
					}
				}
			}
		}
		if(checkcount >= 2) return moves;
		if constexpr(we == WHITE){
			if((spec_mem.cr & WHITE_OO) && (W_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED & occ) == 0){
				if((their_pseudo & (W_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							compress_piece<W_KING>(),
							compress_piece<W_ROOK>(),
							1 | 3 << 5,
							(1ull << 4 | 1ull << 6),
							(1ull << 5 | 1ull << 7)
						}
					);
				}
			}
			if((spec_mem.cr & WHITE_OOO) && (W_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED & occ) == 0){
				if((their_pseudo & (W_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							short(compress_piece<W_KING>()),
							short(compress_piece<W_ROOK>()),
							3 | 3 << 5,
							(1ull << 4 | 1ull << 2),
							(1ull << 0 | 1ull << 3)
						}
					);
				}
			}
		}
		else{
			if((spec_mem.cr & BLACK_OO) && (B_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED & occ) == 0){
				if((their_pseudo & (B_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							short(compress_piece<B_KING>()),
							short(compress_piece<B_ROOK>()),
							1 | 3 << 5,
							(1ull << 60 | 1ull << 62),
							(1ull << 63 | 1ull << 61)
						}
					);
				}
			}
			if((spec_mem.cr & BLACK_OOO) && (B_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED & occ) == 0){
				if((their_pseudo & (B_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							short(compress_piece<B_KING>()),
							short(compress_piece<B_ROOK>()),
							3 | 3 << 5,
							(1ull << 60 | 1ull << 58),
							(1ull << 56 | 1ull << 59)
						}
					);
				}
			}
		}


		//PROMOTIONS


		Piece ourpawn = make_piece(we, PAWN);
		Bitboard ourpawns = get(ourpawn);
		if((we == WHITE && (ourpawns & rank_bb(RANK_7))) || (we == BLACK && (ourpawns & rank_bb(RANK_2))))
		Bitloop(ourpawns){
			int reverse_bit = ((we == BLACK) ? 2 : 0);
			const Bitboard our_piece_single = _blsi_u64(ourpawns);
			const Square our_piece_square = lsb(ourpawns);
			Bitboard attacks = pawn_attacks<we>(our_piece_square, occ, their_pieces, spec_mem.ep);
			attacks &= (pinlines[our_piece_square]);
			attacks &= checkmask_for_normal_figures;
			attacks &= last_rank<we>();
			Bitloop(attacks){
				Bitboard singleattack = _blsi_u64(attacks);
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					1 << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : attacks
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					2 << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : attacks
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					3 << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : attacks
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					4 << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : attacks
				});
			}
		}



		return moves;
	}
	void apply_move(const turbomove& tm){
		piece_boards[tm.index1] ^= tm.bb1;
		if(tm.index2 < 12)
			piece_boards[tm.index2] ^= tm.bb2;
		for(Bitboard b : {tm.bb1, tm.bb2}){
			if(b == 1ull << SQ_A1){
				spec_mem.cr &= ~WHITE_OOO;
			}
			if(b & 1ull << SQ_H1){
				spec_mem.cr &= ~WHITE_OO;
			}
			if(b & 1ull << SQ_E1){
				spec_mem.cr &= ~WHITE_CASTLING;
			}
		}
		for(Bitboard b : {tm.bb1, tm.bb2}){
			if(b == 1ull << SQ_A8){
				spec_mem.cr &= ~BLACK_OOO;
			}
			if(b & 1ull << SQ_H8){
				spec_mem.cr &= ~BLACK_OO;
			}
			if(b & 1ull << SQ_E8){
				spec_mem.cr &= ~BLACK_CASTLING;
			}
		}
		if((tm.flags >> 5) & 3){
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
		}
		spec_mem.ep = SQ_NONE;
		if(tm.index1 == 0 || tm.index1 == 6){
			if(lsb(_blsr_u64(tm.bb1)) - lsb(tm.bb1) == 16){
				spec_mem.ep = Square((lsb(_blsr_u64(tm.bb1)) + lsb(tm.bb1)) / 2);
			}
		}
		at_move = ~at_move;
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
	}
};
#endif //POSITION_HPP_INCLUDED
