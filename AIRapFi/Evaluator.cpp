#include "Evaluator.h"
#include "MoveDatabase.cpp"
#include <sstream>
#include <iomanip>
#include <Windows.h>

Pattern Evaluator::PATTERN[256][256];
PatternCode Evaluator::PCODE[16][16][16][16];

Pattern4 Evaluator::PATTERN4[3876];

Evaluator::Evaluator(Board * board) : board(board) {
	init();
	newGame();
}

template <Evaluator::MoveType MT>
void Evaluator::makeMove(Pos pos) {
	Piece self = SELF;
	if (MT == MoveType::MuiltVC) {
		board->muiltMove(pos);
	} else {
		board->move(pos);
		ply++;
	}

	Pos p;
	Cell * c;
	PatternCode pCodeBlack, pCodeWhite;
	for (int i = 0; i < 4; i++) {
		p = pos - D[i] * 4;
		for (UInt8 k = 1; k < (1 << 4); k <<= 1) {
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] |= k;

				c->updatePattern(i);
				
				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
			p += D[i];
		}
		for (UInt8 k = 1 << 4; k != 0; k <<= 1) {
			p += D[i];
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] |= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
		}
	}

	c = &cell(pos);
	if (MT == MoveType::Normal) {
		eval[Black] -= c->eval[Black]; 
		eval[White] -= c->eval[White];
	}
	p4Count[Black][c->pattern4[Black]]--;
	p4Count[White][c->pattern4[White]]--;

	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cell(pos + RANGE_MIN[k]).cand++;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cell(pos + RANGE_MIDDLE[k]).cand++;
#endif
#ifdef GENERATION_LARGE
    for (int k = 0; k < 32; k++)
	    cell(pos + RANGE_LARGE[k]).cand++;
#endif
}

template void Evaluator::makeMove<Evaluator::Normal>(Pos pos);
template void Evaluator::makeMove<Evaluator::VC>(Pos pos);
template void Evaluator::makeMove<Evaluator::MuiltVC>(Pos pos);

template <Evaluator::MoveType MT>
void Evaluator::undoMove() {
	assert(board->getMoveCount() > 0);
	PatternCode pCodeBlack, pCodeWhite;
	Pos p, pos = board->getLastMove();
	if (MT == MoveType::MuiltVC) {
		board->muiltUndo();
	} else {
		board->undo();
		ply--;
	}
	Piece self = SELF;

	Cell * c = &cell(pos);
	if (MT == MoveType::Normal) {
		eval[Black] += c->eval[Black]; 
		eval[White] += c->eval[White];
	}
	p4Count[Black][c->pattern4[Black]]++;
	p4Count[White][c->pattern4[White]]++;
	
	for (int i = 0; i < 4; i++) {
		p = pos - 4 * D[i];
		for (UInt8 k = 1; k < (1 << 4); k <<= 1) {
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] ^= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
			p += D[i];
		}
		for (UInt8 k = 1 << 4; k != 0; k <<= 1) {
			p += D[i];
			if (board->isEmpty(p)) {
				c = &cell(p);
				c->key[i][self] ^= k;

				c->updatePattern(i);

				pCodeBlack = c->getPatternCode(Black);
				pCodeWhite = c->getPatternCode(White);

				c->updateScore(pCodeBlack, pCodeWhite);

				if (MT == MoveType::Normal) {
					eval[Black] -= c->eval[Black]; eval[White] -= c->eval[White];
					c->updateEval(pCodeBlack, pCodeWhite);
					eval[Black] += c->eval[Black]; eval[White] += c->eval[White];
				}

				p4Count[Black][c->pattern4[Black]]--; p4Count[White][c->pattern4[White]]--;
				c->updatePattern4(pCodeBlack, pCodeWhite);
				p4Count[Black][c->pattern4[Black]]++; p4Count[White][c->pattern4[White]]++;
			}
		}
	}
	assert(checkP4Count());

#ifdef GENERATION_MIN
	for (int k = 0; k < 16; k++)
		cell(pos + RANGE_MIN[k]).cand--;
