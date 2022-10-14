#ifndef MOVE_HPP
#define MOVE_HPP
#include "types.hpp"
#include "bitboard.hpp"
#include "io.hpp"
#include <string>
struct Position;
struct complete_move {
    Piece moving_piece;
    Piece captured_piece;
    Square from;
    Square to;
    Piece promoted_to;
    complete_move(Square _from, Square _to, Piece mp, Piece cp) : moving_piece(mp), captured_piece(cp),  from(_from), to(_to) {
        //assert(from != to && "Move goes to the same square");
    }
    complete_move(){}
    complete_move(const std::array<Bitboard, 12>& pos, Square from, Square to){
        Bitboard fromm = (1ULL << from);
        Bitboard tom = (1ULL << to);
        //std::cout << square_to_string(from) << " <\n";
        moving_piece = NO_PIECE;
        for(auto& p : pieces){
            if(pos[compress_piece(p)] & fromm){
                moving_piece = p;
                break;
            }
        }
        this->from = from;
        this->to = to;
        captured_piece = NO_PIECE;
        for(auto& p : pieces){
            if(pos[compress_piece(p)] & tom){
                captured_piece = p;
                break;
            }
        }
    }
    complete_move(const std::array<Bitboard, 12>& pos, const std::string& movestr){
        assert(movestr.size() == 4 || movestr.size() == 5);
        Square from = string_to_square(movestr.substr(0,2));
        Square to = string_to_square(movestr.substr(2,4));
        Bitboard fromm = (1ULL << from);
        Bitboard tom = (1ULL << to);
        //std::cout << square_to_string(from) << " <\n";
        moving_piece = NO_PIECE;
        for(auto& p : pieces){
            if(pos[compress_piece(p)] & fromm){
                moving_piece = p;
                break;
            }
        }
        this->from = from;
        this->to = to;
        captured_piece = NO_PIECE;
        for(auto& p : pieces){
            if(pos[compress_piece(p)] & tom){
                captured_piece = p;
                break;
            }
        }
        if(movestr.size() == 5){
            Piece wp = charPiece(movestr.back());
            PieceType pt = get_type(wp);
            promoted_to = make_piece(get_color(moving_piece), pt);
        }
    }
    bool operator==(const complete_move& o)const{
        return (moving_piece == o.moving_piece) &&
               (captured_piece == o.captured_piece) &&
               (from == o.from) &&
               (to == o.to);
    }
    bool operator!=(const complete_move& o)const{
        return !operator==(o);
    }
    std::string to_string()const{
        return std::string("Move: ") + pieceChar(moving_piece) + square_to_string(from) + square_to_string(to) + ((captured_piece) ? (std::string(" captures ") + pieceChar(captured_piece)) : (""));
    }
    std::string to_short_notation()const{
        return square_to_string(from) + square_to_string(to);
    }
};
struct turbomove{
    unsigned short index1, index2;
    unsigned int flags;
    Bitboard bb1, bb2;
    /*turbomove(unsigned short i1, unsigned short i2, unsigned int f, Bitboard b1, Bitboard b2)
    : index1(i1),
    index2(i2),
    flags(f),
    bb1(b1),
    bb2(b2)
    {
        if()
    }*/
    std::string to_short_notation()const{
        std::string str;
        //if(flags & 1){
        //    str += "Castling: ";
        //}
        
        Bitboard moving = bb1;
        for(;moving; moving = (moving & (moving - 1))){
            Square sq = lsb(moving);
            str += square_to_string(sq);
        }
        if((flags >> 1) & 1){
            std::swap(str[0], str[2]);
            std::swap(str[1], str[3]);
        }
        if(7 & (flags >> 2)){
            switch(7 & (flags >> 2)){
                case 1:str += 'q';break;
                case 2:str += 'r';break;
                case 3:str += 'b';break;
                case 4:str += 'n';break;
            }
        }
        return str;
    }
};
#endif
