#include "position.h"
#include "search.h"
#include "uci.h"
#include "thread.h"

/* Excalibur engine entry point */
int main(int argc, char **argv)
{
	display_info;

	Utils::init();
	Board::init_tables();
	Zobrist::init_keys_psqt();
	UCI::init();
	Eval::init();
	ThreadPool::init();
	TT.set_size((int) OptMap["Hash"]); // set transposition table size

	UCI::process();

	ThreadPool::clean();
	// Static evaluation debugging
	/*string fen = concat_args(argc, argv);
	Position pos(fen);
	Value margin = 0;
	Search::RootColor = pos.turn;
	Value value = Eval::evaluate(pos, margin);
	cout << "~~~~~~~~~~~~~~~~~~~" << endl;
	cout << "Value = " << fixed << setprecision(2) 
	<< centi_pawn(value) << endl;
	cout << "Margin = " << centi_pawn(margin) << endl;*/

	return 0;
}