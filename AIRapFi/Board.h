#pragma once
#include "Define.h"

#define MAX_BOARD_SIZE_BIT 5
#define BOARD_BOUNDARY 4

enum Piece : UInt8 {
	Black = 0, // 00(Bin)
	White = 1, // 01(Bin)
	Empty = 3, // 11(Bin)
	Wrong = 2  // 10(Bin)
};

inline ostream & operator<<(ostream & out, const Piece piece) {
	switch (piece) {
	case Empty: return out << "Empty";
	case Black: return out << "Black";
	case White: return out << "White";
	default: return out << "Wrong";
	}
}

inline Piece Opponent(const Piece piece) { return static_cast<Piece>(piece ^ (UInt8)1); }

typedef short Delta;
typedef UShort Pos;

const Pos NullPos = UShort(0);

//四个方向的变化
const Int8 D[4] = {
	1,                             //{ 0, 1 } = 0 * 32 + 1
	(1 << MAX_BOARD_SIZE_BIT) - 1, //{ 1,-1 } = 1 * 32 - 1
	(1 << MAX_BOARD_SIZE_BIT),     //{ 1, 0 } = 1 * 32 + 0
	(1 << MAX_BOARD_SIZE_BIT) + 1, //{ 1, 1 } = 1 * 32 + 1
};

const Int8 RANGE_NEIGHBOR[8] = {
	-D[3], -D[2], -D[1], -D[0],
	 D[0],  D[1],  D[2],  D[3],
};

const Int8 RANGE_MIN[16] = {
	-D[3] - D[3], -D[2] - D[2], -D[1] - D[1], 
	-D[3]       , -D[2]       , -D[1]       ,
	-D[0] - D[0], -D[0]       ,  D[0]       ,  D[0] + D[0],
	 D[1]       ,  D[2]       ,  D[3]       ,
	 D[1] + D[1],  D[2] + D[2],  D[3] + D[3],
};

const Int8 RANGE_MIDDLE[24] = {
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],
	-D[0] - D[0], -D[0]       ,                D[0]       ,  D[0] + D[0],
	 D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	 D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
};

const Int8 RANGE_LARGE[32] = {
	-D[3] - D[3] - D[3], -D[2] - D[2] - D[2], -D[1] - D[1] - D[1],
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],
	-D[0] - D[0] - D[0], -D[0] - D[0], -D[0],                
	 D[0],  D[0] + D[0],  D[0] + D[0] + D[0],
	D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
	D[1] + D[1] + D[1], D[2] + D[2] + D[2], D[3] + D[3] + D[3]
};

const short RANGE_LINE_4[32] = {
	-D[3] - D[3] - D[3] - D[3], -D[2] - D[2] - D[2] - D[2], -D[1] - D[1] - D[1] - D[1],
		  - D[3] - D[3] - D[3],	      - D[2] - D[2] - D[2],		  - D[1] - D[1] - D[1],
		          -D[3] - D[3],	             - D[2] - D[2],              - D[1] - D[1],
	                    - D[3],                     - D[2],                     - D[1],
	-D[0] - D[0] - D[0] - D[0],       - D[0] - D[0] - D[0],              - D[0] - D[0], -D[0],
	                      D[0],                D[0] + D[0],         D[0] + D[0] + D[0],  D[0] + D[0] + D[0] + D[0],
						  D[1],                       D[2],                       D[3],
				   D[1] + D[1],                D[2] + D[2],                D[3] + D[3],
			D[1] + D[1] + D[1],         D[2] + D[2] + D[2],         D[3] + D[3] + D[3],
	 D[1] + D[1] + D[1] + D[1],  D[2] + D[2] + D[2] + D[2],  D[3] + D[3] + D[3] + D[3],
};

