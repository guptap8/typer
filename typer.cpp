#include "words.h"
#include <asm-generic/ioctls.h>
#include <atomic>
#include <bits/types/struct_timeval.h>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#define RGB_COLOR(r, g, b)                                                     \
	"\x1b[38;2;" + to_string(r) + ";" + to_string(g) + ";" +               \
	    to_string((b)) + "m"
#define WHITE_COLOR "\x1b[38;2;255;255;255m"
#define GREY_COLOR "\x1b[38;2;100;100;100m"
#define RED_COLOR "\x1b[31m"
#define GREEN_COLOR "\x1b[32m"
#define YELLOW_COLOR "\x1b[33m"
#define BLUE_COLOR "\x1b[34m"
#define MAGENTA_COLOR "\x1b[35m"
#define CYAN_COLOR "\x1b[36m"
#define RESET_COLOR "\x1b[0m"

#define HIDE_CURSOR "\033[?25l"
#define UNHIDE_CURSOR "\033[?25h"
#define REVERSE_VIDEO "\033[7m"
#define RESET_VIDEO "\033[0m"

#define BACKSPACE 127

#define CLEAR_SCREEN "\033[2J\033[1;1H"

using namespace std;

void startTheGame();

ofstream logs("./logs");

enum CursorDirection { UP, DOWN, RIGHT, LEFT };

int wordsSetIdx = -1;
vector<pair<vector<string>, vector<string>>> wordsSet = {};
int totalSeconds = 15;

string moveCursor(int n, CursorDirection direction)
{
	string result = "\033[" + to_string(n);
	switch (direction) {
	case UP:
		result += "A";
		break;
	case DOWN:
		result += "B";
		break;
	case RIGHT:
		result += "C";
		break;
	case LEFT:
		result += "D";
		break;
	}
	cout << result;
	return result;
}

atomic<bool> stopFlag{false};
atomic<bool> started{false};
atomic<int> currentTime{0};

void printTimer(int time)
{
	cout << RGB_COLOR(255, 255, 255);
	cout << "\r";
	moveCursor(1, UP);
	cout << time << "/" << totalSeconds;
	cout << "\r";
	cout << RESET_COLOR;
	cout.flush();
}

struct termios originalTermios;

