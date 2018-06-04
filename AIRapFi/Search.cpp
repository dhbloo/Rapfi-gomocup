#include "Search.h"
#include "HashTable.h"
#include <fstream>

AI::AI(Board * board) : Evaluator(board) {
	hashTable = new HashTable();
}

AI::~AI() {
	delete hashTable;
}

void AI::newGame() {
	Evaluator::newGame();
	clearHash();
}

void AI::clearHash() {
	hashTable->clearHash();
}

void AI::setMaxDepth(int depth) {
	maxSearchDepth = MAX(MIN(depth, 255), 2);
}

Pos AI::turnMove() {
	startTime = getTime();
	terminateAI = false;

	if (board->getMoveCount() == 0)
		return POS(board->centerPos(), board->centerPos());
	else if (board->getMoveCount() == 1) {
		Pos p = board->getLastMove();
		if (!board->isNearBoard(p, 5)) {
			Pos b;
			std::uniform_int_distribution<> dis(-1, 1);
			do {
				b = POS((CoordX(p)) + dis(random), CoordY(p) + dis(random));
			} while (!board->isEmpty(b));
			return b;
		} else if (board->isNearBoard(p, 1)) {
			expendCand(p, 3, 5);
		} else if (board->isNearBoard(p, 2)) {
			expendCand(p, 3, 4);
		}
	} else if (board->getMoveLeftCount() == 0)
		return NullPos;

	Pos best;
#ifndef VERSION_YIXIN_BOARD
	if (useOpeningBook) {
		best = databaseMove();
		if (board->isEmpty(best))
			return best;
	}
#endif

	node = nodeExpended = 0;
	ply = 0;
	maxPlyReached = 0;
	clearLose();

#ifdef VERSION_YIXIN_BOARD
	clearHash();
	MESSAGEL("Expect Time: " << timeForTurn() << "ms");
#else
	if (reloadConfig)
		clearHash();
#endif
	aiPiece = SELF;
	hashTable->newSearch();

	best = fullSearch();

	long time = timeUsed();
#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Node: " << node << " Speed: " << node / (time + 1) << "K");
	MESSAGEL("Time: " << time << " ms");
#else
	MESSAGEL("节点数: " << node << " NPS: " << node / (time + 1) << "K");
	MESSAGEL("节点展开数: " << nodeExpended << " NPS: " << nodeExpended / (time + 1) << "K");
	MESSAGEL("平均分支: " << (nodeExpended == 0 ? 0 : double(node) / nodeExpended));
	MESSAGEL("总耗时: " << time << " ms");
#endif
	return best;
}

Pos AI::fullSearch() {
	long lastDepthTime, turnTime;
	Move bestMove, move;
	int lastValue = -INF;
	int lastNode, lastNodeExpended;
	int searchDepth;
	int PVStableCount = 0;
	bool shouldBreak;

	turnTime = timeForTurn();

	moveLists[0].init_GenAllMoves();
	for (searchDepth = 2; searchDepth <= maxSearchDepth; searchDepth++) {
		lastDepthTime = getTime();
		lastNode = node;
		lastNodeExpended = nodeExpended;
		BestMoveChangeCount = 0;

		move = alphabeta_root(searchDepth, -INF, INF);
		if (!board->isEmpty(move.pos)) break;
		else if (move.value == -INF)  // 这个时候不知道这一步的真正估值，用上一步的代替
			move.value = bestMove.value; 
		bestMove = move;

		if (nodeExpended != lastNodeExpended) {
			if (BestMoveChangeCount == 0) {
				if (++PVStableCount >= BM_STABLE_MIN)
					turnTime = MAX(turnTime * TIME_DECREASE_PERCENTAGE / 100, timeForTurn() / TURNTIME_MIN_DIVISION);
			} else {
				PVStableCount = 0;
				if (searchDepth >= BM_CHANGE_MIN_DEPTH)
					turnTime = MIN(turnTime * TIME_INCRESE_PERCENTAGE / 100, timeForTurnMax());
			}
		}

		// 提前退出|超时退出|赢或输退出|平局退出
		shouldBreak = (timeLeft() / MATCH_SPARE_MAX < info.timeout_turn || timeUsed() > turnTime * TIMEOUT_PREVENT_MIN / 100) && (turnTime * 10 <= (getTime() - lastDepthTime) * TIMEOUT_PREVENT)
			|| terminateAI
			|| bestMove.value >= WIN_MIN || bestMove.value <= -WIN_MIN;

		// 有搜索节点(非提取置换表)才输出层信息
		if (nodeExpended != lastNodeExpended || shouldBreak)
		#ifdef VERSION_YIXIN_BOARD
			MESSAGEL("Depth:" << searchDepth << "-" << maxPlyReached << " Val:" << bestMove.value << " Time:" << getTime() - lastDepthTime << "ms Node:" << (node - lastNode) / 1000000 << "M " << YXPos(bestMove.pos, board->size()));
	#else
			MESSAGEL("深度: " << searchDepth << "-" << maxPlyReached << " 估值: " << bestMove.value << " 最佳: " << PosStr(bestMove.pos) << " 层耗时: " << getTime() - lastDepthTime << " ms");
	#endif
		if (shouldBreak) break;
		lastValue = bestMove.value;
	}

	if (!board->isEmpty(bestMove.pos))
		bestMove = Move(getHighestScoreCandPos(), 0);

#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Evaluation: " << bestMove.value << " | Best Point: " << YXPos(bestMove.pos, board->size()));
#else
	MESSAGEL("估值: " << bestMove.value << " | 最佳: " << PosStr(bestMove.pos));
#endif

	Line line;
	fetchPVLineInTT(line, bestMove.pos, searchDepth);
#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("BestLine: " << line.YXPrint(board->size()));
#else
	MESSAGEL("最佳路线: " << line);
#endif

	bool hasLose = false;
	FOR_EVERY_POSITION_POS(p) {
		if (cell(p).isLose) {
			if (!hasLose) {
				MESSAGES_BEGIN;
			#ifdef VERSION_YIXIN_BOARD
				MESSAGES("Lose Points: ");
			#else
				MESSAGES("必败点: ");
			#endif
				hasLose = true;
			}
		#ifdef VERSION_YIXIN_BOARD
			MESSAGES(" " << YXPos(p, board->size()));
		#else
			MESSAGES(" " << PosStr(p));
		#endif

		}
	}
	if (hasLose) MESSAGES_END;

	return bestMove.pos;
}