#endif
#ifdef GENERATION_MIDDLE
	for (int k = 0; k < 24; k++)
		cell(pos + RANGE_MIDDLE[k]).cand--;
#endif
#ifdef GENERATION_LARGE
	for (int k = 0; k < 32; k++)
		cell(pos + RANGE_LARGE[k]).cand--;
#endif
}

template void Evaluator::undoMove<Evaluator::Normal>();
template void Evaluator::undoMove<Evaluator::VC>();
template void Evaluator::undoMove<Evaluator::MuiltVC>();

bool Evaluator::checkP4Count() {
	int p4[2][12] = { 0 };
	FOR_EVERY_EMPTY_POS(p) {
		p4[Black][cell(p).pattern4[Black]]++;
		p4[White][cell(p).pattern4[White]]++;
	}
	for (int k = 0; k < 2; k++) {
		for (int i = 1; i < 12; i++) {
			if (p4[k][i] != p4Count[k][i])
				return false;
		}
	}
	return true;
}

void Evaluator::newGame() {
	board->clear();
	ply = 0;
	eval[0] = eval[1] = 0;
	evalLower[0] = evalLower[1] = 0;
	memset(cells, 0, sizeof(cells));
	memset(p4Count, 0, sizeof(p4Count));

	FOR_EVERY_POSITION_POS(p) {
		Cell & c = cell(p);
		for (int i = 0; i < 4; i++) {
			UInt key = 0;
			Pos pi = p - 4 * D[i];
			for (UInt8 k = 1 << 7; k >= (1 << 4); k >>= 1) {
				if (board->get(pi) == Wrong) key |= k;
				pi += D[i];
			}
			for (UInt8 k = 1 << 3; k != 0; k >>= 1) {
				pi += D[i];
				if (board->get(pi) == Wrong) key |= k;
			}
			c.key[i][White] = c.key[i][Black] = key;
			c.updatePattern(i);
			assert(c.pattern[Black][i] <= F1);
			assert(c.pattern[White][i] <= F1);
		}
		PatternCode pCodeBlack = c.getPatternCode(Black);
		PatternCode pCodeWhite = c.getPatternCode(White);
		c.updateEval(pCodeBlack, pCodeWhite);
		eval[Black] += c.eval[Black]; 
		eval[White] += c.eval[White];
		c.updatePattern4(pCodeBlack, pCodeWhite);
		c.updateScore(pCodeBlack, pCodeWhite);
		p4Count[Black][c.pattern4[Black]]++; p4Count[White][c.pattern4[White]]++;
	}
	assert(checkP4Count());
}

void Evaluator::init() {
	const int N = 16;
	int v[N * N * N * N] = { -1 };

	for (int x = 0, i = 0; x < N; x++)
		for (int y = 0; y < N; y++)
			for (int z = 0; z < N; z++)
				for (int w = 0; w < N; w++) {
					int a = x, b = y, c = z, d = w;
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					v[i++] = d * (N * N * N) + c * (N * N) + b * N + a;
				}

	for (int i = 0, sum = N * N * N * N; i < N * N * N * N; i++)
		if (v[i] > -1)
			for (int j = i + 1; j < N * N * N * N; j++)
				if (v[i] == v[j]) v[j] = -1, sum--;

	for (int i = 0, count = 0; i < N * N * N * N; i++)
		if (v[i] > -1) v[i] = count++;

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			for (int m = 0; m < N; m++)
				for (int n = 0; n < N; n++) {
					int a = i, b = j, c = m, d = n;
					if (b > a) swap(a, b);
					if (c > a) swap(a, c);
					if (d > a) swap(a, d);
					if (c > b) swap(c, b);
					if (d > b) swap(d, b);
					if (d > c) swap(d, c);

					int pcode = d * (N * N * N) + c * (N * N) + b * N + a;
					pcode = v[pcode];
					PCODE[i][j][m][n] = pcode;
					PATTERN4[pcode] = getPattern4(Pattern(a), Pattern(b), Pattern(c), Pattern(d));
				}

	for (int key1 = 0; key1 < 256; key1++) {
		for (int key2 = 0; key2 < 256; key2++) {
			PATTERN[key1][key2] = getPattern(key1, key2);
		}
	}
}

