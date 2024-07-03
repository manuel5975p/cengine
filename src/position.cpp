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
					get(W_ROOK) ^= square_bb(SQ_H1);
					get(W_ROOK) ^= square_bb(SQ_F1);
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
	spec_mem.hash ^= global_zobrist_table.values[compress_piece(move.moving_piece)][int(move.from)];
	spec_mem.hash ^= global_zobrist_table.values[compress_piece(move.moving_piece)][int(move.to)];
	if(move.captured_piece != NO_PIECE){
		this->get(move.captured_piece) &= ~(1ULL << move.to);
		spec_mem.hash ^= global_zobrist_table.values[compress_piece(move.captured_piece)][int(move.to)];
		spec_mem.since_capture = 0;
	}
	
	if(move.moving_piece == W_PAWN || move.moving_piece == B_PAWN){
		spec_mem.since_capture = 0;
		if(move.to == this->spec_mem.ep){
			
			if(move.moving_piece == W_PAWN){
				get(B_PAWN) &= ~(1ull << (int(this->spec_mem.ep) - 8));
				spec_mem.hash ^= global_zobrist_table.values[compress_piece(B_PAWN)][int(this->spec_mem.ep) - 8];
			}
			else{
				get(W_PAWN) &= ~(1ull << (int(this->spec_mem.ep) + 8));
				spec_mem.hash ^= global_zobrist_table.values[compress_piece(W_PAWN)][int(this->spec_mem.ep) + 8];
			}
		}
		else if(move.to <= SQ_H1 || move.to >= SQ_A8){
			Bitboard bb = Bitboard(1) << move.to;
			get(move.moving_piece) &= ~bb;
			spec_mem.hash ^= global_zobrist_table.values[compress_piece(move.moving_piece)][int(move.to)];
			spec_mem.hash ^= global_zobrist_table.values[compress_piece(move.promoted_to)][int(move.to)];
			get(move.promoted_to) |= bb;
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
					get(W_ROOK) ^= square_bb(SQ_H1);
					get(W_ROOK) ^= square_bb(SQ_F1);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OO)) ? global_zobrist_table.castling_values[0] : 0);
				    spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OOO)) ? global_zobrist_table.castling_values[1] : 0);
				    spec_mem.cr &= ~(CastlingRight::WHITE_CASTLING);
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(W_ROOK)][int(SQ_H1)];
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(W_ROOK)][int(SQ_F1)];
				break;
				case SQ_C1:
					get(W_ROOK) ^= square_bb(SQ_A1);
					get(W_ROOK) ^= square_bb(SQ_D1);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OO)) ? global_zobrist_table.castling_values[0] : 0);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OOO)) ? global_zobrist_table.castling_values[1] : 0);
					spec_mem.cr &= ~(CastlingRight::WHITE_CASTLING);
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(W_ROOK)][int(SQ_A1)];
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(W_ROOK)][int(SQ_D1)];
				break;
				case SQ_G8:
					get(B_ROOK) ^= square_bb(SQ_H8);
					get(B_ROOK) ^= square_bb(SQ_F8);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OOO)) ? global_zobrist_table.castling_values[3] : 0);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OO)) ? global_zobrist_table.castling_values[2] : 0);
					spec_mem.cr &= ~(CastlingRight::BLACK_CASTLING);
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(B_ROOK)][int(SQ_H8)];
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(B_ROOK)][int(SQ_F8)];
				break;
				case SQ_C8:
					get(B_ROOK) ^= square_bb(SQ_A8);
					get(B_ROOK) ^= square_bb(SQ_D8);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OOO)) ? global_zobrist_table.castling_values[3] : 0);
					spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OO)) ? global_zobrist_table.castling_values[2] : 0);
					spec_mem.cr &= ~(CastlingRight::BLACK_CASTLING);
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(B_ROOK)][int(SQ_A8)];
					spec_mem.hash ^= global_zobrist_table.values[compress_piece(B_ROOK)][int(SQ_D8)];
				break;
				default: assert(false);
			}
		}
		else{
			if(move.moving_piece == W_KING){
				spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OO)) ? global_zobrist_table.castling_values[0] : 0);
				spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OOO)) ? global_zobrist_table.castling_values[1] : 0);
				spec_mem.cr &= ~(CastlingRight::WHITE_CASTLING);
			}
			if(move.moving_piece == B_KING){
				spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OOO)) ? global_zobrist_table.castling_values[3] : 0);
				spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OO)) ? global_zobrist_table.castling_values[2] : 0);
				spec_mem.cr &= ~(CastlingRight::BLACK_CASTLING);
			}
		}
	}
	//if(move.moving_piece == W_ROOK){
		if(move.to == SQ_A1 || move.from == SQ_A1){
			spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OOO)) ? global_zobrist_table.castling_values[1] : 0);
			spec_mem.cr &= ~(CastlingRight::WHITE_OOO);
		}
		if(move.to == SQ_H1 || move.from == SQ_H1){
			spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::WHITE_OO)) ? global_zobrist_table.castling_values[0] : 0);
			spec_mem.cr &= ~(CastlingRight::WHITE_OO);
			
		}
	//}
	//if(move.moving_piece == B_ROOK){
		if(move.to == SQ_A8 || move.from == SQ_A8){
			spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OOO)) ? global_zobrist_table.castling_values[3] : 0);
			spec_mem.cr &= ~(CastlingRight::BLACK_OOO);
		}
		if(move.to == SQ_H8 || move.from == SQ_H8){
			spec_mem.hash ^= ((spec_mem.cr & (CastlingRight::BLACK_OO)) ? global_zobrist_table.castling_values[2] : 0);
			spec_mem.cr &= ~(CastlingRight::BLACK_OO);
		}
	//}
	if(check(at_move)){
		if(move.captured_piece != NO_PIECE){
			this->get(move.captured_piece) |= (1ULL << move.to);
		}
		this->get(move.moving_piece) ^= xor_mask;return false;
	}
	at_move = ~at_move;
	spec_mem.since_capture++;
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
std::vector<Position> Position::generate_all_successors()const{
	std::vector<Position> ret;
	auto leg = generate_legal(at_move);
	ret.reserve(leg.size());
	for(auto& move : leg){
		ret.push_back(*this);
		ret.back().apply_move_checked(move);
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
	std::string castlingstring;
	if(spec_mem.cr == NO_CASTLING){
		castlingstring = "-";
	}
	else{
		if(spec_mem.cr & WHITE_OO){
			castlingstring += "K";
		}
		if(spec_mem.cr & WHITE_OOO){
			castlingstring += "Q";
		}
		if(spec_mem.cr & BLACK_OO){
			castlingstring += "k";
		}
		if(spec_mem.cr & BLACK_OOO){
			castlingstring += "q";
		}
	}
	std::string epstring = spec_mem.ep == SQ_NONE ? "-" : square_to_string(spec_mem.ep);
	return sstr.str() + (at_move == WHITE ? " w " : " b ") + castlingstring + " " + epstring +  " 0 1";
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
template<Color we>
stackvector<turbomove, 256> Position::generate_loud()const{
		stackvector<turbomove, 256> moves;
		Bitboard pinlines[64];
		std::fill(pinlines, pinlines + 64, ~0ull);
		Bitboard checklines = 0;
		constexpr Color them = ~we;
		unsigned checkcount = 0;
		Bitboard our_king_bitboard = piece_boards[compress_piece(make_piece(we, KING))];
		Square our_king_square = lsb(piece_boards[compress_piece(make_piece(we, KING))]);
		Square their_king_square = lsb(piece_boards[compress_piece(make_piece(them, KING))]);
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
				if((attackline & (their_pieces & ~blsi_(es_bb))) == 0){
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
		Bitboard their_king_backward_pseudos[6] = {0};
		their_king_backward_pseudos[1] = pawn_attacks_bb<~we>(1ull << their_king_square);
		for(int x = 2; x < 6;x++){
			their_king_backward_pseudos[x] = attacks_bb(PieceType(x), their_king_square, occ);
		}
		checkcount += popcount(attacks_bb(KNIGHT, our_king_square, occ) & get(them, KNIGHT));
		checklines |= (attacks_bb(KNIGHT, our_king_square, occ) & get(them, KNIGHT));
		checkcount += popcount(pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN));
		checklines |= pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN);
		//print(checklines);
		//std::cout << checkcount << " checks\n";
		Bitboard checkmask_for_normal_figures;
		Bitboard loudness_mask = their_pieces;
		if(checkcount == 0)checkmask_for_normal_figures = ~0ull;
		else if(checkcount == 1){loudness_mask = ~(Bitboard(0));checkmask_for_normal_figures = checklines;}
		else if(checkcount == 2){loudness_mask = ~(Bitboard(0));checkmask_for_normal_figures = 0;}
		Bitboard checkmask_for_pawns = checkmask_for_normal_figures;
		
		//std::cout << our_king_square << "\n";
		if(pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN)){
			if(spec_mem.ep != SQ_NONE)
				checkmask_for_pawns |= square_bb(spec_mem.ep);
		}
		for(PieceType pt : piece_types){
			Piece ourpiece = make_piece(we, pt);
			Bitboard our_pieces_ofthis = get(ourpiece);
			if(checkcount >= 2 && pt != KING)continue;
			if(pt == PAWN){
				Bitloop(our_pieces_ofthis){
					const Bitboard our_piece_single = blsi_(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = pawn_attacks<we>(our_piece_square, occ, their_pieces, spec_mem.ep);
					attacks &= (pinlines[our_piece_square]);
					attacks &= (checkmask_for_pawns & (loudness_mask | their_king_backward_pseudos[pt]));
					attacks &= ~last_rank<we>();
					Bitloop(attacks){
						Bitboard singleattack = blsi_(attacks);
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
						moves.push_back(turbomove{(uint16_t)(compress_piece(ourpiece)), (uint16_t)(compress_piece(their_klonked_piece)), 0, our_piece_single | singleattack, their_klonked_piece == NO_PIECE ? 0 : their_piece_clearance});
						if(int(lsb(singleattack)) < int(our_piece_square)){
							moves.back().flags |= (1 << 1);
						}
					}
				}
			}
			else if(pt == BISHOP || pt == ROOK || pt == QUEEN || pt == KNIGHT){
				Bitloop(our_pieces_ofthis){
					const Bitboard our_piece_single = blsi_(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = attacks_bb(pt, our_piece_square, occ);
					attacks &= ~our_pieces;
					attacks &= (pinlines[our_piece_square]);
					attacks &= (checkmask_for_normal_figures & (loudness_mask | their_king_backward_pseudos[pt]));
					if(pt == ROOK && our_piece_square == SQ_H1){
						//print(attacks);
					}
					Bitloop(attacks){
						Bitboard singleattack = blsi_(attacks);
						unsigned int castling_klonk = 0;
						if constexpr(we == WHITE){
							castling_klonk |= (Bitboard(our_piece_square == SQ_H1 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A1 ? 1 : 0) << 6);
						}
						else{
							castling_klonk |= (Bitboard(our_piece_square == SQ_H8 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A8 ? 1 : 0) << 6);
						}
						moves.push_back(turbomove{uint16_t(compress_piece(ourpiece)), uint16_t(compress_piece(piece_map[lsb(attacks)])), castling_klonk, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
						if(int(lsb(singleattack)) < int(our_piece_square)){
							moves.back().flags |= (1 << 1);
						}
					}
				}
			}
			else /*if(checkcount)*/{
				Bitboard attacks = PseudoAttacks[KING][our_king_square];
				const Bitboard our_piece_single = 1ull << our_king_square;
				attacks &= ~our_pieces;
				attacks &= ~their_pseudo;
				attacks &= loudness_mask;
				Bitloop(attacks){
					Bitboard singleattack = blsi_(attacks);
					moves.push_back(turbomove{uint16_t(compress_piece(ourpiece)), uint16_t(compress_piece(piece_map[lsb(attacks)])), 0, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
					if(int(lsb(singleattack)) < int(our_king_square)){
						moves.back().flags |= (1 << 1);
					}
				}
			}
		}
		if(checkcount >= 2) return moves;
		return moves;
	}
	
	
	template<Color we>
	stackvector<turbomove, 256> Position::generate_new()const{
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
		generate_piecemap(piece_map);
		/*for(auto& pi : pieces){
			Bitboard pib = get(pi);
			while(pib){
				Square x = lsb(pib);
				piece_map[x] = pi;
				pib = _blsr_u64(pib);
			}
		}*/
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
				if((attackline & (their_pieces & ~blsi_(es_bb))) == 0){
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
		Bitboard checkmask_for_pawns = checkmask_for_normal_figures;
		//std::cout << our_king_square << "\n";
		if(pawn_attacks_bb<we>(1ull << our_king_square) & get(them, PAWN)){
			if(spec_mem.ep != SQ_NONE)
				checkmask_for_pawns |= square_bb(spec_mem.ep);
		}
		for(PieceType pt : piece_types){
			Piece ourpiece = make_piece(we, pt);
			Bitboard our_pieces_ofthis = get(ourpiece);
			if(checkcount >= 2 && pt != KING)continue;
			if(pt == PAWN){
				Bitloop(our_pieces_ofthis){
					const Bitboard our_piece_single = blsi_(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = pawn_attacks<we>(our_piece_square, occ, their_pieces, spec_mem.ep);
					attacks &= (pinlines[our_piece_square]);
					attacks &= checkmask_for_pawns;
					attacks &= ~last_rank<we>();
					Bitloop(attacks){
						Bitboard singleattack = blsi_(attacks);
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
					const Bitboard our_piece_single = blsi_(our_pieces_ofthis);
					const Square our_piece_square = lsb(our_pieces_ofthis);
					Bitboard attacks = attacks_bb(pt, our_piece_square, occ);
					attacks &= ~our_pieces;
					attacks &= (pinlines[our_piece_square]);
					
					attacks &= checkmask_for_normal_figures;
					if(pt == ROOK && our_piece_square == SQ_H1){
						//print(attacks);
					}
					Bitloop(attacks){
						Bitboard singleattack = blsi_(attacks);
						unsigned int castling_klonk = 0;
						if constexpr(we == WHITE){
							castling_klonk |= (Bitboard(our_piece_square == SQ_H1 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A1 ? 1 : 0) << 6);
						}
						else{
							castling_klonk |= (Bitboard(our_piece_square == SQ_H8 ? 1 : 0) << 5);
							castling_klonk |= (Bitboard(our_piece_square == SQ_A8 ? 1 : 0) << 6);
						}
						moves.push_back(turbomove{uint16_t(compress_piece(ourpiece)), uint16_t(compress_piece(piece_map[lsb(attacks)])), castling_klonk, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
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
					Bitboard singleattack = blsi_(attacks);
					moves.push_back(turbomove{uint16_t(compress_piece(ourpiece)), uint16_t(compress_piece(piece_map[lsb(attacks)])), 0, our_piece_single | singleattack, piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack});
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
				if((their_pseudo & (W_QUEENSIDE_CASTLING_NOCHECKS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							uint16_t(compress_piece<W_KING>()),
							uint16_t(compress_piece<W_ROOK>()),
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
							uint16_t(compress_piece<B_KING>()),
							uint16_t(compress_piece<B_ROOK>()),
							1 | 3 << 5,
							(1ull << 60 | 1ull << 62),
							(1ull << 63 | 1ull << 61)
						}
					);
				}
			}
			if((spec_mem.cr & BLACK_OOO) && (B_QUEENSIDE_CASTLING_EMPTYNESS_REQUIRED & occ) == 0){
				if((their_pseudo & (B_QUEENSIDE_CASTLING_NOCHECKS_REQUIRED | our_king_bitboard)) == 0){
					moves.push_back(
						turbomove{
							uint16_t(compress_piece<B_KING>()),
							uint16_t(compress_piece<B_ROOK>()),
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
			const Bitboard our_piece_single = blsi_(ourpawns);
			const Square our_piece_square = lsb(ourpawns);
			Bitboard attacks = pawn_attacks<we>(our_piece_square, occ, their_pieces, spec_mem.ep);
			attacks &= (pinlines[our_piece_square]);
			attacks &= checkmask_for_normal_figures;
			attacks &= last_rank<we>();
			Bitloop(attacks){
				Bitboard singleattack = blsi_(attacks);
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					1u << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					2u << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					3u << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack
				});
				moves.push_back(
				turbomove{
					compress_piece(ourpawn),
					compress_piece(piece_map[lsb(attacks)]),
					4u << 2 | reverse_bit,
					our_piece_single | singleattack,
					piece_map[lsb(attacks)] == NO_PIECE ? 0 : singleattack
				});
			}
		}
		return moves;
	}
/*Eigen::VectorXf Position::to_one_hot_repr()const{
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
}*/
template stackvector<turbomove, 256> Position::generate_loud<WHITE>()const;
template stackvector<turbomove, 256> Position::generate_loud<BLACK>()const;
template stackvector<turbomove, 256> Position::generate_new<WHITE>()const;
template stackvector<turbomove, 256> Position::generate_new<BLACK>()const;