Move AI::alphabeta_root(int depth, int alpha, int beta) {
	node++;

	Move best;
	TTEntry * tte;	// 查找置换表
#ifdef Hash_Probe
	if (hashTable->probe(board->getZobristKey(), tte)) {
		if (tte->isValid(depth, alpha, beta, ply))
			return tte->bestMove(ply);
		else
			best.pos = tte->bestPos();
	}
#endif

	//生成根节点着法
	MoveList & moveList = moveLists[0];
	moveList.hashMove = best.pos;
	WinState state = genMove_Root(moveList);
	MoveList::MoveIterator move = moveList.begin();

	if (state == State_Win || state == State_Lose) {
		return *move;
	} else if (moveList.moveCount() == 1) {
		terminateAI = true;
		TTEntry * tte;
		best.value = hashTable->probe(board->getZobristKey(), tte) ? tte->value(ply) : evaluate();
		best.pos = move->pos;
		return best;
	}

	nodeExpended++;
	float newDepth = (float)depth - getDepthReduction();
	HashFlag hashFlag = Hash_Alpha;
	int value;
	int availableCount = 0;

	rawStaticEval[ply] = rawEvaluate();
	minEvalPly = depth;

	do {
		if (cell(move->pos).isLose) {
			DEBUGL("PVS评估" << PosStr(move->pos) << ": Lose");
			ANALYSIS("LOST", move->pos);
			continue;
		}

		lastSelfP4 = cell(move->pos).pattern4[SELF];
		lastOppoP4 = cell(move->pos).pattern4[OPPO];

		ANALYSIS("POS", move->pos);

		makeMove(move->pos);
		if (hashFlag == Hash_PV) {
			value = -alphabeta<NonPV>(newDepth, -(alpha + 1), -alpha, true);
			if (value > alpha && value < beta)
				value = -alphabeta<PV>(newDepth, -beta, -alpha, false);
		} else {
			value = -alphabeta<PV>(newDepth, -beta, -alpha, false);
		}
		undoMove();

		ANALYSIS("DONE", move->pos);

		if (terminateAI) {
			// 终止时,此未搜索完的分支作废
			if (availableCount == 0) { // 如果还未搜到可行选点
				if (best.value >= -WIN_MAX) { // 前面的选点都是必败,选择当前选点
					best.value = -INF;
					best.pos = move->pos;
				} else {  // 如果第一个选点都没有搜完,此层作废
					best.pos = NullPos;
				}
			}
			break;
		}

		DEBUGL("PVS评估" << PosStr(move->pos) << "  预估:" << move->value << "  最佳:" << value << " Best.value = " << best.value);

		if (value <= -WIN_MIN) {
			cell(move->pos).isLose = true;
			move->value = -value;
			ANALYSIS("LOST", move->pos);
		} else {
			availableCount++;
			move->value -= 100;
		}

		if (value > best.value) {
			best.value = value;
			best.pos = move->pos;
			ANALYSIS("BEST", best.pos);
			BestMoveChangeCount++;
			move->value = value + 1000;
			// 根节点的value不会比beta大
			if (value > alpha) {
				hashFlag = Hash_PV;
				alpha = value;
			}
		}

		if (availableCount > 0 && timeUsed() > timeForTurn() - TIME_RESERVED_PER_MOVE)
			terminateAI = true;

	} while (++move < moveList.end());

	if (availableCount <= 1) terminateAI = true;

#ifdef Hash_Record
	if (!terminateAI) {
		assert(board->isEmpty(best.pos));
		tte->save(board->getZobristKey(), best, depth, hashFlag, ply, hashTable->getGeneration());
	}
#endif
	ANALYSIS("REFRESH", NullPos);

	return best;
}

