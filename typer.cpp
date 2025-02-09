#include <atomic>
#include <bits/types/struct_timeval.h>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/poll.h>
#include <sys/select.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define RGB_COLOR(r, g, b)                                                     \
	"\x1b[38;2;" + to_string(r) + ";" + to_string(g) + ";" +               \
	    to_string((b)) + "m"
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

enum CursorDirection { UP, DOWN, RIGHT, LEFT };

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
	// TODO: Move right by default because no error handling
	// right now. Change later to handle invalid input properly.
	default:
		result += "A";
		break;
	}
	return result;
}

atomic<bool> stopFlag{false};
atomic<int> currentTime{0};

void printTimer(int time)
{
	cout << RGB_COLOR(255, 255, 255);
	cout << "\r" << moveCursor(1, UP);
	cout << time;
	cout << "\r";
	cout << RESET_COLOR;
	cout.flush();
}

void flush_input()
{
	tcflush(STDIN_FILENO, TCIFLUSH); // Flush input buffer
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

void printWords(vector<string> &words, vector<string> &guessedWords)
{
	printTimer(currentTime);
	cout << moveCursor(1, DOWN);
	for (int i = 0; i < guessedWords.size(); i++) {
		for (int j = 0;
		     j < max(guessedWords[i].size(), words[i].size()); j++) {
			if (j >= guessedWords[i].size()) {
				cout << GREY_COLOR;
				cout << words[i][j];
			} else if (j >= words[i].size()) {
				cout << RED_COLOR;
				cout << guessedWords[i][j];
			} else if (guessedWords[i][j] == words[i][j]) {
				cout << GREEN_COLOR;
				cout << guessedWords[i][j];
			} else {
				cout << RED_COLOR;
				cout << guessedWords[i][j];
			}
		}
		cout << (i == words.size() - 1 ? "" : " ");
		cout << RESET_COLOR;
	}
	for (int i = guessedWords.size(); i < words.size(); i++) {
		cout << GREY_COLOR;
		if (i == guessedWords.size()) {
			cout << REVERSE_VIDEO;
		}
		cout << words[i];
		if (i == guessedWords.size()) {
			cout << RESET_VIDEO;
		}
		cout << (i == words.size() - 1 ? "" : " ");
	}
	cout << RESET_COLOR;
}

void debugPrinter(vector<string> &words, vector<string> &guessedWords)
{
	for (auto i : words) {
		cout << i << " ";
	}
	cout << "\n";
	for (auto i : guessedWords) {
		cout << i << " ";
	}
	cout << moveCursor(1, UP);
	cout << "\r";
}

void updateAndPrintState(vector<string> &words, vector<string> &guessedWords,
			 char ch, int &currentWord, int &indexInWord)
{
	if (ch == ' ') {
		currentWord++;
		indexInWord = 0;
		printWords(words, guessedWords);
		guessedWords.push_back("");
	} else if (ch == BACKSPACE) {
		if (guessedWords.size() > 1 ||
		    (guessedWords.size() == 1 && guessedWords[0].size() > 0)) {
			int originalSize = guessedWords.back().size();
			if (originalSize == 0) {
				if (guessedWords.size() > 1) {
					guessedWords.pop_back();
				}
				if (guessedWords.size() > 1 ||
				    (guessedWords.size() == 1 &&
				     guessedWords[0].size() > 0)) {
					originalSize =
					    guessedWords.back().size();
				}
			}
			if (guessedWords.size() > 1 ||
			    (guessedWords.size() == 1 &&
			     guessedWords[0].size() > 0)) {
				guessedWords.back() =
				    guessedWords.back().substr(0, originalSize -
								      1);
			}
		}
		printWords(words, guessedWords);
	} else {
		guessedWords.back() += ch;
		printWords(words, guessedWords);
	}
	cout << RESET_COLOR;
	int moveBack = 0;
	int i;
	for (i = words.size() - 1; i > guessedWords.size() - 1; i--) {
		moveBack += words[i].size() + (i == 0 ? 0 : 1);
	}
	if (i >= 0 && i < guessedWords.size() &&
	    guessedWords[i].size() < words[i].size()) {
		moveBack += words[i].size() - guessedWords[i].size();
	}
	string moveBackString = "\033[" + to_string(moveBack) + "D";
	if (moveBack > 0) {
		cout << moveBackString;
	}
}

bool isInputAvailable()
{
	fd_set readfds;
	struct timeval timeout = {0, 0};

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	int selectResult =
	    select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);

	cout << "Select result: " << to_string(selectResult) << endl;

	return selectResult > 0;
}

bool isInputAvailable2(struct pollfd *fds)
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

void runGame()
{
	vector<string> words = {"apple",    "banana", "cherry", "dragonfruit",
				"elephant", "falcon", "guitar", "horizon",
				"island",   "jungle"};
	vector<string> guessedWords = {""};
	int wordsSize = words.size();
	string state = "";
	int currentWord = 0;
	for (int i = 0; i < wordsSize; i++) {
		state += words[i] + ((i == wordsSize - 1) ? "" : " ");
	}
	int indexInWord = 0;
	cin.tie(NULL);
	char ch;

	cout << CLEAR_SCREEN;
	printTimer(currentTime);
	cout << GREY_COLOR;
	cout << moveCursor(1, DOWN) << state << "\r";
	cout << RESET_COLOR;
	cout.flush();
	struct pollfd fds[1];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	while (!stopFlag) {
		if (isInputAvailable2(fds)) {
			ch = get_key();
			cout << CLEAR_SCREEN;

			updateAndPrintState(words, guessedWords, ch,
					    currentWord, indexInWord);

			// Flush output.
			cout.flush();
		}
	}
}

void timer(int seconds)
{
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
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGSEGV, signalHandler);
}

int main(int argc, char **argv)
{
	registerSignalHandlers();
	enterAlternateScreen();
	disableCanonicalMode();

	int totalSeconds = 60;
	thread worker(runGame);
	thread timerWorker(timer, totalSeconds);
	worker.join();
	timerWorker.join();
	cout << CLEAR_SCREEN;
	cout << "GAME OVER" << endl;
	this_thread::sleep_for(chrono::milliseconds(500));

	restoreTerminalSettings();
	exitAlternateScreen();
}
