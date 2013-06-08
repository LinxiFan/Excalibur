#include "position.h"
#define update moveBuffer[index++] = mv // add an entry to the buffer

Move moveBuffer[4096];
int moveBufEnds[64];

/*
 * generate a pseudo-legal move and store it into a board buffer
 * The first free location in moveBuffer[] is given in parameter index
 * the new first location is returned
 */
int Position::genHelper(int index, Bit Target, bool willKingMove)
{
	Color opponent = flipColor[turn];
	Bit Freesq = ~Occupied;
	Bit TempPiece, TempMove;
	uint from, to;
	Move mv;

	/*************** Pawns ***************/
	TempPiece = Pawns[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = pawn_push(from) & Freesq & Target;  // normal push
		if (TempMove != 0) // double push possible
			TempMove |= pawn_push2(from) & Freesq & Target; 
		TempMove |= pawn_attack(from) & Pieces[opponent] & Target;  // pawn capture
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			if (Board::forward_sq(to, turn) == Board::INVALID_SQ) // If forward is invalid, then we reach the last rank
			{
				mv.setPromo(QUEEN); update;
				mv.setPromo(ROOK); update;
				mv.setPromo(BISHOP); update;
				mv.setPromo(KNIGHT); update;
				mv.clearSpecial();
			}
			else
				update;
		}
		if (st->epSquare) // en-passant
		{
			if (pawn_attack(from) & setbit[st->epSquare] & Target)
			{
				// final check to avoid same color capture
				if (Pawns[opponent] & setbit[Board::backward_sq(st->epSquare, turn)])
				{
					mv.setEP();
					mv.setTo(st->epSquare);
					update;
				}
			}
		}
		mv.clearSpecial();  // clear the EP and promo square.
	}
	mv.clear();  

	/*************** Knights ****************/
	TempPiece = Knights[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = Board::knight_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Bishops ****************/
	TempPiece = Bishops[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = bishop_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Rooks ****************/
	TempPiece = Rooks[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = rook_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	/*************** Queens ****************/
	TempPiece = Queens[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = queen_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
	}
	mv.clear();

	if (willKingMove)
	{
	/*************** Kings ****************/
	TempPiece = Kings[turn];
	while (TempPiece)
	{
		from = popLSB(TempPiece);
		mv.setFrom(from);
		TempMove = Board::king_attack(from) & Target;
		while (TempMove)
		{
			to = popLSB(TempMove);
			mv.setTo(to);
			update;
		}
		// King side castling O-O
		if (canCastleOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_FG] & Occupied))  // no pieces between the king and rook
				if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_EG], opponent))
					moveBuffer[index ++] = MOVE_OO_KING[turn];  // pre-stored king's castling move
		}
		if (canCastleOOO(st->castleRights[turn]))
		{
			if (!(CASTLE_MASK[turn][CASTLE_BD] & Occupied))  // no pieces between the king and rook
				if (!isBitAttacked(CASTLE_MASK[turn][CASTLE_CE], opponent))
					moveBuffer[index ++] = MOVE_OOO_KING[turn];  // pre-stored king's castling move
		}
	}

	}  // willKingMove option.

	return index;
}

/*
 *	Generate pseudo-legal check evasions. Include king's flee and blocking
 */
int Position::genEvasions(int index)
{
	Bit Ck = st->CheckerMap;
	int ckCount = 0;
	return 0;
}

/*
 *	Move legality test to see if any '1' in Target is attacked by the specific color
 * for check detection and castling legality
 */
bool Position::isBitAttacked(Bit Target, Color attacker_side)
{
	uint to;
	Color defender_side = flipColor[attacker_side];
	Bit pawn_map = Pawns[attacker_side];
	Bit knight_map = Knights[attacker_side];
	Bit king_map = Kings[attacker_side];
	Bit ortho_slider_map = Rooks[attacker_side] | Queens[attacker_side];
	Bit diag_slider_map = Bishops[attacker_side] | Queens[attacker_side];
	while (Target)
	{
		to = popLSB(Target);
		if (knight_map & Board::knight_attack(to))  return true; 
		if (king_map & Board::king_attack(to))  return true; 
		if (pawn_map & Board::pawn_attack(to, defender_side))  return true; 
		if (ortho_slider_map & rook_attack(to))  return true; 
		if (diag_slider_map & bishop_attack(to))  return true;
	}
	return false; 
}


/*
 *	Make move and update the Position internal states
 */