template <NodeType NT>
int AI::alphabeta(float depth, int alpha, int beta, bool cutNode) {
	const bool PvNode = NT == PV;
	assert(PvNode || (alpha == beta - 1));
	assert(!(PvNode && cutNode));

	node++;

	// Step 01. Mate Distance Purning
	alpha = MAX(-WIN_MAX + ply, alpha);
	beta = MIN(WIN_MAX - ply - 1, beta);
	if (alpha >= beta) return alpha;

	// Step 02. 提前胜负判断
	int staticEval = quickWinCheck();
	if (staticEval != 0) {
		updateMaxPlyReached(ply);
		return staticEval;
	}

	// Step 03. 平局判断
	if (board->getMoveLeftCount() <= 1) return 0;

	// Step 04. 局面静态估值
	staticEval = evaluate();
	const Piece self = SELF, oppo = OPPO;
	int oppo5 = p4Count[oppo][A_FIVE];          // 对方的冲四数
	int oppo4 = oppo5 + p4Count[oppo][B_FLEX4]; // 对方的冲四和活三数

	// Step 05. 叶子节点估值
	
	if (depth <= 0
		&& ply >= minEvalPly) {   // 是否达到最低层数
		updateMaxPlyReached(ply);

	#ifdef VCF_Leaf
		if (staticEval >= beta) {  // 为对方算杀
			if (oppo5 > 0) {  // 对方可能有VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = oppo;
				int mateValue = quickVCFSearch();
				if (mateValue <= -WIN_MIN) return mateValue;
			}
		} else {
			if (oppo5 == 0) {  // 我可以尝试VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = self;
				int mateValue = quickVCFSearch();
				if (mateValue >= WIN_MIN) return mateValue;
			} else if (staticEval >= alpha) {  // 对方可能有VCF
				VCFMaxPly = ply + MAX_VCF_PLY;
				attackerPiece = oppo;
				int mateValue = quickVCFSearch();
				if (mateValue <= -WIN_MIN) return mateValue;
			}
		}
	#endif
		return staticEval;
	}
	
	TTEntry * tte;
	Pos ttMove = NullPos;  // 置换表记录的着法
	bool ttHit;            // 是否命中置换表
	int ttValue;           // 置换表中保存的上一次搜索的估值
	bool pvExact;          // 是否为确定的PV节点
	// Step 06. 查找置换表(Transposition Table Probe)
#ifdef Hash_Probe
	if (ttHit = hashTable->probe(board->getZobristKey(), tte)) {
		if (ply > singularExtensionPly) {   // 单步延伸时不采用置换表截断
			int ttDepth = PvNode ? (int)roundf(depth) + 1 : (int)roundf(depth);
			ttValue = tte->value(ply);

			if (tte->isValid(ttDepth, alpha, beta, ply)) {
				return ttValue;
			} else {
				ttMove = tte->bestPos();
				// 置换表中保存的值比静态估值更精确
				if (tte->flag() == (ttValue > staticEval ? Hash_Beta : Hash_Alpha))
					staticEval = ttValue;
			}
		}
	}
#endif
	pvExact = PvNode ? isPvExact[ply - 1] && (isPvExact[ply] = ttHit && tte->flag() == Hash_PV)
		: (isPvExact[ply] = false);

	nodeExpended++;

	// Step 07. 超时判断(Time Control)
	static int cnt = 0;
	if (--cnt < 0) {
		cnt = 3000;
		if (timeUsed() > timeForTurnMax()) 
			terminateAI = true;
	}

	rawStaticEval[ply] = rawEvaluate();    // 保存每一层的原始估值

	// Singular Extension skip all early purning (跳过所有的早期剪枝)
	if (excludedMove) goto MoveLoops;

	// Step 08. Razoring (skipped when oppo4 > 0)
#ifdef Razoring
	if (!PvNode && depth < RazoringDepth) {
		if (staticEval + RazoringMargin[MAX((int)floorf(depth), 0)] < alpha) {
			return staticEval;
		}
	}
#endif

	// Step 09. Futility Purning : child node
#ifdef Futility_Pruning
	if (depth < FutilityDepth) {
		if (staticEval - FutilityMargin[MAX((int)floorf(depth), 0)] >= beta)
			return staticEval;
	}
#endif

	// Step 10. 内部迭代加深(IID)
#ifdef Internal_Iterative_Deepening
	if (PvNode && !board->isEmpty(ttMove) && depth >= IIDMinDepth && oppo4 == 0) {
		TTEntry * tteTemp;
		alphabeta<NT>(depth * (2.f / 3.f), -beta, -alpha, cutNode);
		if (hashTable->probe(board->getZobristKey(), tteTemp)) {
			ttMove = tteTemp->bestPos();
		}
		assert(board->isEmpty(ttMove));
	}
#endif

MoveLoops:

	// Step 11. 循环全部着法
	assert(ply >= 0 && ply < MAX_PLY - 1);
	MoveList & moveList = moveLists[excludedMove ? ply + 1 : ply];
	moveList.init(ttMove);

	HashFlag hashFlag = Hash_Alpha;
	Move best;
	Pos move;
	int value;
	int branch = 0, maxBranch = getMaxBranch(ply);
	float newDepth = depth - (depth < 0 ? MIN(1.f, getDepthReduction())
		                                : getDepthReduction()); // 计算深度递减
	assert(best.value >= SHRT_MIN);

	bool singularExtensionNode = depth >= 8 && oppo5 == 0 && !excludedMove && ttHit  // 不允许递归调用singular extension search
		&& tte->flag() == Hash_Beta && tte->depth() >= (int)roundf(depth) - 3;

	Pos last1 = board->getMoveBackWard(1);
	Pos last2 = board->getMoveBackWard(2);

	while (moveNext(moveList, move)) {
		if (move == excludedMove && ply == singularExtensionPly) continue;
		branch++;

		lastSelfP4 = cell(move).pattern4[self];
		lastOppoP4 = cell(move).pattern4[oppo];

		// Step 12. 启发式剪枝
		int distance1 = distance(move, last1);
		int distance2 = distance(move, last2);
		bool isNear1 = distance1 <= CONTINUES_NEIGHBOR || isInLine(move, last1) && distance1 <= CONTINUES_DISTANCE;
		bool isNear2 = distance2 <= CONTINUES_NEIGHBOR || isInLine(move, last2) && distance2 <= CONTINUES_DISTANCE;;
		bool noImportantP4 = lastSelfP4 == NONE && lastOppoP4 == NONE;
		// 分支数剪枝(可能会产生误杀)
		if (noImportantP4) {
			if (best.value > -WIN_MIN) {
				if (branch > maxBranch) continue;
			} else {
				if (branch > MAX(maxBranch, MAX_WINNING_CHECK_BRANCH)) {
					bool isNear3 = distance(move, board->getMoveBackWard(3)) <= CONTINUES_DISTANCE;
					if (!(isNear1 && isNear3))
						continue;
				}
			}

			// 保守剪枝
			if (!PvNode && best.value > -WIN_MIN) {
				// near-horizon
				if (ply >= minEvalPly - 2 && newDepth <= 1) {
					const int MinPreFrontierBranch = isNear1 ? (isNear2 ? 24 : 18) : 10;
					if (branch >= MinPreFrontierBranch) continue;
				}
			}
		}

		float moveDepth = newDepth;

		// Step 13. Singular extension search(单步延伸)
		// 当我有一步着法大于beta且其他着法在搜索窗口(alpha-s, beta-s)时均fail low,
		// 说明这个着法是单一的,需要延伸
	#ifdef Singular_Extension
		if (singularExtensionNode && move == ttMove) {
			int rBeta = MAX(ttValue - (int)ceilf(SEBetaMargin * depth), -WIN_MAX);
			float SEDepth = depth * 0.5f;
			int minEvalPlyTemp = minEvalPly;

			excludedMove = move;
			minEvalPly = 0;
			singularExtensionPly = ply;

			value = alphabeta<NonPV>(SEDepth, rBeta - 1, rBeta, cutNode);

			singularExtensionPly = -1;
			minEvalPly = minEvalPlyTemp;
			excludedMove = NullPos;

			if (value < rBeta)
				moveDepth += 1.0f;
		}
	#endif

		// Step 14. 下出着法(Make move)
		makeMove(move);

		bool doFullDepthSearch = !PvNode || branch > 1;

		// Step 15. LMR(Late Move Reduction)
	#ifdef Late_Move_Reduction
		const int LMR_MinBranch = PvNode ? 30 : 20;
		if (depth >= 3 && oppo4 == 0 && branch >= LMR_MinBranch) {
			float reduction = 0.f;
			reduction = (branch - LMR_MinBranch) * 0.5f;
			
			if (pvExact) 
				reduction -= 1;

			if (selfP4 >= H_FLEX3)
				reduction += 1;

			if (oppoP4 >= H_FLEX3)
				reduction -= 1;
			
			if (cutNode)
				reduction += 2;

			reduction = MIN(reduction, moveDepth - newDepth * 0.4f);
			
			if (reduction > 0) {
				value = -alphabeta<false>(moveDepth - reduction, -(alpha + 1), -alpha, !cutNode);

				if (value <= alpha)
					doFullDepthSearch = false;
			}
		}
	#endif

		// Step 16. 完全搜索 (Full depth search when no cut exist and LMR failed)
		if (doFullDepthSearch) {
			value = -alphabeta<NonPV>(moveDepth, -(alpha + 1), -alpha, !cutNode);
		}

		// Step 17. PV node full search.
		if (PvNode && (branch <= 1 || (value > alpha && value < beta))) {
			value = -alphabeta<PV>(moveDepth, -beta, -alpha, false);
		}

		// Step 18. 撤回着法(Undo move)
		undoMove();

		// Step 19. 提前终止搜索(终止时，此未搜索完的分支作废)
		if (terminateAI) break;

		// Step 20. 更新最佳着法
		if (value > best.value) {
			best.value = value;
			best.pos = move;
			if (value >= beta) {
				hashFlag = Hash_Beta;
				break;
			} else if (value > alpha) {
				hashFlag = Hash_PV;
				alpha = value;
			}
		}
	}

	assert(terminateAI || best.value >= -WIN_MAX && best.value <= WIN_MAX);

	// Step 21. 置换表保存(Transposition Table Record)
	// 提前终止或Singular Extension时跳过
#ifdef Hash_Record
	if (!terminateAI && !excludedMove)
		tte->save(board->getZobristKey(), best, (int)roundf(depth), hashFlag, ply, hashTable->getGeneration());
#endif
	return best.value;
}

