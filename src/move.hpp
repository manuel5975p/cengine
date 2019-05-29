#ifndef MOVE_HPP
#define MOVE_HPP
#include "types.hpp"
#include "bitboard.hpp"
#include "io.hpp"
#include <string>
struct complete_move {
  Piece moving_piece;
  Bitboard xor_mask;
  complete_move(unsigned int from, unsigned int to, Piece mp)
   : moving_piece(mp),  xor_mask((1ULL << from) | (1ULL << to)) {
    assert(from != to && "Move goes to the same square");
  }
  complete_move(Bitboard xm, Piece mp)
   : moving_piece(mp), xor_mask(xm) {
    assert(popcount(xm) == 2 && "Move goes to the same square");
  }
  complete_move(){}
  std::string to_string(){
      return std::string("Move: ") + pieceChar(moving_piece) + square_to_string(lsb(xor_mask)) + square_to_string(msb(xor_mask));
    }
};
#endif