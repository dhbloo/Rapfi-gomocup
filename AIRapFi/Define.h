#pragma once

#define RapFi_Version "2018.02"
///#define VERSION_YIXIN_BOARD

#ifndef _DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <array>
#include <set>
#include <algorithm>
#include <iterator>
#include <functional>
#include <random>
#include <ctime>
#include <iostream>

using std::string;
using std::vector;
using std::array;
using std::ostream;
using std::list;
using std::set;
using std::cout;
using std::endl;
using std::swap;

typedef char Int8;
typedef unsigned char UInt8;
typedef unsigned short UShort;
typedef unsigned int UInt;
typedef unsigned long ULong;
typedef unsigned long long U64;

template<class T1, class T2>
inline auto _max(const T1 a, const T2 b) {
	return (a > b) ? a : b;
}

template<class T1, class T2>
inline auto _min(const T1 a, const T2 b) {
	return (a < b) ? a : b;
}

template<class T>
inline auto _abs(const T a) {
	return (a < 0) ? -a : a;
}

#define MAX(a,b) _max(a,b)
#define MIN(a,b) _min(a,b)
#define ABS(a) _abs(a)

static std::mt19937_64 random(time(NULL));

inline void toupper(string & str) {
	for (size_t i = 0; i < str.size(); i++) {
		char &c = str[i];
		if (c >= 'a' && c <= 'z') {
			c += 'A' - 'a';
		}
	}
}

// 返回当前时间(单位:ms)
inline long getTime() {
	return clock() * 1000L / CLOCKS_PER_SEC;
}

#ifdef _DEBUG
#define DEBUGL(message) cout << message << endl
#define MESSAGEL(message) cout << message << endl
#define MESSAGES_BEGIN 
#define MESSAGES(message) cout << message
#define MESSAGES_END cout << endl
#define ANALYSIS(type, pos) ((void) 0)
#else
#define DEBUGL(message)
#define MESSAGEL(message) cout << "MESSAGE " << message << endl
#define MESSAGES_BEGIN cout << "MESSAGE "
#define MESSAGES(message) cout << message
#define MESSAGES_END cout << endl

#ifdef VERSION_YIXIN_BOARD
#define ANALYSIS(type, pos) if (depth >= 5) cout << "MESSAGE REALTIME " << type << ' ' << (int)CoordX(pos) << ',' << (int)CoordY(pos) << endl
#else
#define ANALYSIS(type, pos) ((void) 0)
#endif
#endif