template int AI::alphabeta<PV>(float depth, int alpha, int beta, bool cutNode);
template int AI::alphabeta<NonPV>(float depth, int alpha, int beta, bool cutNode);

// 返回 0 如果没有找到 VCF
template <bool Root>
int AI::quickVCFSearch() {
	assert(attackerPiece == Black || attackerPiece == White);
	node++;

	const Piece self = SELF, oppo = OPPO;
	int value;
	TTEntry * tte;

	if (Root) {
	#ifdef Hash_Probe
		if (hashTable->probe(board->getZobristKey(), tte)) {
			if (tte->isMate())
				return tte->value(ply);
		}
	#endif
	}

	// 提前胜负判断
	value = quickWinCheck();
	if (value != 0) {
		updateMaxPlyReached(ply);
		return value;
	}

	// VCF最大深度限制
	if (ply > VCFMaxPly) {
		updateMaxPlyReached(ply);
		return 0;
	}

	if (p4Count[oppo][A_FIVE] > 0) {  // VCF节点判断上一手是否是冲四
		Pos move = getCostPosAgainstB4(board->getLastMove(), oppo);
		if (self == attackerPiece) {  // 如果我是攻击方,攻击不了
			if (cell(move).pattern4[self] < E_BLOCK4) {  // 如果堵对方的冲四的棋不是我的冲四
				updateMaxPlyReached(ply);
				return 0;
			}
		}

		makeMove<VC>(move);
		value = -quickVCFSearch<Root>();
		undoMove<VC>();

		return value;
	}

	assert(p4Count[oppo][A_FIVE] == 0);
	assert(self == attackerPiece);

	// 攻击方检查是否还有VCF着法
	if (p4Count[self][C_BLOCK4_FLEX3] == 0 && p4Count[self][D_BLOCK4_PLUS] == 0 && p4Count[self][E_BLOCK4] == 0) {
		updateMaxPlyReached(ply);
		return 0;
	}

	// 超时判断
	static int cnt = 0;
	if (--cnt < 0) {
		cnt = 7000;
		if (timeUsed() > timeForTurnMax()) 
			return terminateAI = true, 0;
	}

	nodeExpended++;

	const GenLevel Level = Root ? InFullBoard : InLine;
	assert(ply >= 0 && ply < MAX_PLY);
	MoveList & moveList = moveLists[ply];
	moveList.init_GenAllMoves();
	if (Root)        // 根据生成层级生成VCF着法
		genMoves_VCF(moveList);
	else
		genContinueMoves_VCF(moveList, RANGE_LINE_4, 32);

	if (moveList.moveCount() == 0)  // 判断是否有连贯的进攻着法
		return 0;

	sort(moveList.begin(), moveList.end(), std::greater<Move>());  // VCF着法排序

	Move best;
	Pos attMove, defMove;
	assert(moveList.n == 0);

	do {
		attMove = moveList.moves[moveList.n].pos;
		
		makeMove<VC>(attMove);

		defMove = getCostPosAgainstB4(attMove, self);
		assert(cell(defMove).pattern4[self] == A_FIVE);
		assert(p4Count[self][A_FIVE] > 1 || defMove == findPosByPattern4(self, A_FIVE));

		makeMove<VC>(defMove);
		value = quickVCFSearch<false>();  // 两次move后不反号
		undoMove<VC>();

		undoMove<VC>();

		if (value > best.value) {
			best.value = value;
			best.pos = attMove;
			if (value >= WIN_MIN) break;
		}
		if (terminateAI) break;

	#ifdef VCF_Branch_Limit
		if (moveList.n >= MAX_VCF_BRANCH - 1) break;
	#endif
	} while (++moveList.n < moveList.moveCount());

	if (best.value <= -WIN_MIN)  // 因为攻击方限制了只走VCF着法，即使判输也不是真正的输
		best.value = 0;

#ifdef Hash_Record
	if (Root) {
		if (!terminateAI && best.value >= WIN_MIN) {
			tte->save(board->getZobristKey(), best, 0, HashFlag::Hash_PV, ply, hashTable->getGeneration());
		}
	}
#endif

	return best.value;
}

