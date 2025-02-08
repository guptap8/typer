#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
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
	cout << "\r" << moveCursor(1, UP);
	cout << time;
	cout << "\r";
	cout.flush();
}

void flush_input()
{
	tcflush(STDIN_FILENO, TCIFLUSH); // Flush input buffer
}

char get_key()
{
	struct termios oldt, newt;
	char ch;

	// Get current terminal settings
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;

	// Disable canonical mode and echo
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	// Read single character
	read(STDIN_FILENO, &ch, 1);

	// Restore old terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	// Flush input buffer
	flush_input();

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
		cout << words[i];
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
		// debugPrinter(words, guessedWords);
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
		// debugPrinter(words, guessedWords);
		printWords(words, guessedWords);
	} else {
		guessedWords.back() += ch;
		// debugPrinter(words, guessedWords);
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
	while (!stopFlag) {
		if ((ch = get_key()) != EOF) {
			cout << CLEAR_SCREEN;

			updateAndPrintState(words, guessedWords, ch,
					    currentWord, indexInWord);

			// Flush output.
			cout.flush();
		} else {
			break;
		}
	}
}

void timer(int seconds)
{
	for (int i = seconds; i > 0 && !stopFlag; --i) {
		// std::cout << "Time remaining: " << i << " seconds\r";
		// std::cout.flush();
		currentTime = seconds - i;
		// printTimer(i);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	// std::cout << "\nTime's up!" << std::endl;
	stopFlag = true; // Signal worker to stop
}

void enterAlternateScreen()
{
	std::cout << "\033[?1049h"; // Switch to the alternate screen buffer
	std::cout.flush();
}

void exitAlternateScreen()
{
	std::cout << "\033[?1049l"; // Return to the normal screen buffer
	std::cout.flush();
}

void signalHandler(int signal)
{
	exitAlternateScreen();
	exit(signal);
}

int main(int argc, char **argv)
{
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	enterAlternateScreen();

	int totalSeconds = 10;
	thread worker(runGame);
	timer(totalSeconds);
	worker.join();
	cout << CLEAR_SCREEN;
	cout << "GAME OVER" << endl;
	sleep(1);

	exitAlternateScreen();
}
