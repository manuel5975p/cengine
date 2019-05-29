#include <iostream>
#include "types.hpp"
#include "bitboard.hpp"
#include "position.hpp"
#include "io.hpp"
int main(){
	Bitboards::init();
	Position p;
	p.generate_trivial();
}
