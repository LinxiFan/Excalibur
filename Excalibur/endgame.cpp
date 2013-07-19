#include "endgame.h"
using namespace Board;

/* A few predetermined tables */
// Table used to drive the defending king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
const int MateTable[SQ_N] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70,  90,
	80, 60, 40, 30, 30, 40, 60,  80,
	70, 50, 30, 20, 20, 30, 50,  70,
	70, 50, 30, 20, 20, 30, 50,  70,
	80, 60, 40, 30, 30, 40, 60,  80,
	90, 70, 60, 50, 50, 60, 70,  90,
	100, 90, 80, 70, 70, 80, 90, 100,
};

// Table used to drive the defending king towards a corner square of the
// right color in KBN vs K endgames.
const int KBNKMateTable[SQ_N] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

// The attacking side is given a descending bonus based on distance between
// the two kings in basic endgames.
const int DistanceBonus[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };

// Get the material key of a Position out of the given endgame key code
// like "KBPKN". The trick here is to first forge an ad-hoc fen string
// and then let a Position object to do the work for us. Note that the
// fen string could correspond to an illegal position.
U64 code2key(const string& code, Color c)
{
	string sides[] = { code.substr(code.find('K', 1)),  // Weaker
		code.substr(0, code.find('K', 1)) }; // Stronger

	std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

	string fen =  sides[0] + char('0' + int(8 - code.length()))
		+ sides[1] + "/8/8/8/8/8/8/8 w - - 0 10";

	return Position(fen).material_key();
}


// Endgame namespace definition
namespace Endgame
{
	// key and evaluator function collection.
	// Initialized at program startup. 
	Map evalFuncMap;
	Map scalingFuncMap;

	void init()
	{
		add_eval_func<KBNK>("KBNK");
		add_eval_func<KPK>("KPK");
		add_eval_func<KRKP>("KRKP");
		add_eval_func<KRKB>("KRKB");
		add_eval_func<KRKN>("KRKN");
		add_eval_func<KQKP>("KQKP");
		add_eval_func<KQKR>("KQKR");
		add_eval_func<KBBKN>("KBBKN");
		add_eval_func<KNNK>("KNNK");

		add_scaling_func<KRPKR>("KRPKR");
		add_scaling_func<KRPPKRP>("KRPPKRP");
		add_scaling_func<KBPKB>("KBPKB");
		add_scaling_func<KBPPKB>("KBPPKB");
		add_scaling_func<KBPKN>("KBPKN");
		add_scaling_func<KNPK>("KNPK");
		add_scaling_func<KNPKB>("KNPKB");
	}

	template<EndgameType E>
	void add_eval_func(const string& code)
	{
		for (Color c : COLORS)
		evalFuncMap[code2key(code, c)] = std::unique_ptr<EndEvaluatorBase>(new EndEvaluator<E>(c));
	}
	template<EndgameType E>
	void add_scaling_func(const string& code)
	{
		for (Color c : COLORS)
		scalingFuncMap[code2key(code, c)] = std::unique_ptr<EndEvaluatorBase>(new EndEvaluator<E>(c));
	}
}


/* 
* Various endgame evaluators 
* First: evalFunc() series
*/

/// Mate with KX vs K. This function is used to evaluate positions with
/// King and plenty of material vs a lone king. It simply gives the
/// attacking side a bonus for driving the defending king towards the edge
/// of the board, and for keeping the distance between the two kings small.
template<>
Value EndEvaluator<KXK>::operator()(const Position& pos) const 
{
	// Stalemate detection with lone king
	if ( pos.turn == weakerSide && !pos.checkermap()
		&& pos.count_legal() == 0)
			return VALUE_DRAW;

	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);

	Value result =   pos.non_pawn_material(strongerSide)
		+ pos.pieceCount[strongerSide][PAWN] * PIECE_VALUE[EG][PAWN] 
		+ MateTable[loserKSq]
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	// the last condition checks if the stronger side has a bishop pair
	if (   pos.pieceCount[strongerSide][QUEEN]
	|| pos.pieceCount[strongerSide][ROOK] 
	|| (pos.pieceCount[strongerSide][BISHOP] >= 2 
		&& opp_color_sq(pos.pieceList[strongerSide][BISHOP][0], pos.pieceList[strongerSide][BISHOP][1])) ) 
		result += VALUE_KNOWN_WIN;

	return strongerSide == pos.turn ? result : -result;
}

