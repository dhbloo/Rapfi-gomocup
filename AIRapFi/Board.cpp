#include "Board.h"

Board::Board(UInt8 boardSize_) {
	boardSize = boardSize_;
	boardSizeSqr = boardSize * boardSize;
	center = boardSize / 2;
	boardStartPos = POS(0, 0);
	boardEndPos = POS(boardSize - 1, boardSize - 1);
	initBoard();
	initZobrish();
	historyMoves = new Pos[boardSizeSqr];
	historyAreas = new CandArea[boardSizeSqr];
}

Board::~Board() {
	delete[] historyMoves;
	delete[] historyAreas;
}

void Board::initZobrish() {
	for (int i = 0; i < MaxBoardSizeSqr; i++)
		zobrist[0][i] = random();
	for (int i = 0; i < MaxBoardSizeSqr; i++)
		zobrist[1][i] = random();
	zobristKey = random();
}

void Board::initBoard() {
	for (int i = 0; i < MaxBoardSizeSqr; i++) {
		board[i] = (CoordX(i) >= 0 && CoordX(i) < boardSize && CoordY(i) >= 0 && CoordY(i) < boardSize) ? Empty : Wrong;
	}
}

void Board::clear() {
	initBoard();
	moveCount = 0;
	nullMoveCount = 0;
	playerToMove = Black;
	playerToMoveOppo = White;
	playerWon = Empty;
	zobristKey = 0;
	area = CandArea();
}

inline void Board::setPiece(Pos pos, Piece piece) {
	assert(isInBoard(pos));
	assert(board[pos] == Empty);
	board[pos] = piece;
	zobristKey ^= zobrist[piece][pos];
}

inline void Board::delPiece(Pos pos) {
	assert(isInBoard(pos));
	assert(board[pos] <= White);
	zobristKey ^= zobrist[board[pos]][pos];
	board[pos] = Empty;
}

void Board::move(Pos pos) {
	setPiece(pos, playerToMove);
	historyMoves[moveCount] = pos;
	historyAreas[moveCount] = area;
	area.expend(pos, boardSize);
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
	moveCount++;
}

void Board::undo() {
	assert(moveCount > 0);
	moveCount--;
	delPiece(historyMoves[moveCount]);
	area = historyAreas[moveCount];
	playerToMove = Opponent(playerToMove);
	playerToMoveOppo = Opponent(playerToMoveOppo);
}

void Board::muiltMove(Pos pos) {
	setPiece(pos, playerToMove);
	historyMoves[moveCount] = pos;
	historyAreas[moveCount] = area;
	area.expend(pos, boardSize);
	moveCount++;
}

void Board::muiltUndo() {
	assert(moveCount > 0);
	moveCount--;
	delPiece(historyMoves[moveCount]);
	area = historyAreas[moveCount];
}

void Board::makeNullMove() {
	if (nullMoveCount == 0) {
		playerToMove = Opponent(playerToMove);
		playerToMoveOppo = Opponent(playerToMoveOppo);
	}
	nullMoveCount++;
}

void Board::undoNullMove() {
	assert(nullMoveCount > 0);
	nullMoveCount--;
	if (nullMoveCount == 0) {
		playerToMove = Opponent(playerToMove);
		playerToMoveOppo = Opponent(playerToMoveOppo);
	}
}

bool Board::check5InLine(Pos origin, Delta d, Piece p) {
	int count1 = 0, count2 = 0;
	Pos pos = origin;
	pos += d;
	for (int i = 0; i < 4; i++) {
		if (get(pos) == p) {
			count1++;
			pos += d;
		} else break;
	}
	origin -= d;
	for (int i = 0; i < 4 - count1; i++) {
		if (get(origin) == p) {
			count2++;
			origin -= d;
		} else break;
	}
	if (count1 + count2 >= 4)
		return true;
	else
		return false;
}

bool Board::checkWin() {
	if (moveCount < 5) return false;
	Pos lastPos = historyMoves[moveCount - 1];
	Piece lastPiece = get(lastPos);
	assert(lastPiece != Empty);
	for (int i = 0; i < 4; i++) {
		if (check5InLine(lastPos, D[i], lastPiece)) {
			playerWon = lastPiece;
			return true;
		}
	}
	return false;
}

void CandArea::expend(Pos p, UInt8 boardSize) {
	x0 = MIN(MIN(x0, MAX(CoordX(p) - 3, 0)), boardSize - 5);
	y0 = MIN(MIN(y0, MAX(CoordY(p) - 3, 0)), boardSize - 5);
	x1 = MAX(MAX(x1, MIN(CoordX(p) + 3, boardSize - 1)), 4);
	y1 = MAX(MAX(y1, MIN(CoordY(p) + 3, boardSize - 1)), 4);
}

void Board::expendCandArea(Pos pos, int expendWidth) {
	area.x0 = MIN(MIN(area.x0, MAX(CoordX(pos) - expendWidth, 0)), boardSize - 5);
	area.y0 = MIN(MIN(area.y0, MAX(CoordY(pos) - expendWidth, 0)), boardSize - 5);
	area.x1 = MAX(MAX(area.x1, MIN(CoordX(pos) + expendWidth, boardSize - 1)), 4);
	area.y1 = MAX(MAX(area.y1, MIN(CoordY(pos) + expendWidth, boardSize - 1)), 4);
}