template int AI::quickVCFSearch<true>();
template int AI::quickVCFSearch<false>();

WinState AI::genMove_Root(MoveList & moveList) {
	switch (moveList.phase) {
	case MoveList::AllMoves:
		if (moveList.moveCount() > 0) {
			// 根节点的排序要保留原始顺序
			stable_sort(moveList.begin(), moveList.end(), std::greater<Move>());
			return State_Unknown;
		} else
			moveList.phase = MoveList::GenAllMoves;
	case MoveList::GenAllMoves:
	{
		Piece self = SELF, oppo = OPPO;
		if (p4Count[self][A_FIVE] > 0) {
			moveList.addMove(findPosByPattern4(self, A_FIVE), WIN_MAX);
			return State_Win;
		} else if (p4Count[oppo][A_FIVE] >= 2) {
			moveList.addMove(findPosByPattern4(oppo, A_FIVE), -WIN_MAX + 1);
			return State_Lose;
		} else if (p4Count[oppo][A_FIVE] == 1) {
			moveList.addMove(findPosByPattern4(oppo, A_FIVE), 0);
			return State_Unknown;
		} else {
			if (p4Count[self][B_FLEX4] > 0) {
				moveList.addMove(findPosByPattern4(self, B_FLEX4), WIN_MAX - 2);
				return State_Win;
			} else if (p4Count[oppo][B_FLEX4] > 0) {
				genMoves_defence(moveList);
			} else {
				genMoves(moveList);
			}
		}
		// 将HashMove提到最前
		if (board->isEmpty(moveList.hashMove)) {
			for (MoveList::MoveIterator move = moveList.begin(); move != moveList.end(); move++)
				if (move->pos == moveList.hashMove) {
					move->value += 10000;
					break;
				}
		}
		assert(moveList.moveCount() > 0);
		sort(moveList.begin(), moveList.end(), std::greater<Move>());
		moveList.phase = MoveList::AllMoves;
		return State_Unknown;
	}
	default: 
		assert(false);
		return State_Unknown;
	}
}