const short RANGE_4[40] = {
	-D[3] - D[3] - D[3] - D[3], -D[2] - D[2] - D[2] - D[2], -D[1] - D[1] - D[1] - D[1],
	       -D[3] - D[3] - D[3],        -D[2] - D[2] - D[2],        -D[1] - D[1] - D[1],
	-D[3] - D[3], -D[3] - D[2], -D[2] - D[2], -D[2] - D[1], -D[1] - D[1],
	-D[3] - D[0], -D[3]       , -D[2]       , -D[1]       , -D[1] + D[0],

	-D[0] - D[0] - D[0] - D[0], -D[0] - D[0] - D[0], -D[0] - D[0], -D[0],
	 D[0],  D[0] + D[0],  D[0] + D[0] + D[0],  D[0] + D[0] + D[0] + D[0],

	D[1] - D[0],  D[1]       ,  D[2]       ,  D[3]       ,  D[3] + D[0],
	D[1] + D[1],  D[1] + D[2],  D[2] + D[2],  D[3] + D[2],  D[3] + D[3],
	       D[1] + D[1] + D[1],         D[2] + D[2] + D[2],         D[3] + D[3] + D[3],
	D[1] + D[1] + D[1] + D[1],  D[2] + D[2] + D[2] + D[2],  D[3] + D[3] + D[3] + D[3],
};

inline Pos POS_R(UInt8 x = 0, UInt8 y = 0) { return (x << MAX_BOARD_SIZE_BIT) + y; }
inline Pos POS(UInt8 x = 0, UInt8 y = 0) { return POS_R(x + BOARD_BOUNDARY, y + BOARD_BOUNDARY); }
inline UInt8 CoordX(Pos p) { return (p >> MAX_BOARD_SIZE_BIT) - BOARD_BOUNDARY; }
inline UInt8 CoordY(Pos p) { return (p & ((1 << MAX_BOARD_SIZE_BIT) - 1)) - BOARD_BOUNDARY; }

struct PosStr {
private:
	Pos p;
public:
	PosStr(Pos p) : p(p) {}
	inline Pos operator()() { return p; }
};

inline ostream & operator<<(ostream & out, PosStr pos) {
#ifdef _DEBUG
	return out << "[" << char(CoordY(pos()) + 65) << "," << int(CoordX(pos()) + 1) << "]";
#else
	return out << "[" << int(CoordX(pos()) + 1) << "," << int(CoordY(pos()) + 1) << "]";
#endif
}

inline string YXPos(Pos pos, UInt8 boardSize) {
	std::ostringstream s;
	s << "[" << char(CoordY(pos) + 65) << "," << int(boardSize - CoordX(pos)) << "]";
	return s.str();
}

inline int distance(Pos p1, Pos p2) {
	return MAX(ABS(static_cast<int>(CoordX(p1)) - CoordX(p2)), ABS(static_cast<int>(CoordY(p1)) - CoordY(p2)));
}

inline bool isInLine(const Pos & p1, const Pos & p2) {
	//To-do: 优化
	if (CoordX(p1) == CoordX(p2) || CoordY(p1) == CoordY(p2)) return true;
	return ABS(static_cast<int>(CoordX(p1)) - CoordX(p2)) == ABS(static_cast<int>(CoordY(p1)) - CoordY(p2));
}

struct Move {
	Pos pos;
	int value;

	Move() : pos(NullPos), value(SHRT_MIN) {}
	Move(int x, int y, int value) : pos(POS(x, y)), value(value) {}
	Move(Pos pos, int value) : pos(pos), value(value) {}
	inline bool operator == (const Move & move) { return pos == move.pos; }
	inline bool operator < (const Move & move) const { return value < move.value; }
	inline bool operator > (const Move & move) const { return value > move.value; }
};

struct CandArea {
	UInt8 x0, y0, x1, y1;

	CandArea() : x0(255), y0(255), x1(0), y1(0) {}
	CandArea(UInt8 x0, UInt8 y0, UInt8 x1, UInt8 y1) : x0(x0), y0(y0), x1(x1), y1(y1) {}

	void expend(Pos p, UInt8 boardSize);
};

