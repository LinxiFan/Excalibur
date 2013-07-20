#ifndef __board_h__
#define __board_h__

#include "utils.h" 

namespace Board
{
	// Initialize all tables or constants. Called once at program start. 
	void init_tables();

	// Precalculated attack tables for non-sliding pieces
	extern Bit knightMask[SQ_N], kingMask[SQ_N];
	 // pawn has 3 kinds of moves: attack, push, and double push (push2)
	extern Bit pawnAttackMask[COLOR_N][SQ_N], pawnPushMask[COLOR_N][SQ_N], pawnPush2Mask[COLOR_N][SQ_N];
	// passed pawn table = inFrontMask[c][sq] & (fileMask[file] | fileAdjacentMask[sq])
	extern Bit passedPawnMask[COLOR_N][SQ_N];

	// Precalculated attack tables for sliding pieces. 
	extern byte rookKey[SQ_N][4096]; // Rook attack keys. any &mask-result is hashed to 2 ** 12
	extern Bit rookMask[4900];  // Rook attack table. Use attack_key to lookup. 4900: all unique possible masks
	extern byte bishopKey[SQ_N][512]; // Bishop attack keys. any &mask-result is hashed to 2 ** 9
	extern Bit bishopMask[1428]; // Bishop attack table. 1428: all unique possible masks
	extern Bit rookRayMask[SQ_N];
	extern Bit bishopRayMask[SQ_N];
	extern Bit queenRayMask[SQ_N];

	/* Castling constants and masks */
	// King-side
	const int CASTLE_FG = 0;  // file f to g should be vacant
	const int CASTLE_EG = 1; // file e to g shouldn't be attacked
	// Queen-side
	const int CASTLE_BD = 2;  // file b to d should be vacant
	const int CASTLE_CE = 3;  // file c to e shouldn't be attacked
	// the CASTLE_MASK is filled out in Board::init_tables()
	extern Bit CASTLE_MASK[COLOR_N][4];
	// location of the rook for castling: [COLOR_N][0=from, 1=to]. Used in make/unmakeMove
	const Square SQ_OO_ROOK[COLOR_N][2] = { {7, 5}, {63, 61} };
	const Square SQ_OOO_ROOK[COLOR_N][2] = { {0, 3}, {56, 59} };
	// Rook from-to
	extern Bit ROOK_OO_MASK[COLOR_N];
	extern Bit ROOK_OOO_MASK[COLOR_N];

	// Other tables
	extern Bit forwardMask[COLOR_N][SQ_N]; // represent all squares ahead of the square on its file
	extern Bit betweenMask[SQ_N][SQ_N];  // get the mask between two squares: if not aligned diag or orthogonal, return 0
	extern int squareDistanceTbl[SQ_N][SQ_N]; // max(fileDistance, rankDistance)
	extern Bit fileMask[FILE_N], rankMask[RANK_N], fileAdjacentMask[FILE_N]; // entire row or column
	extern Bit inFrontMask[COLOR_N][RANK_N]; // Everything in front of a rank, with respect to a color

	// for the magics parameters. Will be precalculated
	struct Magics
	{
		Bit mask;  // &-mask
		int offset;  // attack_key + offset == real attack lookup table index
	};
	extern Magics rookMagics[SQ_N];  // for each square
	extern Magics bishopMagics[SQ_N]; 

	// Generate the U64 magic multipliers. Won't actually be run. Pretabulated literals
	void rook_magicU64_generator();  // will display the results to stdout
	void bishop_magicU64_generator();
	// Constants for magics. Generated by the functions *_magicU64_generator()
	const U64 ROOK_MAGIC[64] = {
		0x380028420514000ULL, 0x234000b000e42840ULL, 0xad0017470a00100ULL, 0x4600020008642cfcULL, 0x60008031806080cULL, 0x1040008210100120ULL, 0x4000442010040b0ULL, 0x50002004c802d00ULL,
		0x2a102008221000c0ULL, 0x481a400106c061b0ULL, 0x6c8c44a50c241014ULL, 0x5941400801236140ULL, 0x581b780b00612110ULL, 0x5941400801236140ULL, 0x114063556026142ULL, 0x850010114171188ULL,
		0x2580005003040491ULL, 0x7a10298810201288ULL, 0x2000b000b24400ULL, 0x45920695400d1010ULL, 0xa2450805082a80ULL, 0x820011004040090ULL, 0x550001100a2208c8ULL, 0x40144a806a51100ULL,
		0x2201428430043280ULL, 0x41602072204d0104ULL, 0x4103002b0040200bULL, 0x80153680a700017ULL, 0x353030070064010ULL, 0x49204448024446ULL, 0x6190080660004049ULL, 0x832302200087104ULL,
		0x4800110202002ea0ULL, 0x5212702416700054ULL, 0x388220a6600f2000ULL, 0xa2450805082a80ULL, 0x170018e4c080008ULL, 0x1498498129d06a8eULL, 0x16024c0900882010ULL, 0xc63468322000234ULL,
		0x1782619040800820ULL, 0x4026302820444000ULL, 0x28040114502441a4ULL, 0xe00024540410404ULL, 0xc42c4910510009ULL, 0x16024c0900882010ULL, 0x921108038010640ULL, 0x3330014229810002ULL,
		0x4040220011004020ULL, 0x4000281208040050ULL, 0x353030070064010ULL, 0x6802010224302020ULL, 0x353030070064010ULL, 0x921108038010640ULL, 0x4088400250215141ULL, 0x644a090000441020ULL,
		0x244108d042800023ULL, 0x4089429910810122ULL, 0x66b014810412001ULL, 0x22a10ab022000c62ULL, 0x189020440861301aULL, 0x1021684084d1006ULL, 0x800410800914214ULL, 0x201454022080430aULL
	};
	const U64 BISHOP_MAGIC[64] = {
		0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x123000c0c08000cULL, 0x106418d208000380ULL, 0x40b1213e4c100c60ULL, 0x106418d208000380ULL, 0x1609004520240000ULL, 0x5100080909305040ULL,
		0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL, 0x106418d208000380ULL, 0x123000c0c08000cULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL,
		0x123000c0c08000cULL, 0x123000c0c08000cULL, 0x80553d511ac480dULL, 0x38e0481901881089ULL, 0x6832108254804040ULL, 0x5100080909305040ULL, 0x40b1213e4c100c60ULL, 0x40b1213e4c100c60ULL,
		0x1498498129d06a8eULL, 0x1498498129d06a8eULL, 0x2000206a42004820ULL, 0x1048180001620020ULL, 0x310901001a904000ULL, 0x2004014010080043ULL, 0x6500520414092850ULL, 0x6500520414092850ULL,
		0x6500520414092850ULL, 0x6500520414092850ULL, 0x904808008f0040ULL, 0x4886200800090114ULL, 0x101010400a60020ULL, 0x68c4011502f5032ULL, 0x45884444050101a1ULL, 0x45884444050101a1ULL,
		0x6500520414092850ULL, 0x11204e0610040028ULL, 0x6c04020161224b60ULL, 0x45884444050101a1ULL, 0x41174002032000c0ULL, 0x6aa0005c18024124ULL, 0x6500520414092850ULL, 0x11204e0610040028ULL,
		0x11204e0610040028ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL,
		0x904808008f0040ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x6500520414092850ULL, 0x45884444050101a1ULL, 0x11204e0610040028ULL, 0x6500520414092850ULL, 0x6500520414092850ULL
	};

