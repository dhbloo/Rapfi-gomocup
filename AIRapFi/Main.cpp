#include "Search.h"
#include <sstream>
#include <windows.h>
#ifdef VERSION_YIXIN_BOARD
#include <chrono>
#include <thread>
#include <mutex>
#endif

using namespace std;

#ifdef VERSION_YIXIN_BOARD
mutex mtx;
#endif
bool thinking = false;
string configPath;

string getDefaultConfigPath() {
	char exeFullPath[MAX_PATH]; // Full path
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	string strPath(exeFullPath);    // Get full path of the file
	size_t pos = strPath.find_last_of('\\', strPath.length());
	string path = strPath.substr(0, pos);  // Return the directory without the file name
	return path + "\\config";
}

void thinkMove(AI * ai) {
	thinking = true;
	if (ai->shouldReloadConfig())
		ai->tryReadConfig(configPath);
	Pos best = ai->turnMove();
	ai->makeMove(best);
	cout << int(CoordX(best)) << "," << int(CoordY(best)) << endl;
	thinking = false;
}

void pipeLoop() {
	Board * board = nullptr;
	AI * ai = nullptr;
	string command;
	Pos input;
	char dot;
	int x, y;
	int boardSize = 15;

	while (true) {
		cin >> command;
		toupper(command);

	#ifdef VERSION_YIXIN_BOARD
		if (command == "YXSTOP") { // Yixin-Board Extension
			ai->stopThinking();
		}

		while (thinking) {
			this_thread::yield();
		}

		mtx.lock();
	#endif

		if (command == "START") {
			cin >> boardSize;
			if (boardSize <= 5) {
				cout << "ERROR" << endl;
			} else {
				if (!board || boardSize != board->size()) {
					if (ai) delete ai;
					if (board) delete board;
					board = new Board(boardSize);
					ai = new AI(board);
					ai->tryReadConfig(configPath);
				} else {
					ai->newGame();
				}
				cout << "OK" << endl;
			}
		} else if (command == "END") {
			exit(0);
		} else if (command == "INFO") {
			int value;
			long valueL;
			string key;
			cin >> key;
			toupper(key);

			if (key == "TIMEOUT_TURN") {
				cin >> valueL;
				if (valueL > 0) ai->info.timeout_turn = valueL;
			} else if (key == "TIMEOUT_MATCH") {
				cin >> valueL;
				if (valueL > 0) ai->info.timeout_match = valueL;
			} else if (key == "TIME_LEFT") {
				cin >> valueL;
				if (valueL > 0) ai->info.time_left = valueL;
			} else if (key == "MAX_MEMORY") {
				cin >> valueL;
				if (valueL >= 0) ai->info.setMaxMemory(valueL);
			} else if (key == "GAME_TYPE") {
				cin >> value; // TODO
			} else if (key == "RULE") {
				cin >> value; // TODO
			} else if (key == "FOLDER") {
				string t;
				cin >> t;
			} else if (key == "MAX_DEPTH") { // Yixin-Board Extension
				cin >> value;
				ai->setMaxDepth(value);
			}
		} else if (command == "ABOUT") {
			cout << "name=\"Rapfi\", version=\"2018.02\", author=\"Haobin Duan\", country=\"China\"" << endl;
		} else if (command == "YXHASHCLEAR") { // Yixin-Board Extension
			ai->clearHash();
		} else if (command == "RESTART") {
			ai->newGame();
			cout << "OK" << endl;
		} else if (command == "RECTSTART") {
			cin >> x >> dot >> y;
			cout << "ERROR Rectangular board is not supported.";

		} else if (!ai) {
			if (command == "TAKEBACK" || command == "BEGIN" || command == "TURN" || command == "BOARD" || command == "YXBOARD")
				cout << "ERROR No game has been started." << endl;
		} else if (command == "TAKEBACK") {
			cin >> x >> dot >> y;
			if (!board->isInBoard(POS(x, y)))
				cout << "ERROR Coord is outside the board." << endl;
			else if (board->isEmpty(POS(x, y)))
				cout << "ERROR Coord is empty." << endl;
			else {
				ai->undoMove();
				cout << "OK" << endl;
			}
		} else if (command == "BEGIN") {
			ai->makeMove(POS(boardSize / 2, boardSize / 2));
			cout << int(boardSize / 2) << "," << int(boardSize / 2) << endl;
		} else if (command == "TURN") {
			cin >> x >> dot >> y;
			input = POS(x, y);
			if (!board->isEmpty(input)) {
				cout << "ERROR Coord is not empty." << endl;
			} else {
				ai->makeMove(input);

			#ifdef VERSION_YIXIN_BOARD
				if (!thinking) {
					thread thinkThread(thinkMove, ai);
					thinkThread.detach();
				}
			#else
				thinkMove(ai);
			#endif
			}
		} else if (command == "BOARD") {
			int c;
			stringstream ss;
			ai->newGame();

			cin >> command;
			toupper(command);
			while (command != "DONE") {
				ss.clear();
				ss << command;
				ss >> x >> dot >> y >> dot >> c;
				input = POS(x, y);
				if (!board->isEmpty(input)) {
					cout << "ERROR Coord is not empty." << endl;
				} else {
					ai->makeMove(input);
				}
				cin >> command;
				toupper(command);
			}

		#ifdef VERSION_YIXIN_BOARD
			thread thinkThread(thinkMove, ai);
			thinkThread.detach();
		#else
			thinkMove(ai);
		#endif
		} else if (command == "YXBOARD") { // Yixin-Board Extension
			int c;
			stringstream ss;
			ai->newGame();

			cin >> command;
			toupper(command);
			while (command != "DONE") {
				ss.clear();
				ss << command;
				ss >> x >> dot >> y >> dot >> c;
				input = POS(x, y);
				if (!board->isEmpty(input)) {
					cout << "ERROR" << endl;
				} else {
					ai->makeMove(input);
				}
				cin >> command;
				toupper(command);
			}
		}
	#ifdef VERSION_YIXIN_BOARD
		mtx.unlock();
	#endif
	}
}

