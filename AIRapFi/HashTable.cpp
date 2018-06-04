#include "HashTable.h"

HashTable::HashTable(int size) {
	hashSize = 1U << size;
	hashSizeMask = hashSize - 1;
	hashTable = new Cluster[hashSize];
}

HashTable::~HashTable() {
	delete[] hashTable;
}

void HashTable::clearHash() {
	for (UInt i = 0; i < hashSize; i++) hashTable[i].clear();
	generation = 0;
}

bool HashTable::probe(U64 key, TTEntry* & tte) {
	TTEntry * entry = hashTable[key & hashSizeMask].first_entry();
	UInt key32 = key >> 32;
	tte = nullptr;

	for (int i = 0; i < CLUSTER_SIZE; i++, entry++) {
		if (entry->_key32 == key32) {
			if ((entry->_genFlag & 0xFC) != generation)
				entry->saveGeneration(generation); // ¸üÐÂGeneration

			tte = entry;
			return true;
		} else if (entry->flag() == HashFlag::Hash_Unknown) {
			tte = entry;
			// From StockFish:
			// Due to our packed storage format for generation and its cyclic
			// nature we add 259 (256 is the modulus plus 3 to keep the lowest
			// two bound bits from affecting the result) to calculate the entry
			// age correctly even after generation8 overflows into the next cycle.
		} else if (!tte || tte->flag() != HashFlag::Hash_Unknown
			&&   tte->_depth - ((259 + generation -   tte->_genFlag) & 0xFC) * 2
	         > entry->_depth - ((259 + generation - entry->_genFlag) & 0xFC) * 2) {
			tte = entry;
		}
	}
	assert(tte);
	return false;
}