bool AI::moveNext(MoveList & moveList, Pos & pos) {
	switch (moveList.phase) {
	case MoveList::HashMove:
		moveList.phase = MoveList::GenAllMoves;
		if (board->isEmpty(moveList.hashMove)) {
			pos = moveList.hashMove;
			return true;
		}
	case MoveList::GenAllMoves:
	{
		moveList.phase = MoveList::AllMoves;
		if (p4Count[OPPO][A_FIVE] > 0) {
			pos = findPosByPattern4(OPPO, A_FIVE);
		} else {
			if (p4Count[OPPO][B_FLEX4] > 0) {
				genMoves_defence(moveList);
			} else {
				genMoves(moveList);
			}
			assert(moveList.moveCount() > 0);
			sort(moveList.begin(), moveList.end(), std::greater<Move>());
			pos = moveList.moves.front().pos;
		}
		return true;
	}
	case MoveList::AllMoves:
		if (++moveList.n >= moveList.moveCount())
			return false;
		pos = moveList.moves[moveList.n].pos;
		return true;
	default: assert(false); return false;
	}
}

// 生成全部着法
void AI::genMoves(MoveList & moveList) {
	Piece self = SELF;
	int score;

	FOR_EVERY_CAND_POS(p) {
		score = cell(p).getScore(self);
		moveList.addMove(p, score);
	}
}
// 生成全部防御活三的着法（己方活四冲四，对方活四和冲四）
void AI::genMoves_defence(MoveList & moveList) {
	Piece self = SELF, oppo = OPPO;
	static set<Pos> defence;
	defence.clear();
	assert(p4Count[OPPO][B_FLEX4] > 0);

	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[oppo] == B_FLEX4) {
			getAllCostPosAgainstF3(p, oppo, defence);
		} else if (cell(p).pattern4[self] >= E_BLOCK4) {
			moveList.addMove(p, cell(p).getScore(self));
		}
	}

	set<Pos>::iterator it2 = defence.end();
	for (set<Pos>::iterator it1 = defence.begin(); it1 != it2; it1++) {
		moveList.addMove(*it1, cell(*it1).getScore(oppo) + 10000);
	}
	assert(moveList.moveCount() > 0);
}
// 只生成连续/区域内的冲四着法
void AI::genMoves_VCF(MoveList & moveList) {
	Piece self = SELF;

	FOR_EVERY_CAND_POS(p) {
		if (cell(p).pattern4[self] >= E_BLOCK4) {
			moveList.addMove(p, cell(p).getScore_VC(self));
		}
	}
}
// 生成连续冲四进攻着法
void AI::genContinueMoves_VCF(MoveList & moveList, const short * range, int n) {
	Piece self = SELF;
	Pos last = board->getMoveBackWard(2), p;

	if (last == NullPos) return;

	for (int i = 0; i < n; i++) {
		p = last + range[i];

		if (board->isEmpty(p)) {
			if (cell(p).pattern4[self] >= E_BLOCK4) {
				moveList.addMove(p, cell(p).getScore_VC(self));
			}
		}
	}
}

