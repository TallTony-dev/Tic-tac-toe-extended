#include "board-algorithms.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#ifndef ASSERT_INCLUDED
#define ASSERT_INCLUDED
#include <assert.h> //header guard because assert.h is defined to not have an internal one
#endif
#ifdef __STDC_NO_VLA__
    _Static_assert(__STDC_NO_VLA__ == 0, "Implementation must support VLAs");   //this version of the keyword is used due to a gcc issue
#endif

#define BOARDARR(b,x,y) (*(b->tiles + (((y) * b->xSize) + (x))))

typedef struct boardState {
	int xSize;
	int ySize;
	int playerCount;
	enum Ruleset ruleset;
	char *tiles; //2d array [xSize][ySize]
	//example array access: char c = BOARDARR(b,x,y);
	//This is one way to do a 2D array that I just did to test it out, see playerList for another way
} BoardState;


BoardState *InitializeBoard(int _xSize, int _ySize, enum Ruleset _ruleset, int _playerCount) {
	BoardState *board = malloc(sizeof(BoardState));
	board->ruleset = _ruleset;
	board->xSize = _xSize;
	board->ySize = _ySize;
	board->tiles = calloc(_xSize * _ySize, sizeof(char)); 
	board->playerCount = _playerCount;
	return board;
}

//unsafely takes coordinates starting at 1,1 and converts to array index, then checks if tile is filled before filling
//returns 0 on fail and 1 on success
int AddToBoard(BoardState *board, Coordinate coord, int playerNum) {
	if (BOARDARR(board, coord.x - 1, coord.y - 1) == '\0') { //subtract 1 from each coordinate to account for offset 
		BOARDARR(board, coord.x - 1, coord.y - 1) = (char)playerNum + 'A';
		return 1;
	}
	else
		return 0;
}

void PrintBoardState(BoardState *board) {
	printf("   ");
	for (int x = 0; x < board->xSize; x++) printf("| %c ", 'A' + x);
	printf("|\n   =");
	for (int x = 0; x < board->xSize; x++) printf("====");
	printf("\n");
	
	for (int y = 0; y < board->ySize; y++) {
		printf("%d |", (y + 1));
		for (int x = 0; x < board->xSize; x++) {
			printf("| %c ", BOARDARR(board,x,y));
		}
		printf("|\n");
	}
	printf("\n");
	return;
}

//return the number of adjacent tiles in a given direction, this was also to test recursion
int CheckAdjacencyRecursive(Coordinate startCoord, int xDir, int yDir, BoardState *board, int n) {
	char toLook = BOARDARR(board,startCoord.x,startCoord.y);
	int newX = startCoord.x + xDir;
	int newY = startCoord.y + yDir;
	//check array bounds
	if (newX < 0 || newY < 0 || newX >= board->xSize || newY >= board->ySize)
		return n;
	
	//check if next tile is a match or if checking null tiles
	if (toLook != '\0' && toLook == BOARDARR(board, newX, newY)) {
		startCoord.x += xDir;
		startCoord.y += yDir;
		return CheckAdjacencyRecursive(startCoord, xDir, yDir, board, n + 1);
	}
	else
		return n;
}