void disableCanonicalMode()
{
	struct termios raw;
	tcgetattr(STDIN_FILENO, &originalTermios);
	raw = originalTermios;
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void restoreTerminalSettings()
{
	cout << UNHIDE_CURSOR << endl;
	tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
}

char get_key()
{
	char ch;
	read(STDIN_FILENO, &ch, 1);
	return ch;
}

bool isInputAvailable(struct pollfd *fds)
{
	int ret = poll(fds, 1, 5);

	bool res = false;

	if (ret == -1) {
		res = false;
	} else if (ret > 0) {
		if (fds[0].revents & POLLIN) {
			res = true;
		}
	}
	return res;
}

pair<string, string> getStateInputs(pair<vector<string>, vector<string>> &words)
{
	auto concat = [](vector<string> &list) -> string {
		string ret = "";
		for (int i = 0; i < list.size(); i++) {
			ret += list[i];
			if (i != list.size() - 1) {
				ret += " ";
			}
		}
		return ret;
	};
	return {concat(words.first), concat(words.second)};
}

pair<vector<string>, vector<string>> getPrevWordSet()
{
	wordsSetIdx--;
	if (wordsSetIdx < 0) {
		wordsSetIdx = 0;
	}
	return wordsSet[wordsSetIdx];
}

pair<vector<string>, vector<string>> getNextWordSet(string &state,
						    string &inputs)
{
	vector<string> inputsParsed;
	string temp = "";
	for (int i = 0; i < inputs.size(); i++) {
		if (inputs[i] == ' ') {
			inputsParsed.push_back(temp);
			temp = "";
			continue;
		}
		temp += inputs[i];
	}
	if (temp != "") {
		inputsParsed.push_back(temp);
	}
	if (!wordsSet.empty()) {
		wordsSet[wordsSetIdx].second = inputsParsed;
	}
	if (wordsSetIdx < wordsSet.size() - 1) {
		wordsSetIdx++;
		auto temp = wordsSet[wordsSetIdx];
		return temp;
	}
	vector<string> nextWords;
	for (int i = 0; i < 10; i++) {
		int randNum = rand() % 1000;
		nextWords.push_back(dictionary[randNum]);
	};
	wordsSet.push_back({nextWords, inputsParsed});
	wordsSetIdx++;
	return {nextWords, {}};
}

void updateStateToPrev(string &state, string &inputs)
{
	pair<vector<string>, vector<string>> prevWords = getPrevWordSet();
	pair<string, string> stateInputs = getStateInputs(prevWords);
	state = stateInputs.first;
	inputs = stateInputs.second;
}

void updateStateToNext(string &state, string &inputs, int &spacesCount, int &wc)
{
	pair<vector<string>, vector<string>> words =
	    getNextWordSet(state, inputs);
	wc = words.first.size();
	state = "";
	for (int i = 0; i < wc; i++) {
		state += words.first[i];
		if (i != wc - 1) {
			state += " ";
		}
	}
	inputs = "";
	spacesCount = 0;
	if (!inputs.empty())
		inputs.pop_back();
}

void updateState(string &state, string &inputs, char &ch, int &spacesCount,
		 int &wc)
{
	if ((ch > 'z' || ch < 'a') && (ch < 'A' || ch > 'Z') &&
	    ch != BACKSPACE && ch != ' ') {
		return;
	}
	if (ch == BACKSPACE) {
		if (!inputs.empty()) {
			if (inputs.back() == ' ') {
				spacesCount--;
			}
			inputs.pop_back();
		} else {
			updateStateToPrev(state, inputs);
			spacesCount = 9;
		}
		return;
	} else if (ch != BACKSPACE) {
		inputs.push_back(ch);
	}
	if (ch == ' ') {
		spacesCount++;
	}
	if (spacesCount >= wc) {
		updateStateToNext(state, inputs, spacesCount, wc);
	}
}

void printGame(string &state, string &inputs, char &ch, int &spacesCount,
	       int &wc)
{
	cout << CLEAR_SCREEN;
	printTimer(currentTime);
	moveCursor(1, DOWN);
	int i, j;
	for (i = 0, j = 0; i < inputs.size() && j < state.size();) {
		if (state[j] == ' ' && inputs[i] != ' ') {
			cout << RED_COLOR;
			cout << inputs[i];
			i++;
		} else if (inputs[i] == ' ' && state[j] != ' ') {
			while (j < state.size() && state[j] != ' ') {
				cout << GREY_COLOR;
				cout << state[j];
				j++;
			}
			cout << inputs[i];
			i++;
			j++;
		} else {
			if (state[j] == inputs[i]) {
				cout << GREEN_COLOR;
			} else if (state[j] != inputs[i]) {
				cout << RED_COLOR;
			}
			cout << state[j];
			i++;
			j++;
		}
	}
	if (i >= inputs.size() && j >= state.size()) {
		cout << WHITE_COLOR;
		cout << REVERSE_VIDEO;
		cout << ' ';
		cout << RESET_VIDEO;
		cout << GREY_COLOR;
	}
	if (j < state.size()) {
		cout << WHITE_COLOR;
		cout << REVERSE_VIDEO;
		cout << state[j];
		cout << RESET_VIDEO;
		cout << GREY_COLOR;
		j++;
		for (; j < state.size(); j++) {
			cout << state[j];
		}
	}
	if (i < inputs.size()) {
		cout << RED_COLOR;
		cout << inputs[i];
		i++;
		for (; i < inputs.size(); i++) {
			cout << inputs[i];
		}
		cout << WHITE_COLOR;
		cout << REVERSE_VIDEO;
		cout << ' ';
		cout << RESET_VIDEO;
	}

	// Flush output.
	cout.flush();
}

void updateAndPrintGame(string &state, string &inputs, char &ch,
			int &spacesCount, int &wc)
{
	updateState(state, inputs, ch, spacesCount, wc);
	printGame(state, inputs, ch, spacesCount, wc);
}

void runGame()
{
	string state = "";
	string inputs = "";
	pair<vector<string>, vector<string>> words =
	    getNextWordSet(state, inputs);
	int wc = words.first.size();
	int spacesCount = 0;
	int wordsSize = words.first.size();
	for (int i = 0; i < wordsSize; i++) {
		state += words.first[i] + ((i == wordsSize - 1) ? "" : " ");
	}
	cin.tie(NULL);
	char ch;

	cout << CLEAR_SCREEN;
	printTimer(currentTime);
	cout << GREY_COLOR;
	moveCursor(1, DOWN);
	for (int i = 0; i < state.size(); i++) {
		if (i == 0) {
			cout << WHITE_COLOR;
			cout << REVERSE_VIDEO;
		}
		cout << state[i];
		if (i == 0) {
			cout << RESET_VIDEO;
			cout << GREY_COLOR;
		}
	}
	cout << "\r";
	cout << RESET_COLOR;
	cout.flush();

	struct pollfd fds[1];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	while (!stopFlag) {
		if (isInputAvailable(fds)) {
			ch = get_key();
			if (!started) {
				started = true;
			}
			updateAndPrintGame(state, inputs, ch, spacesCount, wc);
		}
	}
	if (wordsSetIdx == wordsSet.size() - 1) {
		getNextWordSet(state, inputs);
	}
}

void timer(int seconds)
{
	while (true) {
		if (started) {
			break;
		}
	}
	for (int i = seconds; i > 0 && !stopFlag; --i) {
		currentTime = seconds - i;
		printTimer(currentTime);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	stopFlag = true; // Signal worker to stop
}

void enterAlternateScreen()
{
	std::cout << "\033[?1049h"; // Switch to the alternate screen buffer
	cout << HIDE_CURSOR << endl;
	std::cout.flush();
}

void exitAlternateScreen()
{
	std::cout << "\033[?1049l"; // Return to the normal screen buffer
	std::cout.flush();
}

void signalHandler(int signal)
{
	restoreTerminalSettings();
	exitAlternateScreen();
	exit(signal);
}

void registerSignalHandlers()
{
	std::signal(SIGTSTP, signalHandler);
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGSEGV, signalHandler);
}

void printer()
{
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		return;
	}
	const int height = w.ws_row;
	const int width = w.ws_col;

	cout << "height of the terminal is " << to_string(height) << endl;
	cout << "width of the terminal is " << to_string(width) << endl;
}