inline int AI::evaluate() {
	assert(p4Count[SELF][A_FIVE] == 0);

	// 全局评估
	int value = eval[SELF] - eval[OPPO];

	value = (value - rawStaticEval[ply - 1]) / 2;   // 上一层分数的符号相对这一层是负的

	return value;
}

inline int AI::rawEvaluate() {
	return eval[SELF] - eval[OPPO];
}

int AI::quickWinCheck() {
	Piece self = SELF, oppo = OPPO;
#ifdef Win_Check_FLEX3_2X
	bool has_FLEX3_2X = lastSelfP4 == F_FLEX3_2X;
#else
	bool has_FLEX3_2X = false;
#endif
	has_FLEX3_2X = false;

	if (p4Count[self][A_FIVE] >= 1) return WIN_MAX - ply;
	if (p4Count[oppo][A_FIVE] >= 2) return -WIN_MAX + ply + 1;
	if (p4Count[oppo][A_FIVE] == 1) return 0;
	if (p4Count[self][B_FLEX4] >= 1) return WIN_MAX - ply - 2;

	int self_C_count = p4Count[self][C_BLOCK4_FLEX3];
	if (self_C_count >= 1) {
		if (p4Count[oppo][B_FLEX4] == 0 && p4Count[oppo][C_BLOCK4_FLEX3] == 0 && p4Count[oppo][D_BLOCK4_PLUS] == 0 && p4Count[oppo][E_BLOCK4] == 0)
			return WIN_MAX - ply - 4;
		FOR_EVERY_CAND_POS(p) {    // 对43棋型反攻的快速验证(静态判断)
			if (cell(p).pattern4[self] == C_BLOCK4_FLEX3) {
				makeMove<VC>(p);
				Pos defMove = getCostPosAgainstB4(p, self);
				if (cell(defMove).pattern4[oppo] < E_BLOCK4) {
					undoMove<VC>();
					return WIN_MAX - ply - 4;
				}
				undoMove<VC>();
				if (--self_C_count <= 0) goto Check_Flex3;
			}
		}
	}
Check_Flex3:
	if (p4Count[self][F_FLEX3_2X] >= 1) {
		if (p4Count[oppo][B_FLEX4] == 0 && p4Count[oppo][C_BLOCK4_FLEX3] == 0 && p4Count[oppo][D_BLOCK4_PLUS] == 0 && p4Count[oppo][E_BLOCK4] == 0)
			return WIN_MAX - ply - 4;
	}

#ifdef Win_Check_FLEX3_2X
	if (has_FLEX3_2X) { // 对方有两个活三以上
		assert(p4Count[oppo][B_FLEX4] > 0);

		// 先检查下这个局面是不是已经判断过了
		TTEntry * tte;
		if (!hashTable->probe(board->getZobristKey(), tte)) { // 如果还没判断过
			int value = quickDefenceCheck();
			if (value <= -WIN_MIN) return value;
		}
	}
#endif
	return 0;
}

int AI::quickDefenceCheck() {
	Piece self = SELF, oppo = OPPO;
	int oppoB_count = p4Count[oppo][B_FLEX4];
	assert(oppoB_count > 0);
	size_t selfB4_count = 0;
	static vector<Pos> self_BLOCK4;

	while (p4Count[self][D_BLOCK4_PLUS] + p4Count[self][E_BLOCK4] > 0) { // 一直冲四直到没有可以冲四的选点
		self_BLOCK4.clear();
		FOR_EVERY_CAND_POS(p) {
			Pattern4 p4 = cell(p).pattern4[self];
			if (p4 >= E_BLOCK4 && p4 != B_FLEX4) {
				Cell & c = cell(p);
				int dir;
				for (dir = 0; dir < 4; dir++) {
					Pattern pattern = c.pattern[self][dir];
					if (pattern >= B4) {
						Pos pos;
						int i;
						if (pattern == F5) pos = p;
						else {
							if (cell(pos = p - D[dir]).pattern4[self] == A_FIVE);
							else if (cell(pos = p + D[dir]).pattern4[self] == A_FIVE);
							else break;
						}
						for (i = 1; i <= 7; i++) {
							pos -= D[dir];
							Piece piece = board->get(pos);
							if (piece != self) break;
						}
						if (i > 7) continue;
						for (i = 1; i <= 7; i++) {
							pos += D[dir];
							Piece piece = board->get(pos);
							if (piece != self) break;
						}
						if (i > 7) continue;
						break;
					}
				}
				if (dir < 4) self_BLOCK4.push_back(p);
			}
		}
		if (self_BLOCK4.size() == 0) break;
		for (size_t i = 0; i < self_BLOCK4.size(); i++)
			makeMove<MuiltVC>(self_BLOCK4[i]);
		selfB4_count += self_BLOCK4.size();
		if (p4Count[self][B_FLEX4] > 0 || p4Count[self][C_BLOCK4_FLEX3] > 0) {
			oppoB_count = 0;
			break;  // 我在VCF(伪)时自己有了活四
		}
	}
	if (oppoB_count > 0 && p4Count[oppo][B_FLEX4] == oppoB_count)
		oppoB_count = INF;

	for (size_t i = 0; i < selfB4_count; i++)
		undoMove<MuiltVC>();

	// 因为每个冲四会多算2步棋,保守估计步数除以2
	if (oppoB_count == INF)
		return -WIN_MAX + ply + 3 + (int)selfB4_count / 2;  
	return 0;
}

