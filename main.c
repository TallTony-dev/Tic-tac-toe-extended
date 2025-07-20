#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#ifndef ASSERT_INCLUDED
#define ASSERT_INCLUDED
#include <assert.h> //header guard because is defined to not have an internal one
#endif
#include "board-algorithms.h"

_Static_assert('a' == 97 && 'z' == 122, "Incompatible character representation");  //this version of the keyword is used due to a gcc issue
//this is true for code page 437 and ascii, but its here just in case and to test static assertions

void clear_stdin() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	return;
}

//returns a raw input string up to maxLength characters from stdin, returns 0 if error has occured
int GetRawInput(char *out, int maxLength) {
	clearerr(stdin);
	int index = 0;
	 while ((out[index] = fgetc(stdin)) != EOF) {
		if (++index > maxLength || out[index - 1] == '\n') break;
	}
	if (index > maxLength && out[index - 1] != '\n')
		clear_stdin();
	
	out[index - 1] = '\0'; //set last character to null
	if (ferror(stdin) != 0)
		return 0;
	else
		return 1;
}

//Sets coordinate from user input, validates that it is two numbers delimited by a comma, and checks that they are within exclusive bounds
void GetInput(Coordinate *coord, const int xbound, const int ybound) {
	char in[10] = {0}; //might need to be cleared in odd cases
	beginning:
	if (GetRawInput(in, 10)) { //get raw input string up to 9 effective chars
		int commaIndex = 0;
		char *commaAddress = strchr(in, ',');
		if(commaAddress == NULL || (commaIndex = commaAddress - in) != 1) { //could probably be simplified using strtok
			if (!strcmp("exit", in)) //exit if user typed exit
				exit(2);
			puts("no comma detected or in wrong place, please retry");
			goto beginning; //try again if there is no comma or if the comma is not the second character
		}
		char *val1 = strtok(in, ",");
		int val2 = atoi(strtok(NULL, ",")); //tokenize the input string at the properly placed comma
		char *emptyval3 = strtok(NULL, ","); //check for any extra comma
		if (emptyval3 != NULL || val2 < 0 || val1 == NULL || ((int)*val1) - 96 < 0 || ((int)*val1) - 96 > 17){ //make sure val1 is a lowercase character in range
			puts("values improperly formatted, be sure to use lowercase eg. a,2");
			goto beginning;
		}
		
		if (*val1 - 96 <= xbound && val2 <= ybound && val2 > 0) { //check if values are within bounds
			coord->x = (int)*val1 - 96; //a is 1, b is 2, etc.
			coord->y = val2;
		}
		else {
			puts("out of bounds");
			goto beginning;
		}
		return;
	}
	else {
		puts("string gathering error, please try entering again using format b,7:");
		goto beginning;
	}
	return;
}

int main(){	
	puts("Welcome to tic tac toe extended!\n");
	
	char *answerBuf = malloc(10 * sizeof(char));//memory to store the output from GetRawInput()
	//could be a fixed size but I wanted to try basic dynamic allocation
	int xSize, ySize, aiCount = 0;
	enum Ruleset ruleset;
	bool isUserPlayer;
	char winner;
	do {
		puts("Enter board x dimension (3-17):");
		GetRawInput(answerBuf, 10);
	} while ((xSize = atoi(answerBuf)) < 3 || atoi(answerBuf) > 17); //loop if oob
	do {
		puts("Enter board y dimension (3-17):");
		GetRawInput(answerBuf, 10);
	} while ((ySize = atoi(answerBuf)) < 3 || atoi(answerBuf) > 17); //loop if oob
	do {
		puts("Choose a win condition!\n1: Four in a row\n2: Three in a row\nAnswer using the corresponding number:");
		GetRawInput(answerBuf, 10);
	} while ((ruleset = atoi(answerBuf) - 1) < 0 || ruleset > MAX_RULE_INDEX); //if the enum isn't a rule loop
	do {
		puts("How many AI bots to play? (1-8)\nNote that you must play if there is only one bot");
		GetRawInput(answerBuf, 10);
	} while ((aiCount = atoi(answerBuf)) < 1 || aiCount > 8);
	
	
	enum AiType *aiTypes = malloc(sizeof(enum AiType) * aiCount);
	for (int i = 0; i < aiCount; i++) {
		do {
		printf("What type of AI should bot %d have?\n1: Standard\n2: Greedy\nAnswer with just the relevant index.\n", i + 1);
		GetRawInput(answerBuf, 10);
		} while (((aiTypes[i] = atoi(answerBuf)) - 1) > MAX_AITYPE_INDEX || aiTypes[i] < 0);
	}
	
	
	
	if (aiCount > 1) { //allow the player to not play as long as there is more than one bot
		do {
			puts("Do you want to play? (y/n)");
			GetRawInput(answerBuf, 10);
		} while (!(*answerBuf == 'y' || *answerBuf == 'n')); //if the first char is y or n
		*answerBuf == 'y' ? (isUserPlayer = true) : (isUserPlayer = false);
	}
	
	else isUserPlayer = true;
	free(answerBuf); //again, just for learning
	
	BoardState *board = InitializeBoard(xSize, ySize, ruleset, (isUserPlayer ? aiCount + 1 : aiCount)); //initialize the game board memory
	
	do { //main game loop
		puts("Current board state:\n");
		PrintBoardState(board);
		
		if (isUserPlayer) {
			Coordinate coord = {0,0};
			do {
				puts("Enter valid position eg. b,2 [type exit to exit]");
				GetInput(&coord, xSize, ySize);
			} while (!AddToBoard(board, coord, 0));
		}
		
		for (int i = isUserPlayer ? 1 : 0; i < (aiCount + (isUserPlayer ? 1 : 0)); i++) { //start at player 1 instead of 0 if human is playing
			Coordinate aiMove = DetermineMove(board, i - (isUserPlayer ? 1 : 0), i);
			if (aiMove.x == -1 || !AddToBoard(board, aiMove, i)) { //check for if the ai can't find a move or if it chose invalid location
				break;
			}
		}
		
	} while ((winner = IsWinner(board)) == '\0'); //check for winning or tying state
	free(aiTypes);
	PrintBoardState(board);
	if (winner != '?')
		printf("The winner is %c!", winner); //announce winner
	else
		printf("It's a draw :(");
	return EXIT_SUCCESS;
}	
