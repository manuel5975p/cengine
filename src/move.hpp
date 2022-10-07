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
        assert(movestr.size() == 4);
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
#endif