/// Mate with KBN vs K. This is similar to KX vs K, but we have to drive the
/// defending king towards a corner square of the right color.
template<>
Value EndEvaluator<KBNK>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);
	uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];

	// kbnk_mate_table() tries to drive toward corners A1 or H8,
	// if we have a bishop that cannot reach the above squares we
	// mirror the kings so to drive enemy toward corners A8 or H1.
	if (opp_color_sq(bishopSq, 0))  // square A1 == 0
	{
		// horizontal flip A1 to H1
		flip_hori(winnerKSq);
		flip_hori(loserKSq);
	}

	Value result =  VALUE_KNOWN_WIN
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)]
	+ KBNKMateTable[loserKSq];

	return strongerSide == pos.turn ? result : -result;
}

/// KP vs K. This endgame is evaluated with the help of KPK endgame bitbase.
template<>
Value EndEvaluator<KPK>::operator()(const Position& pos) const 
{
	uint wksq, bksq, wpsq;
	Color us;

	if (strongerSide == W)
	{
		wksq = pos.king_sq(W);
		bksq = pos.king_sq(B);
		wpsq = pos.pieceList[W][PAWN][0];
		us = pos.turn;
	}
	else
	{
		wksq = flip_vert(pos.king_sq(B));
		bksq = flip_vert(pos.king_sq(W));
		wpsq = flip_vert(pos.pieceList[B][PAWN][0]);
		us = ~pos.turn;
	}

	if (sq2file(wpsq) >= FILE_E)
	{
		flip_hori(wksq);
		flip_hori(bksq);
		flip_hori(wpsq);
	}

	if (!KPKbase::probe(wksq, wpsq, bksq, us))
		return VALUE_DRAW;

	Value result = VALUE_KNOWN_WIN + 
		PIECE_VALUE[EG][PAWN] + sq2rank(wpsq);

	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KP. This is a somewhat tricky endgame to evaluate precisely without
/// a bitbase. The function below returns drawish scores when the pawn is
/// far advanced with support of the king, while the attacking king is far
/// away.
template<>
Value EndEvaluator<KRKP>::operator()(const Position& pos) const 
{
	uint wksq, wrsq, bksq, bpsq;
	int tempo = (pos.turn == strongerSide);

	wksq = pos.king_sq(strongerSide);
	wrsq = pos.pieceList[strongerSide][ROOK][0];
	bksq = pos.king_sq(weakerSide);
	bpsq = pos.pieceList[weakerSide][PAWN][0];

	if (strongerSide == B)
	{
		wksq = flip_vert(wksq);
		wrsq = flip_vert(wrsq);
		bksq = flip_vert(bksq);
		bpsq = flip_vert(bpsq);
	}

	uint queeningSq = fr2sq(sq2file(bpsq), RANK_1);
	Value result;

	// If the stronger side's king is in front of the pawn, it's a win
	if (wksq < bpsq && sq2file(wksq) == sq2file(bpsq))
		result = PIECE_VALUE[EG][ROOK] - square_distance(wksq, bpsq);

	// If the weaker side's king is too far from the pawn and the rook,
	// it's a win
	else if (  square_distance(bksq, bpsq) - (tempo ^ 1) >= 3
		&& square_distance(bksq, wrsq) >= 3)
		result = PIECE_VALUE[EG][ROOK] - square_distance(wksq, bpsq);

	// If the pawn is far advanced and supported by the defending king,
	// the position is drawish
	else if (   sq2rank(bksq) <= RANK_3
		&& square_distance(bksq, bpsq) == 1
		&& sq2rank(wksq) >= RANK_4
		&& square_distance(wksq, bpsq) - tempo > 2)
		result = 80 - square_distance(wksq, bpsq) * 8;

	else
		result =  200
		- square_distance(wksq, bpsq - 8) * 8
		+ square_distance(bksq, bpsq - 8) * 8
		+ square_distance(bpsq, queeningSq) * 8;

	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KB. This is very simple, and always returns drawish scores.  The
/// score is slightly bigger when the defending king is close to the edge.
template<>
Value EndEvaluator<KRKB>::operator()(const Position& pos) const 
{
	Value result = MateTable[pos.king_sq(weakerSide)];
	return strongerSide == pos.turn ? result : -result;
}

/// KR vs KN.  The attacking side has slightly better winning chances than
/// in KR vs KB, particularly if the king and the knight are far apart.
template<>
Value EndEvaluator<KRKN>::operator()(const Position& pos) const 
{
	const int penalty[8] = { 0, 10, 14, 20, 30, 42, 58, 80 };

	uint bksq = pos.king_sq(weakerSide);
	uint bnsq = pos.pieceList[weakerSide][KNIGHT][0];
	Value result = MateTable[bksq] + penalty[square_distance(bksq, bnsq)];
	return strongerSide == pos.turn ? result : -result;
}

/// KQ vs KP.  In general, a win for the stronger side, however, there are a few
/// important exceptions.  Pawn on 7th rank, A,C,F or H file, with king next can
/// be a draw, so we scale down to distance between kings only.
template<>
Value EndEvaluator<KQKP>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);
	uint pawnSq = pos.pieceList[weakerSide][PAWN][0];

	Value result =  PIECE_VALUE[EG][QUEEN]
		- PIECE_VALUE[EG][PAWN]
		+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	if (   square_distance(loserKSq, pawnSq) == 1
		&& relative_rank(weakerSide, pawnSq) == RANK_7)
	{
		int f = sq2file(pawnSq);
		if (f == FILE_A || f == FILE_C || f == FILE_F || f == FILE_H)
			result = DistanceBonus[square_distance(winnerKSq, loserKSq)];
	}
	return strongerSide == pos.turn ? result : -result;
}

