#pragma once
#include "Search.h"
#include <climits>

enum HashFlag : UInt8 {
	Hash_Unknown = 0,
	Hash_Alpha = 1,
	Hash_Beta = 2,
	Hash_PV = 3
};

/*
	TTEntry Struct (10 bytes)
	key32        32 bit
	value        16 bit
	depth         8 bit
	generation    6 bit
	flag          2 bit
	best         16 bit
*/

#pragma pack(push, 2)
class TTEntry {
private:
	friend class HashTable;
	UInt _key32;
	short _value;
	Int8 _depth;
	UInt8 _genFlag;
	Pos _best = NullPos;

	inline void saveGeneration(UInt8 generation) { _genFlag = flag() | generation; }
	inline void clear() {
		_genFlag = HashFlag::Hash_Unknown;
		_depth = 0;
		_key32 = 0;
		_best = NullPos;
	}

public:
	inline int value(int ply) const { return _value >= AI::WIN_MIN ? _value - ply : (_value <= -AI::WIN_MIN ? _value + ply : _value); }
	inline int depth() const { return _depth; }
	inline Pos bestPos() const { return _best; }
	inline Move bestMove(int ply) const { return Move(bestPos(), value(ply)); }
	inline HashFlag flag() const { return static_cast<HashFlag>(_genFlag & 3); }
	inline UInt8 generation() const { return static_cast<UInt8>(_genFlag & 0xFC); }

	inline bool isMate() { return _value >= AI::WIN_MIN || _value <= -AI::WIN_MIN; }
	inline bool isValid(int depth, int alpha, int beta, int ply) {
		bool mate = false;
		int value = _value;
		if (value >= AI::WIN_MIN) {
			value -= ply;
			mate = true;
		} else if (value <= -AI::WIN_MIN) {
			value += ply;
			mate = true;
		}
		if (mate || _depth >= depth) {
			HashFlag f = flag();
			return f == HashFlag::Hash_PV
				|| f == HashFlag::Hash_Alpha && value <= alpha
				|| f == HashFlag::Hash_Beta  && value >= beta;
		}
		return false;
	}

	inline void save(U64 key, const Move & move, int depth, HashFlag flag, int ply, UInt8 gen) {
		int value = move.value;
		assert(value >= -AI::WIN_MAX && value <= AI::WIN_MAX);
		assert(depth >= SCHAR_MIN && depth <= SCHAR_MAX);

		UInt newKey = static_cast<UInt>(key >> 32);
		// 保留更重要的entry
		if (_key32 == key && depth < _depth) return;   // 深度小的不能覆盖深度大的

		if (value >= AI::WIN_MIN) value += ply;
		else if (value <= -AI::WIN_MIN) value -= ply;

		_key32 = newKey;
		_value = static_cast<short>(value);
		_depth = static_cast<Int8>(depth);
		_genFlag = flag | gen;
		_best = move.pos;
	}
};
#pragma pack(pop)

class HashTable {
private:
	static const int CACHE_LINE_SIZE = 64;
	static const int CLUSTER_SIZE = 3;
	static const int DEFAULT_HASH_SIZE = 23;
	UInt hashSize;
	UInt hashSizeMask;

	struct Cluster {
		TTEntry entry[CLUSTER_SIZE];
		UInt8 padding[2];    //补齐Cache line (32 bytes)

		inline TTEntry * first_entry() { return entry; }
		inline void clear() {
			for (int i = 0; i < CLUSTER_SIZE; i++) entry[i].clear();
		}
	};
	Cluster* hashTable;
	UInt8 generation; // Size must be not bigger than TTEntry::_genFlag

	static_assert(CACHE_LINE_SIZE % sizeof(Cluster) == 0, "Cluster size incorrect");

public:
	HashTable(int size = DEFAULT_HASH_SIZE);
	~HashTable();

	void clearHash();
	void newSearch() { generation += 4; } // Lower 2 bits are used by Bound

	inline UInt8 getGeneration() { return generation; }

	bool probe(U64 key, TTEntry* & tte);
};