Pattern Evaluator::getPattern(UInt8 key1, UInt8 key2) {
	array<Piece, 9> line;
	line[4] = Black;
	for (int i = 0; i < 8; i++) {
		UInt p = 1 << i;
		UInt k1 = key1 & p;
		UInt k2 = key2 & p;
		Piece piece = k1 ? (k2 ? Wrong : Black) : (k2 ? White : Empty);
		line[i < 4 ? i : i + 1] = piece;
	}

	// 双向判断，取最大的棋型
	Pattern p1 = shortLinePattern(line);
	// 将棋型反向，再次判断
	reverse(line.begin(), line.end());
	Pattern p2 = shortLinePattern(line);

	Pattern p;

	// 同线双四，双三
	if (p1 == B4 && p2 == B4)
		p = checkFlex4(line, p1, p2);
	else if ((p1 == B3J0 || p1 == B3J1 || p1 == B3J2) && (p2 == B3J0 || p2 == B3J1 || p2 == B3J2))
		p = checkFlex3(line, p1, p2);
	else
		p = MAX(p1, p2);

	return p;
}
// 判断单线棋型
Pattern Evaluator::shortLinePattern(array<Piece, 9> & line) {
	int empty = 0, block = 0;
	int len = 1, len2 = 1, count = 1;

	Piece self = line[4];
	for (int i = 5; i <= 8; i++) {
		if (line[i] == self) {
			assert(empty + count <= 4);
			count++;
			len++;
			len2 = empty + count;
		} else if (line[i] == Empty) {
			len++;
			empty++;
		} else {
			if (line[i - 1] == self) block++;
			break;
		}
	}
	// 计算中间空格
	empty = len2 - count;
	for (int i = 3; i >= 0; i--) {
		if (line[i] == self) {
			if (empty + count > 4) break;
			count++;
			len++;
			len2 = empty + count;
		} else if (line[i] == Empty) {
			if (empty + count > 4) break;
			len++;
			empty++;
		} else {
			if (line[i + 1] == self) block++;
			break;
		}
	}
	return getType(len, len2, count, block > 0, len2 - count);
}
// 同线双三特判
Pattern Evaluator::checkFlex3(array<Piece, 9> & line, Pattern p1, Pattern p2) {
	Piece self = line[4];
	Pattern type;
	for (int i = 0; i < 9; i++) {
		if (line[i] == Empty) {
			line[i] = self;
			type = checkFlex4(line, p1, p2);
			line[i] = Empty;
			if (type >= F4)
				return F3J1;
		}
	}
	return MAX(p1, p2);
}
// 同线双四特判
Pattern Evaluator::checkFlex4(array<Piece, 9> & line, Pattern p1, Pattern p2) {
	if (checkFive(line, 4)) return F5;
	int five = 0;
	for (int i = 0; i < 9; i++) {
		if (line[i] == Empty)
			five += checkFive(line, i);
	}
	return five >= 2 ? F4 : MAX(p1, p2);
}
// 检查某个点是否是连五
bool Evaluator::checkFive(array<Piece, 9> & line, int i) {
	int count = 0;
	Piece self = line[4];
	for (int j = i - 1; j >= 0 && line[j] == self; j--)
		count++;
	for (int j = i + 1; j <= 8 && line[j] == self; j++)
		count++;
	return count >= 4;
}
// 获得对应的单线棋形
Pattern Evaluator::getType(int length, int fullLength, int count, bool block, int jump) {
	if (length < 5) return DEAD;
	if (count >= 5) return F5;
	if (length > 5 && fullLength < 5 && (!block)) {
		switch (count) {
		case 1: return F1;
		case 2: 
			switch (jump) {
			case 0: return F2J0;
			case 1: return F2J1;
			case 2: return F2J2;
			default: assert(false);
			}
		case 3: 
			switch (jump) {
			case 0: return F3J0;
			case 1: return F3J1;
			default: assert(false);
			}
		case 4: return F4;
		}
	} else {
		switch (count) {
		case 1: return B1;
		case 2:
			switch (jump) {
			case 0:
			case 1: return B2J0;
			case 2:
			case 3: return B2J2;
			default: assert(false);
			}
		case 3:
			switch (jump) {
			case 0: return B3J0;
			case 1: return B3J1;
			case 2: return B3J2;
			default: assert(false);
			}
		case 4: return B4;
		}
	}
	return DEAD;
}

