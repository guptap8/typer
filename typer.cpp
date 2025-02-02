#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define GREY_COLOR "\x1b[38;2;100;100;100m"
#define RED_COLOR "\x1b[31m"
#define GREEN_COLOR "\x1b[32m"
#define YELLOW_COLOR "\x1b[33m"
#define BLUE_COLOR "\x1b[34m"
#define MAGENTA_COLOR "\x1b[35m"
#define CYAN_COLOR "\x1b[36m"
#define RESET_COLOR "\x1b[0m"

using namespace std;

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
	cout << "\033[1A";
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
	} else if (ch == 127) {
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

int main(int argc, char **argv)
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

	cout << "\033[2J\033[1;1H";
	cout << GREY_COLOR;
	cout << state << "\r";
	cout << RESET_COLOR;
	cout.flush();
	while ((ch = get_key()) != EOF) {
		// Clear screen.
		cout << "\033[2J\033[1;1H";

		updateAndPrintState(words, guessedWords, ch, currentWord,
				    indexInWord);

		// Flush output.
		cout.flush();
	}
}