/// KQ vs KR.  This is almost identical to KX vs K:  We give the attacking
/// king a bonus for having the kings close together, and for forcing the
/// defending king towards the edge.  If we also take care to avoid null move
/// for the defending side in the search, this is usually sufficient to be
/// able to win KQ vs KR.
template<>
Value EndEvaluator<KQKR>::operator()(const Position& pos) const 
{
	uint winnerKSq = pos.king_sq(strongerSide);
	uint loserKSq = pos.king_sq(weakerSide);

	Value result =  PIECE_VALUE[EG][QUEEN]
		- PIECE_VALUE[EG][ROOK]
		+ MateTable[loserKSq]
	+ DistanceBonus[square_distance(winnerKSq, loserKSq)];

	return strongerSide == pos.turn ? result : -result;
}

/// KBB vs KN. Bishop pair vs lone knight.
template<>
Value EndEvaluator<KBBKN>::operator()(const Position& pos) const 
{
	Value result = PIECE_VALUE[EG][BISHOP];
	uint wksq = pos.king_sq(strongerSide);
	uint bksq = pos.king_sq(weakerSide);
	uint nsq = pos.pieceList[weakerSide][KNIGHT][0];

	// Bonus for attacking king close to defending king
	result += DistanceBonus[square_distance(wksq, bksq)];

	// Bonus for driving the defending king and knight apart
	result += square_distance(bksq, nsq) * 32;

	// Bonus for restricting the knight's mobility
	result += (8 - bit_count(Board::knight_attack(nsq))) * 8;

	return strongerSide == pos.turn ? result : -result;
}

/// K and two minors vs K and one or two minors or K and two knights against
/// king alone are always draw.
template<>
Value EndEvaluator<KmmKm>::operator()(const Position&) const { return VALUE_DRAW; }

template<>
Value EndEvaluator<KNNK>::operator()(const Position&) const { return VALUE_DRAW; }


/* 
* Second part: scalingFunc() series
*/

/// K, bishop and one or more pawns vs K. It checks for draws with rook pawns and
/// a bishop of the wrong color. If such a draw is detected, SCALE_FACTOR_DRAW
/// is returned. If not, the return value is SCALE_FACTOR_NONE, i.e. no scaling
/// will be used.
template<>
ScaleFactor EndEvaluator<KBPsK>::operator()(const Position& pos) const 
{
	Bit pawns = pos.Pawnmap[strongerSide];
	int pawnFile = sq2file( pos.pieceList[strongerSide][PAWN][0] );

	// All pawns are on a single rook file A or H?
	if (    (pawnFile == FILE_A || pawnFile == FILE_H)
		&& !(pawns & ~file_mask(pawnFile)))
	{
		uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];
		uint queeningSq = relative_square(strongerSide, fr2sq(pawnFile, RANK_8));
		uint kingSq = pos.king_sq(weakerSide);

		if (   opp_color_sq(queeningSq, bishopSq)
			&& abs(sq2file(kingSq) - pawnFile) <= 1)
		{
			// The bishop has the wrong color, and the defending king is on the
			// file of the pawn(s) or the adjacent file. Find the rank of the
			// frontmost pawn.
			int rank;
			if (strongerSide == W)
				for (rank = RANK_7; !(rank_mask(rank) & pawns); rank--) {}
			else
			{
				for (rank = RANK_2; !(rank_mask(rank) & pawns); rank++) {}
				rank = relative_rank(B, rank*8);
			}
			// If the defending king has distance 1 to the promotion square or
			// is placed somewhere in front of the pawn, it's a draw.
			if (   square_distance(kingSq, queeningSq) <= 1
				|| relative_rank(strongerSide, kingSq) >= rank)
				return SCALE_FACTOR_DRAW;
		}
	}

	Bit weakPawns;
	// All pawns on same B or G file? Then potential draw
	if (    (pawnFile == FILE_B || pawnFile == FILE_G)
		&& !((pawns | (weakPawns=pos.Pawnmap[weakerSide]) ) & ~file_mask(pawnFile))
		&& pos.non_pawn_material(weakerSide) == 0
		&& pos.pieceCount[weakerSide][PAWN] >= 1)
	{
		// Get weaker pawn closest to opponent's queening square
		uint weakerPawnSq = strongerSide == W ? msb(weakPawns) : lsb(weakPawns);

		uint strongerKingSq = pos.king_sq(strongerSide);
		uint weakerKingSq = pos.king_sq(weakerSide);
		uint bishopSq = pos.pieceList[strongerSide][BISHOP][0];

		// Draw if weaker pawn is on rank 7, bishop can't attack the pawn, and
		// weaker king can stop opposing opponent's king from penetrating.
		if (   relative_rank(strongerSide, weakerPawnSq) == RANK_7
			&& opp_color_sq(bishopSq, weakerPawnSq)
			&& square_distance(weakerPawnSq, weakerKingSq) <= square_distance(weakerPawnSq, strongerKingSq))
			return SCALE_FACTOR_DRAW;
	}

	return SCALE_FACTOR_NONE;
}


