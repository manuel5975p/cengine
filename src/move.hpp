#ifndef MOVE_HPP
#define MOVE_HPP
#include "types.hpp"
#include "bitboard.hpp"
#include "io.hpp"
#include <string>
struct complete_move {
  Piece moving_piece;
  Square from;
  Square to;
  complete_move(Square _from, Square _to, Piece mp)
   : moving_piece(mp),  from(_from), to(_to) {
    assert(from != to && "Move goes to the same square");
  }
  complete_move(){}
  std::string to_string(){
      return std::string("Move: ") + pieceChar(moving_piece) + square_to_string(from) + square_to_string(to);
    }
};
#endif