//Use an algorithm which assigns weights to the board in order to determine next move based off aitype
Coordinate DetermineMove(BoardState *board, enum AiType aiType, int playerNum) {
	int xLeng = board->xSize;
	int yLeng = board->ySize;
	float weights[xLeng][yLeng]; //VLA just to try them out
	//should use dynamic if using a giant board to avoid potential stack overflow, however 25x25 is current max size
	int playerCount = board->playerCount;
	
	//initialize values to 0
	for (int y = 0; y < board->ySize; y++) {
		for (int x = 0; x < board->xSize; x++) {
			weights[x][y] = 0;
		}
	}

	//allocate an array for each player temporarily storing all their tiles in a list
	Coordinate **playerList = malloc(playerCount * sizeof(Coordinate *));
	int maxTilesPerChar = (xLeng * yLeng) / playerCount + 1;
	for (int i = 0; i < playerCount; i++) {
		playerList[i] = malloc((maxTilesPerChar + 1) * sizeof(Coordinate)); //+1 adds a padding coordinate 
		for (int j = 0; j < maxTilesPerChar + 1; j++) {
			playerList[i][j].x = -1;
			playerList[i][j].y = -1;
		}
	}
	for (int x = 0; x < xLeng; x++) {
		for (int y = 0; y < yLeng; y++) {
			//add player tiles to playerList for easy access
			char tileChar;
			if ((tileChar = BOARDARR(board,x,y)) != '\0') {
				weights[x][y] = -1e10; //make sure that any tiles already taken aren't chosen
				int i = 0;
				while (playerList[tileChar - 'A'][i].x != -1) { i++; } //iterate until empty spot
				Coordinate coord = {x, y};
				playerList[tileChar - 'A'][i] = coord; //should always be in bounds and empty
			}
		}
	}
	//positive weights make a tile the more likely choice
	float edgeDistWeightRatio; //higher means higher weight near middle
	float enemyDistWeight; //higher means higher weight near enemy
	float allyDistWeight; //higher means higher weight near ally
	float enemyBlockWeight; //higher means higher weight to block enemy path
	float allyLineWeight; //higher means higher weight to contribute to ally lines
	
	if (aiType == Standard) {
		edgeDistWeightRatio = -1;
		enemyDistWeight = 2;
		allyDistWeight = 4;
		enemyBlockWeight = 10;
		allyLineWeight = 10;
	}
	else if (aiType == Greedy) {
		edgeDistWeightRatio = -1;
		enemyDistWeight = 2;
		allyDistWeight = 7;
		enemyBlockWeight = 12;
		allyLineWeight = 20;
	}

	//change weights based on contribution to player line or blocking enemy line
	for (int player = 0; player < playerCount; player++) { //iterate through players
		float weight = (playerNum == player ? allyLineWeight : enemyBlockWeight); //set weight based off if is ally or enemy line
		
		for (int i = 0; playerList[player][i].x != -1; i++) { //iterate through tiles
			Coordinate coord = playerList[player][i];
			//check each tile to see if it contains a row checking below, right, bottom right, and bottom left
			int n = 0; //n represents the number of tiles in a row - 1
			if (n = CheckAdjacencyRecursive(coord, 1, 0, board, 0)) { //check rightwards
				if (coord.x - 1 > 0)
					weights[coord.x - 1][coord.y] += weight * n;
				if (coord.x + n/* + 1*/ < xLeng/* - 1*/)
					weights[coord.x + n + 1][coord.y] += weight * n;
			}
			if (n = CheckAdjacencyRecursive(coord, 1, 1, board, 0)) { //check right-downwards
				if (coord.x - 1 >= 0 && coord.y - 1 >= 0)
					weights[coord.x - 1][coord.y - 1] += weight * n;
				if (coord.x + n/* + 1*/ < xLeng/* - 1*/ && coord.y + n/* + 1*/ < yLeng/* - 1*/)
					weights[coord.x + n + 1][coord.y + n + 1] += weight * n;
			}
			if (n = CheckAdjacencyRecursive(coord, 0, 1, board, 0)) { //check downwards
				if (coord.y - 1 >= 0)
					weights[coord.x][coord.y - 1] += weight * n;
				if (coord.y + n/* + 1*/ < yLeng/* - 1*/)
					weights[coord.x][coord.y + n + 1] += weight * n;
			}
			if (n = CheckAdjacencyRecursive(coord, -1, 1, board, 0)) { //check left-downwards
				if (coord.x + 1 < xLeng && coord.y - 1 >= 0)
					weights[coord.x + 1][coord.y - 1] += weight * n;
				if (coord.x - n - 1 >= 0 && coord.y + n + 1 < yLeng)
					weights[coord.x - n - 1][coord.y + n + 1] += weight * n;
			}
		}
	}
	
	//change weights based off distance from the edge
	for (int x = 0; x < xLeng; x++) {
		for (int y = 0; y < yLeng; y++) {
			weights[x][y] += (abs((float)(x + 1) - (float)(xLeng + 1) / 2.0F) + abs((float)(y + 1) - (float)(yLeng + 1) / 2.0F)) * edgeDistWeightRatio;
		}
	}
	
	//change weights based off being close to player or enemy tiles
	for (int player = 0; player < playerCount; player++) { //iterate through players
		for (int i = 0; playerList[player][i].x != -1; i++) { //iterate through their tiles
		//change the weight of tiles around each player tile
			for (int x = -1; x < 1; x++) {
				for (int y = -1; y < 1; y++) {
					if (playerList[player][i].x + x > 0 && playerList[player][i].y + y > 0
					&& playerList[player][i].x + x < xLeng && playerList[player][i].y + y < yLeng) //check bounds
						weights[playerList[player][i].x + x][playerList[player][i].y + y] += (playerNum == player) ? allyDistWeight : enemyDistWeight;
				}
			}
		}
	}
	//find coord with max weight
	Coordinate toMove = {-1,-1};
	float maxWeight = -1e9; //the max discovered weight so far
	for (int x = 0; x < xLeng; x++) {
		for (int y = 0; y < yLeng; y++) {
			if (weights[x][y] > maxWeight && BOARDARR(board, x, y) == '\0'){
				maxWeight = weights[x][y];
				toMove.x = x + 1;
				toMove.y = y + 1; //+1 to feed into AddToBoard with proper offset (starting at 1,1)
			}
		}
	}
	//free playerlist memory
	for (int i = 0; i < playerCount; i++) {
		free(playerList[i]);
	} 
	free(playerList);
	return toMove;
}

//returns the char of the winner or '?' if draw
char IsWinner(BoardState *board) {
	enum Ruleset rules = board->ruleset;
	int n = 0;
	if (rules == ThreeRow) {
		n = 2;
	}
	else if (rules == FourRow) {
		n = 3;
	}
	Coordinate coord;
	int filledCount = 0;
	for (int x = 0; x < board->xSize; x++) {
		for (int y = 0; y < board->ySize; y++) {
			coord.x = x;
			coord.y = y;
			if (BOARDARR(board, x, y) != '\0')
				filledCount++;
			if (CheckAdjacencyRecursive(coord, 1, 0, board, 0) >= n) { //check rightwards
				return BOARDARR(board, x, y);
			}
			if (CheckAdjacencyRecursive(coord, 1, 1, board, 0) >= n) { //check right-downwards
				return BOARDARR(board, x, y);
			}
			if (CheckAdjacencyRecursive(coord, 0, 1, board, 0) >= n) { //check downwards
				return BOARDARR(board, x, y);
			}
			if (CheckAdjacencyRecursive(coord, -1, 1, board, 0) >= n) { //check left-downwards
				return BOARDARR(board, x, y);
			}
		}
	}
	if (filledCount >= board->xSize * board->ySize)
		return '?';
	return '\0';
}