/// K and queen vs K, rook and one or more pawns. It tests for fortress draws with
/// a rook on the third rank defended by a pawn.
template<>
ScaleFactor EndEvaluator<KQKRPs>::operator()(const Position& pos) const 
{
	uint kingSq = pos.king_sq(weakerSide);
	if (   relative_rank(weakerSide, kingSq) <= RANK_2
		&& relative_rank(weakerSide, pos.king_sq(strongerSide)) >= RANK_4
		&& (pos.Rookmap[weakerSide] & rank_mask(relative_rank(weakerSide, RANK_3 *8)))
		&& (pos.Pawnmap[weakerSide] & rank_mask(relative_rank(weakerSide, RANK_2 *8)))
		&& (king_attack(kingSq) & pos.Pawnmap[weakerSide]))
	{
		uint rsq = pos.pieceList[weakerSide][ROOK][0];
		if (pawn_attack(strongerSide, rsq) & pos.Pawnmap[weakerSide])
			return SCALE_FACTOR_DRAW;
	}
	return SCALE_FACTOR_NONE;
}

/// K, rook and one pawn vs K and a rook. This function knows a handful of the
/// most important classes of drawn positions, but is far from perfect. It would
/// probably be a good idea to add more knowledge in the future.
template<>
ScaleFactor EndEvaluator<KRPKR>::operator()(const Position& pos) const {

	uint wksq = pos.king_sq(strongerSide);
	uint wrsq = pos.pieceList[strongerSide][ROOK][0];
	uint wpsq = pos.pieceList[strongerSide][PAWN][0];
	uint bksq = pos.king_sq(weakerSide);
	uint brsq = pos.pieceList[weakerSide][ROOK][0];

	// Orient the board in such a way that the stronger side is white, and the
	// pawn is on the left half of the board.
	if (strongerSide == B)
	{
		wksq = flip_vert(wksq);
		wrsq = flip_vert(wrsq);
		wpsq = flip_vert(wpsq);
		bksq = flip_vert(bksq);
		brsq = flip_vert(brsq);
	}
	if (sq2file(wpsq) > FILE_D)
	{
		flip_hori(wksq);
		flip_hori(wrsq);
		flip_hori(wpsq);
		flip_hori(bksq);
		flip_hori(brsq);
	}

	int f = sq2file(wpsq);
	int r = sq2rank(wpsq);
	uint queeningSq = fr2sq(f, RANK_8);
	int tempo = (pos.turn == strongerSide);

	// If the pawn is not too far advanced and the defending king defends the
	// queening square, use the third-rank defense.
	if (   r <= RANK_5
		&& square_distance(bksq, queeningSq) <= 1
		&& wksq <= 39  // square H5
		&& (sq2rank(brsq) == RANK_6 || (r <= RANK_3 && sq2rank(wrsq) != RANK_6)))
		return SCALE_FACTOR_DRAW;

	// The defending side saves a draw by checking from behind in case the pawn
	// has advanced to the 6th rank with the king behind.
	if (   r == RANK_6
		&& square_distance(bksq, queeningSq) <= 1
		&& sq2rank(wksq) + tempo <= RANK_6
		&& (sq2rank(brsq) == RANK_1 || (!tempo && abs(sq2file(brsq) - f) >= 3)))
		return SCALE_FACTOR_DRAW;

	if (   r >= RANK_6
		&& bksq == queeningSq
		&& sq2rank(brsq) == RANK_1
		&& (!tempo || square_distance(wksq, wpsq) >= 2))
		return SCALE_FACTOR_DRAW;

	// White pawn on a7 and rook on a8 is a draw if black king is on g7 or h7
	// and the black rook is behind the pawn.
	if (   wpsq == 48 // sq A7
		&& wrsq == 56  // sq A8
		&& (bksq == 55 || bksq == 54)  // H7 and G7
		&& sq2file(brsq) == FILE_A
		&& (sq2rank(brsq) <= RANK_3 || sq2file(wksq) >= FILE_D || sq2rank(wksq) <= RANK_5))
		return SCALE_FACTOR_DRAW;

	// If the defending king blocks the pawn and the attacking king is too far
	// away, it's a draw.
	if (   r <= RANK_5
		&& bksq == wpsq + 8
		&& square_distance(wksq, wpsq) - tempo >= 2
		&& square_distance(wksq, brsq) - tempo >= 2)
		return SCALE_FACTOR_DRAW;

	// Pawn on the 7th rank supported by the rook from behind usually wins if the
	// attacking king is closer to the queening square than the defending king,
	// and the defending king cannot gain tempo by threatening the attacking rook.
	if (   r == RANK_7
		&& f != FILE_A
		&& sq2file(wrsq) == f
		&& wrsq != queeningSq
		&& (square_distance(wksq, queeningSq) < square_distance(bksq, queeningSq) - 2 + tempo)
		&& (square_distance(wksq, queeningSq) < square_distance(bksq, wrsq) + tempo))
		return SCALE_FACTOR_MAX - 2 * square_distance(wksq, queeningSq);

	// Similar to the above, but with the pawn further back
	if (   f != FILE_A
		&& sq2file(wrsq) == f
		&& wrsq < wpsq
		&& (square_distance(wksq, queeningSq) < square_distance(bksq, queeningSq) - 2 + tempo)
		&& (square_distance(wksq, wpsq + 8) < square_distance(bksq, wpsq + 8) - 2 + tempo)
		&& (  square_distance(bksq, wrsq) + tempo >= 3
		|| (    square_distance(wksq, queeningSq) < square_distance(bksq, wrsq) + tempo
		&& (square_distance(wksq, wpsq + 8) < square_distance(bksq, wrsq) + tempo))))
		return SCALE_FACTOR_MAX
		- 8 * square_distance(wpsq, queeningSq)
		- 2 * square_distance(wksq, queeningSq);

	// If the pawn is not far advanced, and the defending king is somewhere in
	// the pawn's path, it's probably a draw.
	if (r <= RANK_4 && bksq > wpsq)
	{
		if (sq2file(bksq) == sq2file(wpsq))
			return 10;
		if (   abs(sq2file(bksq) - sq2file(wpsq)) == 1
			&& square_distance(wksq, bksq) > 2)
			return 24 - 2 * square_distance(wksq, bksq);
	}
	return SCALE_FACTOR_NONE;
}

