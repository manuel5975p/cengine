#include "position.hpp"
#include "io.hpp"
#include "bitboard.hpp"
#include <sstream>
zobrist_table global_zobrist_table;
std::string Position::to_string()const{
	std::string ret = bb_to_string(get(W_PAWN), W_PAWN);
	for(int i = 1;i < 12;i++){
		ret = ret | bb_to_string(piece_boards[i], pieces[i]);
	}
	return ret;
}
Position::Position(const std::string& fen) : Position(0){
	unsigned index = 0;
	for(char x : fen){
		if(x == ' ')break;
		if(x == '/')continue;
		if(x - '0' < 9){
			int xn = x - '0';
			index += xn;
			continue;
		}
		unsigned rank = 7 - index / 8;
		unsigned file = index % 8;
		get(charPiece(x)) |= (1ULL << (rank * 8 + file));
		index++;
	}
	size_t n = fen.find(' ');
	if(fen[n + 1] == 'w'){
		at_move = WHITE;
	}
	else{
		at_move = BLACK;
	}
	spec_mem.cr = NO_CASTLING;
	spec_mem.ep = SQ_NONE;
	size_t cr_begin = fen.find(' ', n + 1) + 1;
	size_t cr_end = fen.find(' ', cr_begin);
	std::string castling_string = fen.substr(cr_begin, cr_end);
	for(char x : castling_string){
		if(x == 'K'){
			spec_mem.cr |= CastlingRight::WHITE_OO;
		}
		if(x == 'Q'){
			spec_mem.cr |= CastlingRight::WHITE_OOO;
		}
		if(x == 'k'){
			spec_mem.cr |= CastlingRight::BLACK_OO;
		}
		if(x == 'q'){
			spec_mem.cr |= CastlingRight::BLACK_OOO;
		}
	}
	size_t ep_end = fen.find(' ', cr_end + 1);
	std::string ep_string = fen.substr(cr_end + 1, ep_end);
	if(ep_string[0] != '-'){
		spec_mem.ep = string_to_square(ep_string);
	}
	size_t fifty_end = fen.find(' ', ep_end + 1);
	spec_mem.since_capture = std::stoi(fen.substr(ep_end + 1, fifty_end));
	spec_mem.hash = this->hash();
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
stackvector<complete_move, 256> Position::generate_trivial(Color color) const{
	Bitboard occupied = this->occupied();
	Bitboard ours = get(color);
	Bitboard theirs = get(opp(color));
	Piece piece_map[64] = {NO_PIECE};
	for(auto& pi : pieces){
		Bitboard pib = get(pi);
		while(pib){
			Square x = lsb(pib);
			piece_map[x] = pi;
			pib ^= (1ULL << x);
		}
	}
	stackvector<complete_move, 256> moves;
	if(color == WHITE){
		Bitboard pboard = this->get(W_PAWN);
		while(pboard){
			Bitboard single_piece = pboard & -pboard;
			Bitboard attacks = pawn_attacks<WHITE>(lsb(single_piece), occupied, theirs, spec_mem.ep);
			while(attacks){
				Bitboard attack_bit = attacks & -attacks;
				moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), W_PAWN, piece_map[lsb(attack_bit)]));
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
					moves.push_back(complete_move(lsb(single_piece), lsb(attack_bit), p, piece_map[lsb(attack_bit)]));
					attacks ^= attack_bit;
				}
				pboard ^= single_piece;
			}
		}
		if((occupied & W_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED) == 0)
		if(spec_mem.cr & WHITE_OO){
			if(!under_attack_for(WHITE, SQ_E1) && !under_attack_for(WHITE, SQ_F1) && !under_attack_for(WHITE, SQ_G1)){
				moves.push_back(complete_move(SQ_E1, SQ_G1, W_KING, NO_PIECE));
			}
		}
		if((occupied & W_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED) == 0)
		if(spec_mem.cr & WHITE_OOO){
			if(!under_attack_for(WHITE, SQ_E1) && !under_attack_for(WHITE, SQ_D1) && !under_attack_for(WHITE, SQ_C1)){
				moves.push_back(complete_move(SQ_E1, SQ_C1, W_KING, NO_PIECE));
			}
		}
	}
	else if(color == BLACK){
		Bitboard pboard = this->get(B_PAWN);
		while(pboard){
			Bitboard single_piece = pboard & -pboard;
			Bitboard attacks = pawn_attacks<BLACK>(lsb(single_piece), occupied, theirs, spec_mem.ep);
			while(attacks){
				Bitboard attack_bit = attacks & -attacks;
				moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), B_PAWN, piece_map[lsb(attack_bit)]));
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
					moves.push_back(complete_move(lsb(single_piece),lsb(attack_bit), p, piece_map[lsb(attack_bit)]));
					attacks ^= attack_bit;
				}
				pboard ^= single_piece;
			}
		}
		if((occupied & B_KINGSIDE_CASTLING_EMPTYNESS_REQUIRED) == 0)
		if(spec_mem.cr & BLACK_OO){
			if(!under_attack_for(BLACK, SQ_E8) && !under_attack_for(BLACK, SQ_F8) && !under_attack_for(BLACK, SQ_G8)){
				moves.push_back(complete_move(SQ_E8, SQ_G8, B_KING, NO_PIECE));
			}
		}
		if((occupied & B_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED) == 0)
		if(spec_mem.cr & BLACK_OOO){
			if(!under_attack_for(BLACK, SQ_E8) && !under_attack_for(BLACK, SQ_D8) && !under_attack_for(BLACK, SQ_C8)){
				moves.push_back(complete_move(SQ_E8, SQ_C8, B_KING, NO_PIECE));
			}
		}
	}
	return moves;
}
void Position::apply_move(const complete_move& move){
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	if(move.captured_piece != NO_PIECE){
		this->get(move.captured_piece) &= ~(1ULL << move.to);
	}
	if(move.moving_piece == W_PAWN || move.moving_piece == B_PAWN){
		if(move.to == this->spec_mem.ep){
			if(move.moving_piece == W_PAWN){
				get(B_PAWN) &= ~(1ull << (int(this->spec_mem.ep) - 8));
			}
			else{
				get(W_PAWN) &= ~(1ull << (int(this->spec_mem.ep) + 8));
			}
		}
		if(std::abs(int(move.to) - int(move.from)) == 16){
			this->spec_mem.ep = Square((int(move.to) + int(move.from)) / 2);
		}
		else{
			this->spec_mem.ep = SQ_NONE;
		}
	}
	else{
		this->spec_mem.ep = SQ_NONE;
	}
	if(move.moving_piece == W_KING || move.moving_piece == B_KING){
		if(std::abs(int((move.to)) - int((move.from))) == 2){
			//std::cout << square_to_string(move.from) << " to ";
			//std::cout << square_to_string(move.to) << std::endl;
			switch(move.to){
				case SQ_G1:
					print(get(W_ROOK));
					get(W_ROOK) ^= square_bb(SQ_H1);
					get(W_ROOK) ^= square_bb(SQ_F1);
					print(get(W_ROOK));
				break;
				case SQ_C1:
					get(W_ROOK) ^= square_bb(SQ_A1);
					get(W_ROOK) ^= square_bb(SQ_D1);
				break;
				case SQ_G8:
					get(B_ROOK) ^= square_bb(SQ_H8);
					get(B_ROOK) ^= square_bb(SQ_F8);
				break;
				case SQ_C8:
					get(B_ROOK) ^= square_bb(SQ_A8);
					get(B_ROOK) ^= square_bb(SQ_D8);
				break;
				default: assert(false);
			}
		} 
	}
}
void Position::revert_move(const complete_move& move){
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	if(move.captured_piece != NO_PIECE){
		this->get(move.captured_piece) |= (1ULL << move.to);
	}
}
bool Position::revert_move_checked(const complete_move& move){
	if(color_of(move.moving_piece) == this->at_move){
		return false;
	}
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	if(move.captured_piece != NO_PIECE){
		this->get(move.captured_piece) |= (1ULL << move.to);
	}
	at_move = ~at_move;
	return true;
}
bool Position::apply_move_checked(const complete_move& move){
	//std::cout << move.moving_piece << "\n";
	//std::cout << at_move << "\n";
	if(!(color_of(move.moving_piece) == this->at_move))return false;
	Bitboard xor_mask = 0;
	xor_mask ^= (1ULL << move.from);
	xor_mask ^= (1ULL << move.to);
	this->get(move.moving_piece) ^= xor_mask;
	if(move.captured_piece != NO_PIECE){
		this->get(move.captured_piece) &= ~(1ULL << move.to);
	}
	
	if(move.moving_piece == W_PAWN || move.moving_piece == B_PAWN){
		if(move.to == this->spec_mem.ep){
			if(move.moving_piece == W_PAWN){
				get(B_PAWN) &= ~(1ull << (int(this->spec_mem.ep) - 8));
			}
			else{
				get(W_PAWN) &= ~(1ull << (int(this->spec_mem.ep) + 8));
			}
		}
		if(std::abs(int(move.to) - int(move.from)) == 16){
			this->spec_mem.ep = Square((int(move.to) + int(move.from)) / 2);
		}
		else{
			this->spec_mem.ep = SQ_NONE;
		}
	}
	else{
		this->spec_mem.ep = SQ_NONE;
	}
	if(move.moving_piece == W_KING || move.moving_piece == B_KING){
		if(std::abs(int((move.to)) - int((move.from))) == 2){
			//std::cout << square_to_string(move.from) << " to ";
			//std::cout << square_to_string(move.to) << std::endl;
			switch(move.to){
				case SQ_G1:
					print(get(W_ROOK));
					get(W_ROOK) ^= square_bb(SQ_H1);
					get(W_ROOK) ^= square_bb(SQ_F1);
					print(get(W_ROOK));
				break;
				case SQ_C1:
					get(W_ROOK) ^= square_bb(SQ_A1);
					get(W_ROOK) ^= square_bb(SQ_D1);
				break;
				case SQ_G8:
					get(B_ROOK) ^= square_bb(SQ_H8);
					get(B_ROOK) ^= square_bb(SQ_F8);
				break;
				case SQ_C8:
					get(B_ROOK) ^= square_bb(SQ_A8);
					get(B_ROOK) ^= square_bb(SQ_D8);
				break;
				default: assert(false);
			}
		}
		else{
			if(move.moving_piece == W_KING){
				spec_mem.cr &= ~(CastlingRight::WHITE_CASTLING);
			}
			if(move.moving_piece == B_KING){
				spec_mem.cr &= ~(CastlingRight::BLACK_CASTLING);
			}
			if(move.moving_piece == W_ROOK){
				if(move.from == SQ_A1)
					spec_mem.cr &= ~(CastlingRight::WHITE_OOO);
				if(move.from == SQ_H1)
					spec_mem.cr &= ~(CastlingRight::WHITE_OO);
			}
			if(move.moving_piece == B_ROOK){
				if(move.from == SQ_A8)
					spec_mem.cr &= ~(CastlingRight::BLACK_OOO);
				if(move.from == SQ_H8)
					spec_mem.cr &= ~(CastlingRight::BLACK_OO);
			}
		}
	}
	if(check(at_move)){
		if(move.captured_piece != NO_PIECE){
			this->get(move.captured_piece) |= (1ULL << move.to);
		}
		this->get(move.moving_piece) ^= xor_mask;return false;
	}
	at_move = ~at_move;
	return true;
}
bool Position::under_attack_for(Color c, Square s)const{
	Bitboard enemy_rooks;
	Bitboard enemy_bishops;
	Bitboard enemy_knights;
	Bitboard enemy_queens;
	Bitboard enemy_kings;
	Bitboard enemy_pawns;
	Bitboard target_bit = Bitboard(1) << s;
	if(c == BLACK){
		enemy_rooks   = get(W_ROOK);
		enemy_bishops = get(W_BISHOP);
		enemy_knights = get(W_KNIGHT);
		enemy_queens  = get(W_QUEEN);
		enemy_kings   = get(W_KING);
		enemy_pawns   = get(W_PAWN);
	}
	else{
		enemy_rooks   = get(B_ROOK);
		enemy_bishops = get(B_BISHOP);
		enemy_knights = get(B_KNIGHT);
		enemy_queens  = get(B_QUEEN);
		enemy_kings   = get(B_KING);
		enemy_pawns   = get(B_PAWN);
	}
	Bitboard occ = this->occupied();
	if(attacks_bb<ROOK>(s, occ) & (enemy_rooks | enemy_queens)){
		return true;
	}
	if(attacks_bb<BISHOP>(s, occ) & (enemy_bishops | enemy_queens)){
		return true;
	}
	if(attacks_bb(KING, s, occ) & (enemy_kings | enemy_queens)){
		return true;
	}
	if(attacks_bb(KNIGHT, s, occ) & enemy_knights){
		return true;
	}
	if(c == WHITE){
		if(shift<NORTH_WEST>(target_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<NORTH_EAST>(target_bit) & (enemy_pawns)){
			return true;
		}
	}
	else{
		if(shift<SOUTH_WEST>(target_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<SOUTH_EAST>(target_bit) & (enemy_pawns)){
			return true;
		}
	}
	return false;
}
bool Position::check(Color c)const{
	Bitboard king_bit;
	Bitboard enemy_rooks;
	Bitboard enemy_bishops;
	Bitboard enemy_knights;
	Bitboard enemy_queens;
	Bitboard enemy_kings;
	Bitboard enemy_pawns;

	if(c == BLACK){
		king_bit = get(B_KING);
		enemy_rooks   = get(W_ROOK);
		enemy_bishops = get(W_BISHOP);
		enemy_knights = get(W_KNIGHT);
		enemy_queens  = get(W_QUEEN);
		enemy_kings   = get(W_KING);
		enemy_pawns   = get(W_PAWN);
	}
	else{
		king_bit = get(W_KING);
		enemy_rooks   = get(B_ROOK);
		enemy_bishops = get(B_BISHOP);
		enemy_knights = get(B_KNIGHT);
		enemy_queens  = get(B_QUEEN);
		enemy_kings   = get(B_KING);
		enemy_pawns   = get(B_PAWN);
	}
	Bitboard occ = this->occupied();
	Square king_pos = lsb(king_bit);
	if(attacks_bb<ROOK>(king_pos, occ) & (enemy_rooks | enemy_queens)){
		return true;
	}
	if(attacks_bb<BISHOP>(king_pos, occ) & (enemy_bishops | enemy_queens)){
		return true;
	}
	if(attacks_bb(KING, king_pos, occ) & (enemy_kings | enemy_queens)){
		return true;
	}
	if(attacks_bb(KNIGHT, king_pos, occ) & enemy_knights){
		return true;
	}
	if(c == WHITE){
		if(shift<NORTH_WEST>(king_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<NORTH_EAST>(king_bit) & (enemy_pawns)){
			return true;
		}
	}
	else{
		if(shift<SOUTH_WEST>(king_bit) & (enemy_pawns)){
			return true;
		}
		if(shift<SOUTH_EAST>(king_bit) & (enemy_pawns)){
			return true;
		}
	}
	return false;
}
stackvector<complete_move, 256> Position::generate_legal(Color c)const{
	stackvector<complete_move, 256> ret = generate_trivial(c);
	
	for(auto it = ret.begin();it != ret.end();){
		Position p(*this);
		complete_move cmm = *it;
		p.apply_move(cmm);
		if(p.check(c)){
			std::swap(*it, *(ret.end() - 1));
			ret.pop_back();
		}
		else{
			it++;
		}
		//p.revert_move(cmm);
	}
	return ret;
}
std::string Position::fen()const{
	int empty = 0;
	std::stringstream sstr;
	for(int i = 63;i >= 0;i--){
		for(auto& p : pieces){
			if(get(p) & (1ull << ((i - i % 8) + (7 - i % 8)))){
				if(empty){
					sstr << empty;
				}
				empty = 0;
				sstr << pieceChar(p);
				goto found;
			}
		}
		empty++;
		found:
		(void)0;
		if(i % 8 == 0){
			if(empty)
			sstr << empty;
			if(i)
			sstr << '/';
			empty = 0;
		}
	}
	return sstr.str() + (at_move == WHITE ? " w " : " b ") + "KQkq - 0 1";
}
float* bit_to_float_ptr(float* dest, const Bitboard bits){
	size_t rank = 7;
	size_t file = 0;
	std::generate(dest, dest + 64, [&bits, &rank, &file]{
		float x = (!!(bits & (1ull << (rank * 8 + file)))) ? 1.0f : 0.0f;
		file++;
		if(file >= 8){
			file = 0;
			rank--;
		}
		return x;
	});
	return dest + 64;
}
Bitboard flipVertical(Bitboard x) {
    return  ( (x << 56)                           ) |
            ( (x << 40) & (0x00ff000000000000ULL) ) |
            ( (x << 24) & (0x0000ff0000000000ULL) ) |
            ( (x <<  8) & (0x000000ff00000000ULL) ) |
            ( (x >>  8) & (0x00000000ff000000ULL) ) |
            ( (x >> 24) & (0x0000000000ff0000ULL) ) |
            ( (x >> 40) & (0x000000000000ff00ULL) ) |
            ( (x >> 56) );
}

Eigen::VectorXf Position::to_one_hot_repr()const{
	Eigen::VectorXf ret(832);
	float* data = ret.data();
	
	if(at_move == WHITE){
		data = bit_to_float_ptr(data, ~this->occupied());
		data = bit_to_float_ptr(data, get(B_ROOK));
		data = bit_to_float_ptr(data, get(B_KNIGHT));
		data = bit_to_float_ptr(data, get(B_BISHOP));
		data = bit_to_float_ptr(data, get(B_QUEEN));
		data = bit_to_float_ptr(data, get(B_KING));
		data = bit_to_float_ptr(data, get(B_PAWN));

		data = bit_to_float_ptr(data, get(W_ROOK));
		data = bit_to_float_ptr(data, get(W_KNIGHT));
		data = bit_to_float_ptr(data, get(W_BISHOP));
		data = bit_to_float_ptr(data, get(W_QUEEN));
		data = bit_to_float_ptr(data, get(W_KING));
		data = bit_to_float_ptr(data, get(W_PAWN));
	}
	else{
		data = bit_to_float_ptr(data, flipVertical(~this->occupied()));
		data = bit_to_float_ptr(data, flipVertical(get(W_ROOK)));
		data = bit_to_float_ptr(data, flipVertical(get(W_KNIGHT)));
		data = bit_to_float_ptr(data, flipVertical(get(W_BISHOP)));
		data = bit_to_float_ptr(data, flipVertical(get(W_QUEEN)));
		data = bit_to_float_ptr(data, flipVertical(get(W_KING)));
		data = bit_to_float_ptr(data, flipVertical(get(W_PAWN)));
		data = bit_to_float_ptr(data, flipVertical(get(B_ROOK)));
		data = bit_to_float_ptr(data, flipVertical(get(B_KNIGHT)));
		data = bit_to_float_ptr(data, flipVertical(get(B_BISHOP)));
		data = bit_to_float_ptr(data, flipVertical(get(B_QUEEN)));
		data = bit_to_float_ptr(data, flipVertical(get(B_KING)));
		data = bit_to_float_ptr(data, flipVertical(get(B_PAWN)));		
	}
	if(data - ret.data() != 832){
		std::terminate();
	}
	return ret;
}