int main(int argc, char* argv[]) {
	bool customConfigPath = false;
	if (argc > 1) {
		char path[MAX_PATH];
		for (int i = 1; i < argc; i++)
			if (strncmp(argv[i], "config=", 7) == 0) {
				sscanf_s(argv[i], "config=%s", &path, MAX_PATH);
				configPath = path;
				customConfigPath = true;
				break;
			}
	}
	if (!customConfigPath)
		configPath = getDefaultConfigPath();

#ifdef _DEBUG

	Board board(15);
	long time = getTime();
	AI ai(&board);
	ai.tryReadConfig(configPath);
	cout << "Initial Time: " << getTime() - time << " ms" << endl;
	ai.makeMove(POS(board.centerPos(), board.centerPos()));
	ai.trace(cout);
	while (true) {
		Pos p;
		do {
			cout << "输入坐标： ";
			char x; int y;
			cin >> x;
			switch (toupper(x)) {
			case 'R':
				ai.undoMove();
				p = NullPos;
				ai.trace(cout);
				break;
			case 'S':
				p = ai.getHighestScoreCandPos();
				break;
			case 'T':
				p = ai.turnMove();
				break;
			default:
				cin >> y;
				p = POS(y - 1, toupper(x) - 65);
				break;
			}
		} while (!board.isEmpty(p));
		ai.makeMove(p);
		ai.trace(cout);
		if (board.checkWin()) {
			cout << board.getPlayerWon() << "胜利!" << endl;
			break;
		}
		ai.makeMove(ai.turnMove());
		ai.trace(cout);
		if (board.checkWin()) {
			cout << board.getPlayerWon() << "胜利!" << endl;
			break;
		}
	}
	system("pause");

#else
	
  #ifdef VERSION_YIXIN_BOARD
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
  #endif

	pipeLoop();

#endif
	return 0;
}