/// K, rook and two pawns vs K, rook and one pawn. There is only a single
/// pattern: If the stronger side has no passed pawns and the defending king
/// is actively placed, the position is drawish.
/*
template<>
ScaleFactor EndEvaluator<KRPPKRP>::operator()(const Position& pos) const 
{
	uint wpsq1 = pos.pieceList[strongerSide][PAWN][0];
	uint wpsq2 = pos.pieceList[strongerSide][PAWN][1];
	uint bksq = pos.king_sq(weakerSide);

	// Does the stronger side have a passed pawn?
	if (   pos.pawn_is_passed(strongerSide, wpsq1)
		|| pos.pawn_is_passed(strongerSide, wpsq2))
		return SCALE_FACTOR_NONE;

	int r = max(relative_rank(strongerSide, wpsq1), relative_rank(strongerSide, wpsq2));

	if (   file_distance(bksq, wpsq1) <= 1
		&& file_distance(bksq, wpsq2) <= 1
		&& relative_rank(strongerSide, bksq) > r)
	{
		switch (r) {
		case RANK_2: return 10;
		case RANK_3: return 10;
		case RANK_4: return 15;
		case RANK_5: return 20;
		case RANK_6: return 40;
		}
	}
	return SCALE_FACTOR_NONE;
}
*/