class Board {
public:
	static const UInt8 MaxBoardSize = 1 << MAX_BOARD_SIZE_BIT;
	static const int MaxBoardSizeSqr = MaxBoardSize * MaxBoardSize;
	static const UInt8 RealBoardSize = MaxBoardSize - 2 * BOARD_BOUNDARY;

private:
	Piece board[MaxBoardSizeSqr];
	UInt8 boardSize, center;
	Pos boardStartPos, boardEndPos;
	int boardSizeSqr;
	int moveCount = 0;
	int nullMoveCount = 0;
	Pos* historyMoves;

	Piece playerToMove = Black;
	Piece playerToMoveOppo = White;
	Piece playerWon = Empty;

	U64 zobrist[2][MaxBoardSizeSqr];
	U64 zobristKey;

	CandArea area;
	CandArea* historyAreas;

	bool check5InLine(Pos origin, Delta d, Piece p);
	inline void setPiece(Pos pos, Piece piece);
	inline void delPiece(Pos pos);
	void initZobrish();
	void initBoard();

public:
	Board(UInt8 boardSize_);
	~Board();

	void clear();
	
	void move(Pos pos);
	void undo();
	
	void muiltMove(Pos pos);
	void muiltUndo();
	inline void switchSide();

	void makeNullMove();
	void undoNullMove();

	inline Piece get(Pos pos) const;
	inline bool isInBoard(Pos pos) const;
	inline bool isNearBoard(Pos pos, int distFromBorder) const;
	inline bool isEmpty(Pos pos) const;
	inline bool isNullMoveAvailable() const { return nullMoveCount == 0; }
	bool checkWin();

	inline int getMoveCount() const { return moveCount; };
	inline int getMoveLeftCount() const { return boardSizeSqr - moveCount; };
	inline Piece getPlayerToMove() const { return playerToMove; }
	inline Piece getPlayerToMoveOppo() const { return playerToMoveOppo; }
	inline Piece getPlayerWon() const { return playerWon; }

	inline Pos getHistoryMove(int moveIndex) const;
	inline Pos getMoveBackWard(int backIndex) const;
	inline Pos getLastMove() const;

	inline U64 getZobristKey() const { return zobristKey; }
	inline UInt8 size() const { return boardSize; }
	inline UInt8 centerPos() const { return center; }
	inline int startPos() const { return boardStartPos; }
	inline int endPos() const { return boardEndPos; }
	inline int maxCells() const { return boardSizeSqr; }
	inline CandArea const & candArea() const { return area; }

	void expendCandArea(Pos pos, int expendWidth = 0);
};

inline Piece Board::get(Pos pos) const {
	assert(pos < MaxBoardSizeSqr);
	return board[pos];
}

inline bool Board::isInBoard(Pos pos) const {
	assert(pos < MaxBoardSizeSqr);
	return board[pos] != Wrong;
}

inline bool Board::isEmpty(Pos pos) const {
	return get(pos) == Empty;
}

inline bool Board::isNearBoard(Pos pos, int distFromBorder) const {
	return CoordX(pos) < distFromBorder || CoordY(pos) < distFromBorder || CoordX(pos) >= boardSize - distFromBorder || CoordY(pos) >= boardSize - distFromBorder;
}

inline Pos Board::getHistoryMove(int moveIndex) const {
	assert(moveIndex <= moveCount);
	return historyMoves[moveIndex - 1];
}
// backIndex需大于0,且小于等于moveCount(否则返回非法着法)
inline Pos Board::getMoveBackWard(int backIndex) const {
	assert(backIndex > 0);
	backIndex = moveCount - backIndex;
	return backIndex < 0 ? NullPos : historyMoves[backIndex];
}

inline Pos Board::getLastMove() const {
	assert(moveCount > 0);
	return moveCount <= 0 ? NullPos : historyMoves[moveCount - 1];
}

inline void Board::switchSide() {
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
}