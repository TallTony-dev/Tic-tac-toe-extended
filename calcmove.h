enum AiType {Greedy, Standard};
enum Ruleset {FourRow, ThreeRow};

#define MAX_RULE_INDEX 1

typedef struct boardState BoardState;

typedef struct coordinate {
	int x;
	int y;
} Coordinate; //coords start at a,1 or 1,1 when , and 0,0 is considered invalid

BoardState *InitializeBoard(int _xSize, int _ySize, enum Ruleset _ruleset, int _playerCount);
int AddToBoard(BoardState *board, Coordinate coord, int playerNum);
void PrintBoardState(BoardState *board);
char IsWinner(BoardState *board);
Coordinate DetermineMove(BoardState *board, enum AiType aiType, int playerNum);