float AI::getDepthReduction() {
	Piece self = SELF, oppo = OPPO;

	int branchCount;

	if (p4Count[oppo][A_FIVE] == 1) {
		branchCount = 1;
	} else {
		int oppo_B_Count = p4Count[oppo][B_FLEX4];
		if (oppo_B_Count > 0) {
			branchCount = oppo_B_Count == 1 ? 3 : oppo_B_Count * 2;
		} else {
			branchCount = 0;
			FOR_EVERY_CAND_POS(p) {
				branchCount++;
			}
		}
	}
	assert(branchCount > 0);

	return logf((float)branchCount /*+ 1e-3f*/) * depthReductionBase;
}

void AI::fetchPVLineInTT(Line & line, Pos firstMove, int maxDepth) {
	if (maxDepth <= 0) return;
	line.pushMove(firstMove);
	makeMove(firstMove);
	TTEntry * tte;
	if (hashTable->probe(board->getZobristKey(), tte)) {
		Pos next = tte->bestPos();
		if (board->isEmpty(next))
			fetchPVLineInTT(line, next, maxDepth - 1);
	}
	undoMove();
}

void AI::tryReadConfig(string path) {
	std::ifstream file(path, std::ifstream::in);
	if (!file) return;
	const int LineSize = 1000;
	char line[LineSize];

	int override;
	file.getline(line, LineSize);
	sscanf_s(line, "Override:%d", &override);
	if (override != 1) {
		file.close();
		return;
	}
	override = 0;

	while (!file.eof()) {
		file.getline(line, LineSize);

		if (strncmp(line, "Eval:", 10) == 0) {
			for (int i = 0; i < 3876; i++)
				file >> Value[i];
			override++;
		} else if (strncmp(line, "Score:", 6) == 0) {
			for (int i = 0; i < 3876; i++)
				file >> Score[i];
			override++;
		} else if (strncmp(line, "ExtensionCoefficient:", 21) == 0) {
			int ExtensionNumBase;
			sscanf_s(line, "ExtensionCoefficient:%d", &ExtensionNumBase);
			depthReductionBase = ExtensionNumBase <= 1 ? 1e6f : 1.f / logf((float)ExtensionNumBase);
			override++;
		} else if (strncmp(line, "UseOpeningBook:", 15) == 0) {
			int opening;
			sscanf_s(line, "UseOpeningBook:%d", &opening);
			useOpeningBook = opening == 1;
			override++;
		} else if (strncmp(line, "FutilityPurningMargin:", 22) == 0) {
			FutilityDepth = sscanf_s(line, "FutilityPurningMargin:%d%d%d%d", FutilityMargin, FutilityMargin + 1, FutilityMargin + 2, FutilityMargin + 3);
			if (FutilityDepth > FUTILITY_MAX_DEPTH) FutilityDepth = FUTILITY_MAX_DEPTH;
			override++;
		} else if (strncmp(line, "RazoringMargin:", 15) == 0) {
			sscanf_s(line, "RazoringMargin:%d%d%d%d", RazoringMargin, RazoringMargin + 1, RazoringMargin + 2, RazoringMargin + 3);
			if (RazoringDepth > RAZORING_MAX_DEPTH) RazoringDepth = RAZORING_MAX_DEPTH;
			override++;
		} else if (strncmp(line, "IIDMinDepth:", 12) == 0) {
			sscanf_s(line, "IIDMinDepth:%d", &IIDMinDepth);
			override++;
		} else if (strncmp(line, "SEBetaMargin:", 13) == 0) {
			sscanf_s(line, "SEBetaMargin:%f", &SEBetaMargin);
			override++;
		} else if (strncmp(line, "ReloadConfigOnEachMove:", 23) == 0) {
			int reload;
			sscanf_s(line, "ReloadConfigOnEachMove:%d", &reload);
			reloadConfig = reload == 1;
			override++;
		}
	}
	file.close();

#ifdef VERSION_YIXIN_BOARD
	MESSAGEL("Custom config has been read, " << override << " properties changed.");
#else
	MESSAGEL("Custom config has been read, " << override << " properties changed.");
#endif
}