	#define rhash(sq, rook) ((rook) * ROOK_MAGIC[sq])>>52  // get the hash value of a rook &-result, shift 64-12
	#define bhash(sq, bishop) ((bishop) * BISHOP_MAGIC[sq])>>55  // get the hash value of a bishop &-result, shift 64-9


	/* Functions that would actually be used to answer queries */
	inline Bit rook_attack(Square sq, Bit occup)
		{ return rookMask[ rookKey[sq][rhash(sq, occup & rookMagics[sq].mask)] + rookMagics[sq].offset ]; }
	inline Bit bishop_attack(Square sq, Bit occup)
		{ return bishopMask[ bishopKey[sq][bhash(sq, occup & bishopMagics[sq].mask)] + bishopMagics[sq].offset ]; }
	inline Bit queen_attack(Square sq, Bit occup) { return rook_attack(sq, occup) | bishop_attack(sq, occup); }
	inline Bit knight_attack(Square sq) { return knightMask[sq]; }
	inline Bit king_attack(Square sq) { return kingMask[sq]; }
	inline Bit pawn_attack(Color c, Square sq) { return pawnAttackMask[c][sq]; }
	inline Bit pawn_push(Color c, Square sq) { return pawnPushMask[c][sq]; }
	inline Bit pawn_push2(Color c, Square sq) { return pawnPush2Mask[c][sq]; }
	inline Bit passed_pawn_mask(Color c, Square sq) { return passedPawnMask[c][sq]; }
	inline Bit rook_ray(Square sq) { return rookRayMask[sq]; }
	inline Bit bishop_ray(Square sq) { return bishopRayMask[sq]; }
	inline Bit queen_ray(Square sq) { return queenRayMask[sq]; }

	inline Square forward_sq(Color c, Square sq) { return sq + (c == W ? 8: -8);  }
	inline Square backward_sq(Color c, Square sq) { return sq + (c == W ? -8 : 8);  }
	inline Bit between(Square sq1, Square sq2) { return betweenMask[sq1][sq2]; }
	inline bool is_aligned(Square sq1, Square sq2, Square sq3)  // are sq1, 2, 3 aligned?
	{		return (  ( between(sq1, sq2) | between(sq1, sq3) | between(sq2, sq3) )
				& ( setbit(sq1) | setbit(sq2) | setbit(sq3) )   ) != 0;  }
	inline int square_distance(Square sq1, Square sq2) { return squareDistanceTbl[sq1][sq2]; }
	inline Bit file_mask(int file) { return fileMask[file]; }
	inline Bit rank_mask(int rank) { return rankMask[rank]; }
	inline Bit file_adjacent_mask(Square sq) { return fileAdjacentMask[sq2file(sq)]; }
	inline Bit in_front_mask(Color c, Square sq) { return inFrontMask[c][sq2rank(sq)]; }
	inline Bit forward_mask(Color c, Square sq) { return forwardMask[c][sq]; }

	// with respect to the reference frame of Color
	inline Square relative_square(Color c, Square s) { return s ^ (c * 56); }
	// relative rank of a square
	template <int> inline int relative_rank(Color c, int sq_or_rank);
	template<> inline int relative_rank<SQ_N>(Color c, int sq) { return (sq >> 3) ^ (c * 7); }
	template<> inline int relative_rank<RANK_N>(Color c, int rank) { return rank ^ (c * 7); }
		// default to relative rank of a square
	inline int relative_rank(Color c, int sq) { return relative_rank<SQ_N>(c, sq); }

}  // namespace Board

#endif // __board_h__