Pattern4 Evaluator::getPattern4(Pattern p1, Pattern p2, Pattern p3, Pattern p4) {
	int n[16] = { 0 };
	n[p1]++; n[p2]++; n[p3]++; n[p4]++;

	if (n[F5] >= 1) return A_FIVE;                                               // OOOO_
	if (n[B4] >= 2) return B_FLEX4;                                              // XOOO_ * _OOOX
	if (n[F4] >= 1) return B_FLEX4;                                              // OOO_
	if (n[B4] >= 1) {
		if (n[F3J0] >= 1 || n[F3J1] >= 1) return C_BLOCK4_FLEX3;                 // XOOO_ * _OO
		if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) return D_BLOCK4_PLUS;  // XOOO_ * _OOX
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return D_BLOCK4_PLUS;  // XOOO_ * _O
		else return E_BLOCK4;                                                    // XOOO_
	}
	if (n[F3J0] >= 1 || n[F3J1] >= 1) {
		if (n[F3J0] + n[F3J1] >= 2) return F_FLEX3_2X;                           // OO_ * _OO
		if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) return G_FLEX3_PLUS;   // OO_ * _OOX
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return G_FLEX3_PLUS;   // OO_ * _O
		else return H_FLEX3;                                                     // OO_
	}
	if (n[B3J0] >= 1 || n[B3J1] >= 1 || n[B3J2] >= 1) {
		if (n[B3J0] + n[B3J1] + n[B3J2] >= 2) return I_BLOCK3_PLUS;              // XOO_ * XOO_
		if (n[F2J0] >= 1 || n[F2J1] >= 1 || n[F2J2] >= 1) return I_BLOCK3_PLUS;  // XOO_ * O_
	}
	if (n[F2J0] + n[F2J1] + n[F2J2] >= 2) {
		return J_FLEX2_2X;                                                       // O_ * O_
	}

	return NONE;
}

Pos Evaluator::findPosByPattern4(Piece piece, Pattern4 p4) {
	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[piece] == p4) return p;
	}
	assert(false);
	return NullPos;
}

Pos Evaluator::getCostPosAgainstB4(Pos posB4, Piece piece) {
	const int FindDistMax = 4;
	assert(p4Count[piece][A_FIVE] > 0);

	int dir;
	Cell & c = cell(posB4);
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] >= B4)
			break;
		assert(dir < 3);
	}

	Piece p;
	Pos pos = posB4;
	int i, j;
	for (i = 0; i < FindDistMax; i++) {
		pos -= D[dir];
		p = board->get(pos);
		if (p == piece) continue;
		else if (p == Empty) {
			if (cell(pos).pattern[piece][dir] == F5)
				return pos;
		}
		break;
	}
	pos = posB4;
	for (j = FindDistMax - i; j >= 1; j--) {
		pos += D[dir];
		p = board->get(pos);
		if (p == piece) continue;
		else if (p == Empty) {
			if (cell(pos).pattern[piece][dir] == F5) {
				return pos;
			}
		}
		break;
	}
	MESSAGEL("ERROR!");
	trace(cout, "MESSAGE ");
	assert(false);
	return findPosByPattern4(piece, A_FIVE);
}