void printHelp()
{
	cout << "Usage:\n\n";
	cout
	    << "--time <SECONDS>: Total number of seconds for the game timer. "
	       "\n"
	       "\tTakes required 1 argument, the total number of seconds the \n"
	       "\tgame should run for. By default, the game runs for 15 "
	       "seconds "
	       "\n"
	       "\tif this option is not specified.\n\n";
	cout << "--help: Print help.\n" << endl;
	exit(0);
}

void printInvalidOption()
{
	cout << "invalid text" << endl;
	exit(1);
}

void setTotalSeconds(char *arg)
{
	if (arg != 0) {
		try {
			totalSeconds = stoi(arg);
		} catch (...) {
			printInvalidOption();
		}
	}
}

void init(int argc, char *argv[])
{
	int opt;
	int option_index = 0;
	struct option long_options[] = {{"time", required_argument, NULL, 't'},
					{"help", no_argument, NULL, 'h'},
					{NULL, 0, NULL, 0}};

	while ((opt = getopt_long(argc, argv, "t:h", long_options,
				  &option_index)) != -1) {
		switch (opt) {
		case 't':
			setTotalSeconds(optarg);
			startTheGame();
			exit(0);
		case 'h':
			printHelp();
			break;
		case '?':
			exit(1);
		}
	}
}

void startTheGame()
{
	registerSignalHandlers();
	enterAlternateScreen();
	disableCanonicalMode();

	thread timerWorker(timer, totalSeconds);
	thread worker(runGame);
	worker.join();
	timerWorker.join();
	cout << CLEAR_SCREEN;
	int score = 0;
	for (auto i : wordsSet) {
		for (int j = 0; j < i.second.size(); j++) {
			if (i.first[j] == i.second[j]) {
				score++;
			}
		}
	}
	cout << WHITE_COLOR;
	cout << "YOUR SCORE IS: " << score << endl;
	cout << "GAME OVER" << endl;
	int endScreenTimeoutMS = 1000;
	this_thread::sleep_for(chrono::milliseconds(endScreenTimeoutMS));

	restoreTerminalSettings();
	exitAlternateScreen();
}

int main(int argc, char **argv)
{
	init(argc, argv);
	srand(time(0));
	startTheGame();
}
