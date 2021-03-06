/* Zobrist keys and PieceSquareTable will be init by Utils::init() at program startup */
#ifndef __zobrist_h__
#define __zobrist_h__

#include "globals.h"

// Accessed as constants in movegen.
extern Score PieceSquareTable[COLOR_N][PIECE_TYPE_N][SQ_N];

namespace Zobrist
{
	extern U64 psq[COLOR_N][PIECE_TYPE_N][SQ_N];
	extern U64 ep[FILE_N];

	// 2nd index: [0] =0ull (preserve whatever xor'ed), 
	//			[1] kingside change, [2] queenside change
	//			[3] bothsides change = [1] ^ [2]
	extern U64 castle[COLOR_N][4];

	extern U64 turn;
	extern U64 exclusion;
}

#endif // __zobrist_h__