// 获得某方在某位置活三时，对方可能下的某一组点
// posB: 对方下一步要下的活四点
// dirIndex: 对方上一步下的活三方向
void Evaluator::getCostPosAgainstF3(Pos posB, Piece piece, vector<Move> & list) {
	const int FindDistMax_F4 = 5;
	const int FindDistMax_B4 = 4;

	bool flex3 = false;
	int dir;
	Cell & c = cell(posB);
	list.emplace_back(posB, c.getScore_VC(piece));
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] == F4) {
			flex3 = true;
			break;
		}
	}

	if (flex3) {
		Piece p;
		Pos pos = posB, posL = NullPos, posR = NullPos;
		int i, j;
		for (i = 1; i <= FindDistMax_F4; i++) {
			pos -= D[dir];
			p = board->get(pos);
			if (p == piece) continue;
			else if (p == Empty) {
				Pattern pattern = cell(pos).pattern[piece][dir];
				if (pattern >= F4) {
					list.emplace_back(pos, cell(pos).getScore_VC(piece));
					continue;
				} else if (pattern >= B4)
					posL = pos;
			}
			break;
		}
		pos = posB;
		for (j = FindDistMax_F4 - i; j >= 1; j--) {
			pos += D[dir];
			p = board->get(pos);
			if (p == piece) continue;
			else if (p == Empty) {
				Pattern pattern = cell(pos).pattern[piece][dir];
				if (pattern >= F4) {
					list.emplace_back(pos, cell(pos).getScore_VC(piece));
					continue;
				} else if (pattern >= B4)
					posR = pos;
			}
			break;
		}
		if (posR && i <= FindDistMax_F4) list.emplace_back(posR, cell(posR).getScore_VC(piece));
		if (posL && j >= 1) list.emplace_back(posL, cell(posL).getScore_VC(piece));
	} else {
		for (dir = 0; dir < 4; dir++) {
			if (c.pattern[piece][dir] < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4) {
						list.emplace_back(pos, cell(pos).getScore_VC(piece));
						goto NoCheck_another_direction;
					}
				}
				break;
			}
			pos = posB;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4)
						list.emplace_back(pos, cell(pos).getScore_VC(piece));
				}
				break;
			}
		NoCheck_another_direction:
			continue;
		}
	}
	assert(list.size() >= 2);
}
// 获得某方在某位置活三时，对方可能下的所有点
void Evaluator::getAllCostPosAgainstF3(Pos posB, Piece piece, set<Pos> & set) {
	const int FindDistMax_F4 = 5;
	const int FindDistMax_B4 = 4;

	bool flex3 = false;
	int dir;
	Cell & c = cell(posB);
	set.insert(posB);
	for (dir = 0; dir < 4; dir++) {
		if (c.pattern[piece][dir] == F4) {
			flex3 = true;

			Piece p;
			Pos pos = posB, posL = NullPos, posR = NullPos;
			int i, j;
			for (i = 1; i <= FindDistMax_F4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					Pattern pattern = cell(pos).pattern[piece][dir];
					if (pattern >= F4) {
						set.insert(pos);
						continue;
					} else if (pattern >= B4)
						posL = pos;
				}
				break;
			}
			pos = posB;
			for (j = FindDistMax_F4 - i; j >= 1; j--) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					Pattern pattern = cell(pos).pattern[piece][dir];
					if (pattern >= F4) {
						set.insert(pos);
						continue;
					} else if (pattern >= B4)
						posR = pos;
				}
				break;
			}
			if (posR && i <= FindDistMax_F4) set.insert(posR);
			if (posL && j >= 1) set.insert(posL);
		}
	}

	if (!flex3) {
		for (dir = 0; dir < 4; dir++) {
			if (c.pattern[piece][dir] < B4) continue;

			Pos pos = posB;
			Piece p;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos -= D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4) {
						set.insert(pos);
						goto NoCheck_another_direction;
					}
				}
				break;
			}
			pos = posB;
			for (int i = 1; i <= FindDistMax_B4; i++) {
				pos += D[dir];
				p = board->get(pos);
				if (p == piece) continue;
				else if (p == Empty) {
					if (cell(pos).pattern[piece][dir] >= B4)
						set.insert(pos);
				}
				break;
			}
		NoCheck_another_direction:
			continue;
		}
	}
	assert(set.size() >= 2);
}