void Position::makeMove(Move& mv, StateInfo& nextSt)
{
	// copy to the next state and begin updating the new state object
	memcpy(&nextSt, st, STATEINFO_COPY_SIZE * sizeof(U64));
	nextSt.st_prev = st;
	st = &nextSt;

	uint from = mv.getFrom();
	uint to = mv.getTo();
	Bit ToMap = setbit[to];  // to update the captured piece's bitboard
	Bit FromToMap = setbit[from] | ToMap;
	PieceType piece = boardPiece[from];
	PieceType capt = boardPiece[to];
	Color opponent = flipColor[turn];

	Pieces[turn] ^= FromToMap;
	boardPiece[from] = NON;
	boardPiece[to] = piece;
	st->epSquare = 0;
	if (turn == B)  st->fullMove ++;  // only increments after black moves

	switch (piece)
	{
	case PAWN:
		Pawns[turn] ^= FromToMap;
		st->fiftyMove = 0;  // any pawn move resets the fifty-move clock
		if (ToMap == pawn_push2(from)) // if pawn double push
			st->epSquare = Board::forward_sq(from, turn);  // new ep square, directly ahead the 'from' square
		if (mv.isEP())  // en-passant capture
		{
			capt = PAWN;
			uint ep_sq = Board::backward_sq(st->st_prev->epSquare, turn);
			ToMap = setbit[ep_sq];  // the captured pawn location
			boardPiece[ep_sq] = NON;
		}
		else if (mv.isPromo())
		{
			PieceType promo = mv.getPromo();
			Pawns[turn] ^= ToMap;  // the pawn's no longer there
			-- pieceCount[turn][PAWN];
			++ pieceCount[turn][promo];
			boardPiece[to] = promo;
			switch (promo)
			{
			case KNIGHT: Knights[turn] ^= ToMap; break;
			case BISHOP: Bishops[turn] ^= ToMap; break;
			case ROOK: Rooks[turn] ^= ToMap; break;
			case QUEEN: Queens[turn] ^= ToMap; break;
			}
		}
		break;

	case KING:
		Kings[turn] ^= FromToMap; 
		kingSq[turn] = to; // update king square
		st->castleRights[turn] = 0;  // cannot castle any more
		if (mv.isCastle())
		{
			if (FILES[to] == 6)  // King side castle
			{
				Rooks[turn] ^= MASK_OO_ROOK[turn];
				Pieces[turn] ^= MASK_OO_ROOK[turn];
				boardPiece[SQ_OO_ROOK[turn][0]] = NON;  // from
				boardPiece[SQ_OO_ROOK[turn][1]] = ROOK;  // to
			}
			else
			{
				Rooks[turn] ^= MASK_OOO_ROOK[turn];
				Pieces[turn] ^= MASK_OOO_ROOK[turn];
				boardPiece[SQ_OOO_ROOK[turn][0]] = NON;  // from
				boardPiece[SQ_OOO_ROOK[turn][1]] = ROOK;  // to
			}
		}
		break;

	case ROOK:
		Rooks[turn] ^= FromToMap;
		if (from == SQ_OO_ROOK[turn][0])   // if the rook moves, cancel the castle rights
			deleteCastleOO(st->castleRights[turn]);
		else if (from == SQ_OOO_ROOK[turn][0])
			deleteCastleOOO(st->castleRights[turn]);
		break;

	case KNIGHT:
		Knights[turn] ^= FromToMap; break;
	case BISHOP:
		Bishops[turn] ^= FromToMap; break;
	case QUEEN:
		Queens[turn] ^= FromToMap; break;
	}

	/* Deal with all kinds of captures, including en-passant */
	if (capt)
	{
		switch (capt)
		{
		case PAWN: Pawns[opponent] ^= ToMap; break;
		case KNIGHT: Knights[opponent] ^= ToMap; break;
		case BISHOP: Bishops[opponent] ^= ToMap; break;
		case ROOK: // if rook is captured, the castling right is canceled
			if (to == SQ_OO_ROOK[opponent][0])  deleteCastleOO(st->castleRights[opponent]);
			else if (to == SQ_OOO_ROOK[opponent][0])  deleteCastleOOO(st->castleRights[opponent]);
			Rooks[opponent] ^= ToMap; 
			break;
		case QUEEN: Queens[opponent] ^= ToMap; break;
		case KING: Kings[opponent] ^= ToMap; break;
		}
		-- pieceCount[opponent][capt];
		Pieces[opponent] ^= ToMap;
		st->fiftyMove = 0;  // deal with fifty move counter
	}
	else if (piece != PAWN)
		st->fiftyMove ++;

	st->capt = capt;
	Occupied = Pieces[W] | Pieces[B];

	turn = opponent;
}

