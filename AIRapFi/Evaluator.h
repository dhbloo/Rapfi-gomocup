#pragma once
#include "Board.h"

#define FOR_EVERY_POSITION(x, y) \
    for (int x = 0; x < board->size(); x++) \
        for (int y = 0; y < board->size(); y++) \

#define FOR_EVERY_POSITION_POS(pos) \
    for (Pos pos = board->startPos(); pos <= board->endPos(); pos++) \
        if (board->isInBoard(pos)) \

#define FOR_EVERY_EMPTY_POS(pos) \
    for (Pos pos = board->startPos(); pos <= board->endPos(); pos++) \
        if (board->isEmpty(pos)) \

#define FOR_EVERY_CAND_POS(pos) \
    for (UInt8 _x = board->candArea().x0, y0 = board->candArea().y0, yl = board->candArea().y1 - y0, _y = yl; _x <= board->candArea().x1; _x++, _y = yl) \
        for (Pos pos = POS(_x, y0); _y < Board::MaxBoardSize; _y--, pos++) \
            if (board->isEmpty(pos) && cell(pos).isCand()) \

#define FOR_EVERY_PIECE_POS(pos) \
	for (int i = 0; i < board->getMoveCount() ? pos = board->getHistoryMove(i), true : false; i++) \

#define SELF (board->getPlayerToMove())
#define OPPO (board->getPlayerToMoveOppo())

enum Pattern : UInt8 {
	DEAD, 
	B1,   F1,
	B2J0, B2J2,                // S 表示该棋型的最小空间(S < 5==DEAD, S最大为9)
	F2J0, F2J1, F2J2,          // J 表示该棋型的跳跃间隔
	B3J0, B3J1, B3J2,
	F3J0, F3J1,
	B4,   F4,
	F5
};

enum Pattern4 : UInt8 {
	A_FIVE = 11, B_FLEX4 = 10, C_BLOCK4_FLEX3 = 9, D_BLOCK4_PLUS = 8,
	E_BLOCK4 = 7, F_FLEX3_2X = 6, G_FLEX3_PLUS = 5, H_FLEX3 = 4,
	I_BLOCK3_PLUS = 3, J_FLEX2_2X = 2, FORBID = 1, NONE = 0
};

typedef short PatternCode;

class Evaluator {
private:
	void init();

	Pattern getPattern(UInt8 key1, UInt8 key2);
	Pattern shortLinePattern(array<Piece, 9> & line);
	Pattern checkFlex3(array<Piece, 9> & line, Pattern p1, Pattern p2);
	Pattern checkFlex4(array<Piece, 9> & line, Pattern p1, Pattern p2);
	bool checkFive(array<Piece, 9> & line, int i);
	Pattern getType(int length, int fullLength, int count, bool block, int jump);
	Pattern4 getPattern4(Pattern p1, Pattern p2, Pattern p3, Pattern p4);

	// for debug
	bool checkP4Count();

	static Pattern PATTERN[256][256];  // 65536 = 256 * 256 = 4 ^ 8
	static PatternCode PCODE[16][16][16][16];  // 65536 = 16 ^ 4
	
	// 3876 是从16种单线棋型中选出可重复的4种棋型的组合数
	static Pattern4 PATTERN4[3876];

protected:
	static short Score[3876];
	static short Value[3876];

	struct Cell {
		UInt8 key[4][2]; // 4个方向的 Black Key 和 White Key
		Pattern pattern[2][4];
		short score[2];
		short eval[2];
		Pattern4 pattern4[2];
		UInt8 cand;
		bool isLose;

		inline void clearPattern4() {
			pattern4[White] = pattern4[Black] = NONE;
		}
		inline void clearEval() {
			eval[Black] = eval[White] = 0;
		}
		inline void updatePattern(int i) {
			pattern[Black][i] = PATTERN[key[i][Black]][key[i][White]];
			pattern[White][i] = PATTERN[key[i][White]][key[i][Black]];
		}
		inline PatternCode getPatternCode(Piece piece) {
			return PCODE[pattern[piece][0]][pattern[piece][1]][pattern[piece][2]][pattern[piece][3]];
		}
		inline void updatePattern4(PatternCode codeBlack, PatternCode codeWhite) {
			pattern4[Black] = PATTERN4[codeBlack];
			pattern4[White] = PATTERN4[codeWhite];
		}
		inline void updatePattern4(Piece piece) {
			pattern4[piece] = PATTERN4[getPatternCode(piece)];
		}
		inline void updateScore(PatternCode codeBlack, PatternCode codeWhite) {
			score[Black] = Score[codeBlack];
			score[White] = Score[codeWhite];
		}
		inline void updateEval(PatternCode codeBlack, PatternCode codeWhite) {
			eval[Black] = Value[codeBlack]; 
			eval[White] = Value[codeWhite];
		}
		inline int getScore(Piece player) { return (int)score[Black] + score[White] + score[player]; }
		inline int getScore() { return (int)score[Black] + score[White]; }
		inline int getScore_VC(Piece player) { return (int)score[player]; }
		inline bool isCand() { return cand > 0; }
	};

	Board * board;
	Cell cells[Board::MaxBoardSizeSqr];

	int eval[2], evalLower[2];
	int p4Count[2][12];
	int ply;

	inline Cell & cell(Pos p) { assert(p < Board::MaxBoardSizeSqr); return cells[p]; }
	inline Cell & cell(int x, int y) { return cell(POS(x, y)); }

	Pos findPosByPattern4(Piece piece, Pattern4 p4);
	
	Pos getCostPosAgainstB4(Pos posB4, Piece piece);
	void getCostPosAgainstF3(Pos posB, Piece piece, vector<Move> & list);
	void getAllCostPosAgainstF3(Pos posB, Piece piece, set<Pos> & set);

	void expendCand(Pos pos, int fillDist, int lineDist);
	void clearLose();

public:
	Evaluator(Board * board);
	~Evaluator() {}

	enum MoveType { Normal, VC, MuiltVC };
/// #define GENERATION_MIN
/// #define GENERATION_MIDDLE
#define GENERATION_LARGE

	template <MoveType MT = Normal> void makeMove(Pos pos);
	template <MoveType MT = Normal> void undoMove();
	
	template <bool Make> inline void switchSide() {
		board->switchSide();
		if (Make) ply++;
		else ply--;
	}

	void newGame();
	Pos getHighestScoreCandPos();
	Pos databaseMove();

	// for debug
	void trace(ostream & ss, const string & appendBefore = "");
};