// 扩展选点(适用于棋在边界位置时)
void Evaluator::expendCand(Pos pos, int fillDist, int lineDist) {
	board->expendCandArea(pos, MAX(lineDist, fillDist));
	int x = CoordX(pos), y = CoordY(pos);
	Pos p;
	for (int xi = -fillDist; xi <= fillDist; xi++) {
		for (int yi = -fillDist; yi <= fillDist; yi++) {
			if (xi == 0 && yi == 0) continue;
			p = POS(x + xi, y + yi);
			if (board->isEmpty(p) && cell(p).cand == 0) {
				cell(p).cand++;
			}
		}
	}
	p = POS(x, y);
	for (int i = MAX(3, fillDist + 1); i <= lineDist; i++) {
		for (int k = 0; k < 8; k++)
			cell(p + RANGE_NEIGHBOR[k] * i).cand++;
	}
}

void Evaluator::clearLose() {
	FOR_EVERY_POSITION_POS(p) {
		cell(p).isLose = false;
	}
}

Pos Evaluator::getHighestScoreCandPos() {
	int highestScore = INT32_MIN;
	Pos hp = NullPos;
	FOR_EVERY_CAND_POS(p) {
		int score = cell(p).getScore();
		if (score > highestScore) {
			hp = p;
			highestScore = score;
		}
	}
	return hp;
}
// 开局库查找
// A Function From Pela
Pos Evaluator::databaseMove() {
	const int MinDistFromBoard = 5;
	char *s, *sn;
	int i, x, y, x1, y1, flip, len1, len2, left, top, right, bottom;

	// board rectangle
	if (board->isNearBoard(POS(board->candArea().x0, board->candArea().y0), 2) ||
		board->isNearBoard(POS(board->candArea().x1, board->candArea().y1), 2))
		return NullPos;
	left = board->candArea().x0 + 2;
	top = board->candArea().y0 + 2;
	right = board->candArea().x1 - 2;
	bottom = board->candArea().y1 - 2;
	// find current board in the database
	for (s = MoveDatabase; ; s = sn) {
		len1 = *s++;
		len2 = *s++;
		sn = s + 2 * (len1 + len2);
		if (len1 != board->getMoveCount()) {
			// data must be sorted by moveCount descending
			if (len1 < board->getMoveCount()) 
				return NullPos; 
			continue;
		}
		// try all symmetries
		for (flip = 0; flip < 8; flip++) {
			for (i = 0;; i++) {
				x1 = s[2 * i];
				y1 = s[2 * i + 1];
				if (i == len1) {
					std::uniform_int_distribution<> dis(0, len2 - 1);
					s += 2 * (len1 + dis(random));
					x1 = *s++;
					y1 = *s;
				}
				switch (flip) {
				case 0: x = left + x1; y = top + y1; break;
				case 1: x = right - x1; y = top + y1; break;
				case 2: x = left + x1; y = bottom - y1; break;
				case 3: x = right - x1; y = bottom - y1; break;
				case 4: x = left + y1; y = top + x1; break;
				case 5: x = right - y1; y = top + x1; break;
				case 6: x = left + y1; y = bottom - x1; break;
				default: x = right - y1; y = bottom - x1; break;
				}
				if (board->isNearBoard(POS(x, y), MinDistFromBoard)) break;
				if (i == len1) return POS(x, y);
				// compare current board and database
				if (board->get(POS(x, y)) != (i & 1)) break;
			}
		}
	}
	return NullPos;
}