// Unmake move and restore the Position internal states
void Position::unmakeMove(Move& mv)
{
	uint from = mv.getFrom();
	uint to = mv.getTo();
	Bit ToMap = setbit[to];  // to update the captured piece's bitboard
	Bit FromToMap = setbit[from] | ToMap;
	bool isPromo = mv.isPromo();
	PieceType piece = isPromo ? PAWN : boardPiece[to];
	PieceType capt = st->capt;
	Color opponent = turn;
	turn = flipColor[turn];

	Pieces[turn] ^= FromToMap;
	boardPiece[from] = piece; // restore
	boardPiece[to] = NON;

	switch (piece)
	{
	case PAWN:
		Pawns[turn] ^= FromToMap;
		if (mv.isEP())  // en-passant capture
		{
			uint ep_sq = Board::backward_sq(st->st_prev->epSquare, turn);
			ToMap = setbit[ep_sq];  // the captured pawn location
			to = ep_sq;  // will restore the captured pawn later, with all other capturing cases
		}
		else if (isPromo)
		{
			PieceType promo = mv.getPromo();
			Pawns[turn] ^= ToMap;  // flip back
			++ pieceCount[turn][PAWN];
			-- pieceCount[turn][promo];
			boardPiece[from] = PAWN;
			boardPiece[to] = NON;
			switch (promo)  // flip back the promoted bit
			{
			case KNIGHT: Knights[turn] ^= ToMap; break;
			case BISHOP: Bishops[turn] ^= ToMap; break;
			case ROOK: Rooks[turn] ^= ToMap; break;
			case QUEEN: Queens[turn] ^= ToMap; break;
			}
		}
		break;

	case KING:
		Kings[turn] ^= FromToMap;
		kingSq[turn] = from; // restore the original king position
		if (mv.isCastle())
		{
			if (FILES[to] == 6)  // king side castling
			{
				Rooks[turn] ^= MASK_OO_ROOK[turn];
				Pieces[turn] ^= MASK_OO_ROOK[turn];
				boardPiece[SQ_OO_ROOK[turn][0]] = ROOK;  // from
				boardPiece[SQ_OO_ROOK[turn][1]] = NON;  // to
			}
			else
			{
				Rooks[turn] ^= MASK_OOO_ROOK[turn];
				Pieces[turn] ^= MASK_OOO_ROOK[turn];
				boardPiece[SQ_OOO_ROOK[turn][0]] = ROOK;  // from
				boardPiece[SQ_OOO_ROOK[turn][1]] = NON;  // to
			}
		}
		break;

	case ROOK:
		Rooks[turn] ^= FromToMap; break;
	case KNIGHT:
		Knights[turn] ^= FromToMap; break;
	case BISHOP:
		Bishops[turn] ^= FromToMap; break;
	case QUEEN:
		Queens[turn] ^= FromToMap; break;
	}

	// Deal with all kinds of captures, including en-passant
	if (capt)
	{
		switch (capt)
		{
		case PAWN: Pawns[opponent] ^= ToMap; break;
		case KNIGHT: Knights[opponent] ^= ToMap; break;
		case BISHOP: Bishops[opponent] ^= ToMap; break;
		case ROOK: Rooks[opponent] ^= ToMap; 	break;
		case QUEEN: Queens[opponent] ^= ToMap; break;
		case KING: Kings[opponent] ^= ToMap; break;
		}
		++ pieceCount[opponent][capt];
		Pieces[opponent] ^= ToMap;
		boardPiece[to] = capt;  // restore the captured piece
	}

	Occupied = Pieces[W] | Pieces[B];

	st = st->st_prev; // recover the state from previous position
}


// return 0 - no mate; CHECKMATE or STALEMATE - defined in typeconsts.h
int Position::mateStatus()
{
	bool isDead = true;  // can be check OR stalemate
	// we use the last 218 places in the moveBuffer to ensure we don't override any previous moves
	// 4096 - 218, 218 is the most move a legal position can ever make
	const int start = 3878;  
	int end = genAllPseudoMove(start);
	Move m;
	StateInfo si;
	for (int i = start; i < end; i++)
	{
		m = moveBuffer[i];
		makeMove(m, si);
		if (!isOppKingAttacked())  // make strictly legal move
			isDead = false;  // found a legal move
		unmakeMove(m);
	}
	if (isDead) 
		return (isOwnKingAttacked()) ? CHECKMATE : STALEMATE;
	else
		return 0;  // good and sound
}