#include <algorithm>
#include <asm-generic/ioctls.h>
#include <atomic>
#include <bits/types/struct_timeval.h>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
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
	cout << result;
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

vector<string> getPrevWordSet()
{
	return {"apple",  "banana", "cherry",  "dragonfruit", "elephant",
		"falcon", "guitar", "horizon", "island",      "jungle"};
}

vector<string> getNextWordSet()
{
	return {"apple",  "banana", "cherry",  "dragonfruit", "elephant",
		"falcon", "guitar", "horizon", "island",      "jungle"};
}

void updateStateToPrev()
{
	// TODO: Add implementation.
}

void updateStateToNext(string &state, string &inputs, int &spacesCount, int &wc)
{
	vector<string> words = getNextWordSet();
	wc = words.size();
	state = "";
	for (int i = 0; i < wc; i++) {
		state += words[i];
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
	if (ch == BACKSPACE) {
		if (!inputs.empty()) {
			if (inputs.back() == ' ') {
				spacesCount--;
			}
			inputs.pop_back();
		} else {
			// TODO: Implement updateStateToPrev().
		}
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
		cout << GREY_COLOR;
		cout << REVERSE_VIDEO;
		cout << ' ';
		cout << RESET_VIDEO;
	}
	if (j < state.size()) {
		cout << GREY_COLOR;
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
		cout << GREY_COLOR;
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
	vector<string> words = getNextWordSet();
	int wc = words.size();
	int spacesCount = 0;
	int wordsSize = words.size();
	string state = "";
	for (int i = 0; i < wordsSize; i++) {
		state += words[i] + ((i == wordsSize - 1) ? "" : " ");
	}
	cin.tie(NULL);
	char ch;

	cout << CLEAR_SCREEN;
	printTimer(currentTime);
	cout << GREY_COLOR;
	moveCursor(1, DOWN);
	for (int i = 0; i < state.size(); i++) {
		if (i == 0) {
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

	string inputs = "";

	struct pollfd fds[1];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	while (!stopFlag) {
		if (isInputAvailable(fds)) {
			ch = get_key();
			updateAndPrintGame(state, inputs, ch, spacesCount, wc);
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