void Evaluator::trace(ostream & ss, const string & appendBefore) {
#define SET_COLOR(CCODE) SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), CCODE)
	int bg = 0;
	ss << appendBefore;
	if (board->getMoveCount() > 0) ss << "LastPos:" << PosStr(board->getLastMove()) << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		if (board->isEmpty(POS(x, y))) {
			if (cell(x, y).isLose) {
				ss << "L";
			} else {
				if (cell(x, y).isCand()) ss << '*';
				else ss << '.';
			}
		} else {
			if (board->getLastMove() == POS(x, y))
				bg = BACKGROUND_INTENSITY;
			switch (board->get(POS(x, y))) {
			case Black: SET_COLOR(bg | FOREGROUND_RED); ss << 'X'; break;
			case White: SET_COLOR(bg | FOREGROUND_GREEN); ss << 'O'; break;
			case Empty: SET_COLOR(bg | 7); ss << '.'; break;
			default: break;
			}
			bg = 0;
			SET_COLOR(bg | 7);
		}
		if (y == board->size() - 1) ss << " " << x + 1;
	}
	ss << endl << appendBefore;
	for (int y = 0; y < board->size(); y++) {
		ss << char(y + 65) << " ";
	}
	ss << endl << appendBefore << "---Score-------------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (y == 0 && x != 0) ss << endl << appendBefore;
		if (board->isEmpty(POS(x, y))) {
			if (cell(x, y).isCand()) ss << std::setw(5) << cell(x, y).getScore();
			else ss << "    .";
		} else {
			ss << "    ";
			switch (board->get(POS(x, y))) {
			case Black: SET_COLOR(FOREGROUND_RED); ss << 'X'; break;
			case White: SET_COLOR(FOREGROUND_GREEN); ss << 'O'; break;
			case Empty: SET_COLOR(7); ss << '.'; break;
			default: break;
			}
			SET_COLOR(7);
		}
	}
	ss << endl << appendBefore << "---Pattern4--Black------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		switch (board->get(POS(x, y))) {
		case Black: SET_COLOR(FOREGROUND_RED); break;
		case White: SET_COLOR(FOREGROUND_GREEN); break;
		case Empty: SET_COLOR(7); break;
		default: break;
		}
		if (board->isEmpty(POS(x, y))) {
			switch (cell(x, y).pattern4[Black]) {
			case A_FIVE: ss << 'A'; break;
			case B_FLEX4: ss << 'B'; break;
			case C_BLOCK4_FLEX3: ss << 'C'; break;
			case D_BLOCK4_PLUS: ss << 'D'; break;
			case E_BLOCK4: ss << 'E'; break;
			case F_FLEX3_2X: ss << 'F'; break;
			case G_FLEX3_PLUS: ss << 'G'; break;
			case H_FLEX3: ss << 'H'; break;
			case I_BLOCK3_PLUS: ss << 'I'; break;
			default: ss << '.'; break;
			}
		} else
			ss << '.';
		SET_COLOR(7);
	}
	ss << endl << appendBefore << "---Pattern4--White------" << endl << appendBefore;
	FOR_EVERY_POSITION(x, y) {
		if (x != 0 || y != 0) ss << " ";
		if (y == 0 && x != 0) ss << endl << appendBefore;
		switch (board->get(POS(x, y))) {
		case Black: SET_COLOR(FOREGROUND_RED); break;
		case White: SET_COLOR(FOREGROUND_GREEN); break;
		case Empty: SET_COLOR(7); break;
		default: break;
		}
		if (board->isEmpty(POS(x, y))) {
			switch (cell(x, y).pattern4[White]) {
			case A_FIVE: ss << 'A'; break;
			case B_FLEX4: ss << 'B'; break;
			case C_BLOCK4_FLEX3: ss << 'C'; break;
			case D_BLOCK4_PLUS: ss << 'D'; break;
			case E_BLOCK4: ss << 'E'; break;
			case F_FLEX3_2X: ss << 'F'; break;
			case G_FLEX3_PLUS: ss << 'G'; break;
			case H_FLEX3: ss << 'H'; break;
			case I_BLOCK3_PLUS: ss << 'I'; break;
			default: ss << '.'; break;
			}
		} else
			ss << '.';
		SET_COLOR(7);
	}
	ss << endl << appendBefore;
	ss << "===============================" << endl;
}

