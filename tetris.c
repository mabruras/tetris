#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Mulig lage variabler av disse??
#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20
#define BLOCK_SIGN "O"

#define CANVAS_NEXT_START_Y 1
#define CANVAS_NEXT_START_X CANVAS_WIDTH + 7

#define CANVAS_SCORE_X CANVAS_NEXT_START_X - 2
#define CANVAS_SCORE_Y CANVAS_NEXT_START_Y + 6

#define CANVAS_INFO CANVAS_SCORE_X + 17

typedef struct Position
{
	int x;
	int y;
} Position;

typedef struct Block
{
	int blockType[3][3];
	Position position;
	int color;
} Block;

typedef int Canvas[CANVAS_WIDTH + 1][CANVAS_HEIGHT + 1];

Canvas canvas;
int timer = 0;
int inGame = 1;
int points = 0;
int gameState = 0;
int wait = 100000;
int currentBlockInBatch = 0;

Block currentBlock;
Block blockBatch[7];


/* Game */
/* Game : Existence */
void checkGameState();
int isGameMoving();
void resetGame();

/* Game : Level status */
void checkRowCompletion();
void updateScore(int rows);
void increaseSpeed();
void moveRowsAbove(int completedRow);


/* User interference */
/* User interference : key input */
void fetchUserInput();


/* Canvas */
/* Canvas : map logic */
int isCollision();

/* Canvas : Existence */
void resetCanvas();


/* Blocks */
/* Blocks : Movement */
void moveLeft();
void moveRight();
void moveDown();
void rotateBlock();

/* Blocks : Existence */
Block createBlock(int blockNumber);
void resetBlock();
void createNewBlockBatch();
void spawnNewBlock();
void setBlockInCanvas();


/* Graphics */
/* Graphics : Setup */
void createFrame();
void setupColors();

/* Graphics : Canvas content */
void printCanvasBlocks();
void renderBlock();

/* Graphics : frames */
void renderCanvas();
void renderFrame();
void drawInstructions();
void renderNextBlockFrame();
void drawScoreboard();

/* Graphics : Side content */
void drawNextBlock();

/* Graphics : Extra screens */
void drawPauseScreen();
void drawGameOverScreen();


int main(int argc, char ** argv)
{
	createFrame();
	createNewBlockBatch();
	renderCanvas();

	while (inGame)
	{
		checkGameState();
		checkRowCompletion();
		spawnNewBlock();

		fetchUserInput();

		if(timer++ % 10 == 0 
			&& gameState == 0) {
			moveDown();
		}
		
		renderCanvas();
		usleep(wait);
	}

	endwin();

	return 0;
}


/* Game */
/* Game : Existence */

void checkGameState()
{
	renderFrame();
	renderNextBlockFrame();
	
	switch (gameState)
	{
	case 0: // Game play
		printCanvasBlocks();
		drawNextBlock();
		renderBlock();
		break;
	case 1: // Game pause
		drawPauseScreen();
		break;
	case 2: // Game over
		drawGameOverScreen();
		break;
	}
	
	drawInstructions();
	drawScoreboard();
}

int isGameMoving()
{
	int x, y;
	for (x = 0; x < 3; x++)
	{
		for (y = 0; y < 3; y++)
		{
			if (currentBlock.blockType[x][y] != 0)
			{
				return 1;
			}
		}
	}
	return 0;
}

void resetGame()
{
	timer = 0;
	points = 0;
	gameState = 0;
	wait = 100000;
	currentBlockInBatch = 0;

	resetBlock();
	resetCanvas();
	
	createNewBlockBatch();
	renderCanvas();
}

/* Game : Level status */

void checkRowCompletion()
{
	int x, y;
	int isFullLine = 0;
	int rowCount = 0;
	
	if (isGameMoving() == 0)
	{
		for (y = 1; y <= CANVAS_HEIGHT; y++)
		{
			for (x = 1; x <= CANVAS_WIDTH; x++)
			{
				if (canvas[x][y] != 0)
				{
					isFullLine++;
				}
			}

			if (isFullLine == CANVAS_WIDTH)
			{
				rowCount++;
				moveRowsAbove(y);
			}
			isFullLine = 0;
		}
		updateScore(rowCount);
	}
}

void updateScore(int rows)
{
	int i;
	for (i = 1; i <= rows; i++)
	{
		points += CANVAS_WIDTH * i;
		increaseSpeed();
	}
}

void increaseSpeed()
{
	if (wait < 10001)
	{
		wait = 10000;
	} else
	{
		wait -= 500;
	}
}

void moveRowsAbove(int completedRow)
{
	int x, y;
	for (y = completedRow; y > 1; y--)
	{
		for (x = 1; x < CANVAS_WIDTH; x++)
		{
			canvas[x][y] = canvas[x][y-1];
		}
	}
}


/* User interference */
/* User interference : key input */

void fetchUserInput()
{
	Block orgBlock = currentBlock;
	if (gameState == 0)
	{
		switch (getch())
		{
			case KEY_LEFT:
				moveLeft();
				break;

			case KEY_RIGHT:
				moveRight();
				break;

			case KEY_UP:
				rotateBlock();
				break;

			case KEY_DOWN:
				moveDown();
				break;

			case 'p':
				gameState = 1;
				break;
		}
	} else {
		switch (getch())
		{
			case 'p':
				if (gameState == 1)
				{
					gameState = 0;
				}
				break;
			case 'r':
				resetGame();
				break;
			case 'q':
				inGame = 0;
				break;
		}
	}

	if (isCollision())
	{
		currentBlock = orgBlock;
	}
}


/* Canvas */
/* Canvas : map logic */

int isCollision()
{
	int i, j;
	int x = currentBlock.position.x;
	int y = currentBlock.position.y;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			if (currentBlock.blockType[i][j] != 0
					&& (canvas[x + i][y + j] != 0
					|| y >= CANVAS_HEIGHT))
			{
				return 1;
			}
		}
	}
	return 0;
}

/* Canvas : Existence */

void resetCanvas()
{
	int x, y;
	for (y = 1; y <= CANVAS_HEIGHT; y++)
	{
		for (x = 1; x <= CANVAS_WIDTH; x++)
		{
			canvas[x][y] = 0;
		}
	}
}


/* Blocks */
/* Blocks : Movement */

void moveLeft()
{
	currentBlock.position.x--;
}

void moveRight()
{
	currentBlock.position.x++;
}

void moveDown()
{
	currentBlock.position.y++;

	if (isCollision() == 1)
	{		
		currentBlock.position.y--;
		setBlockInCanvas();
		resetBlock();
	}
}

void rotateBlock()
{
	int i, j;
	Block tempBlock = currentBlock;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			currentBlock.blockType[i][j] 
				= tempBlock.blockType[j][2 - i];
		}
	}
}

/* Blocks : Existence */

Block createBlock(int blockNumber)
{
	int b[3][3];
	
	Block tempBlock;
	
	switch (blockNumber)
	{
	case 0:
		b[0][0] = 0;	b[1][0] = 1;	b[2][0] = 0;	// .#.
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 0;	// .#.
		b[0][2] = 0;	b[1][2] = 1;	b[2][2] = 0;	// .#.
		break;
	case 1:
		b[0][0] = 0;	b[1][0] = 0;	b[2][0] = 0;	// ...
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 0;	// .#.
		b[0][2] = 1;	b[1][2] = 1;	b[2][2] = 1;	// ###
		break;
	case 2:
		b[0][0] = 0;	b[1][0] = 0;	b[2][0] = 0;	// ...
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 1;	// .##
		b[0][2] = 0;	b[1][2] = 1;	b[2][2] = 1;	// .##
		break;
	case 3:
		b[0][0] = 0;	b[1][0] = 0;	b[2][0] = 0;	// ...
		b[0][1] = 1;	b[1][1] = 1;	b[2][1] = 0;	// ##.
		b[0][2] = 0;	b[1][2] = 1;	b[2][2] = 1;	// .##
		break;
	case 4:	
		b[0][0] = 0;	b[1][0] = 0;	b[2][0] = 0;	// ...
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 1;	// .##
		b[0][2] = 1;	b[1][2] = 1;	b[2][2] = 0;	// ##.
		break;
	case 5:	
		b[0][0] = 0;	b[1][0] = 1;	b[2][0] = 0;	// .#.
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 0;	// .#.
		b[0][2] = 1;	b[1][2] = 1;	b[2][2] = 0;	// ##.
		break;
	case 6:	
		b[0][0] = 0;	b[1][0] = 1;	b[2][0] = 0;	// .#.
		b[0][1] = 0;	b[1][1] = 1;	b[2][1] = 0;	// .#.
		b[0][2] = 0;	b[1][2] = 1;	b[2][2] = 1;	// .##
		break;
	}

	memcpy(tempBlock.blockType, b, sizeof(Block));

	tempBlock.color = blockNumber + 1;
	tempBlock.position.x = (CANVAS_WIDTH / 2) - 1;
	tempBlock.position.y = 1;
	
	return tempBlock;
}

void resetBlock()
{
	int x, y;
	
	for (x = 0; x < 3; x++)
	{
		for (y = 0; y < 3; y++)
		{
			if (currentBlock.blockType[x][y] != 0)
			{
				currentBlock.blockType[x][y] = 0;
			}
		}
	}
}

void createNewBlockBatch()
{
	int i;
	Block tempBlock;
	
	for (i = 0; i < 7; i++) 
	{
		do
		{
			tempBlock = createBlock(rand() % 7);
		}
		while (tempBlock.blockType ==
				blockBatch[i].blockType);
		blockBatch[i] = tempBlock;
	}
}

void spawnNewBlock()
{
	if (isGameMoving() == 0)
	{
		if (currentBlockInBatch >= 6)
		{
			createNewBlockBatch();
			currentBlockInBatch = 0;
		}
		currentBlock = blockBatch[currentBlockInBatch];
		currentBlockInBatch++;
	}
	if (isCollision())
	{
		gameState = 2;
	}
}

void setBlockInCanvas()
{
	int x, y;
	
	for (x = 0; x < 3; x++)
	{
		for (y = 0; y < 3; y++)
		{
			if (currentBlock.blockType[x][y] != 0)
			{
				canvas[currentBlock.position.x + x]
					[currentBlock.position.y + y] 
					= currentBlock.color;
			}
		}
	}
}


/* Graphics */
/* Graphics : Setup */

void createFrame()
{
	noecho();
	cbreak();
	initscr();
	start_color();
	setupColors();
	curs_set(FALSE);
	srand(time(NULL));
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
}

void setupColors()
{
	init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);	
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
}

/* Graphics : Canvas content */

void printCanvasBlocks()
{
	int x, y;
	
	for (y = 1; y <= CANVAS_HEIGHT; y++)
	{
		for (x = 1; x <= CANVAS_WIDTH; x++)
		{
			if (canvas[x][y] == 0)
			{
				mvprintw(y, x,  " ");
			}
			else
			{
				attron(COLOR_PAIR(canvas[x][y]));
				mvprintw(y, x, BLOCK_SIGN);
				attron(COLOR_PAIR(7));
			}
		}
	}
}

void renderBlock()
{
	int x, y;
	
	for (x = 0; x < 3; x++)
	{
		for (y = 0; y < 3; y++)
		{
			if (currentBlock.blockType[x][y] == 1)
			{
				attron(COLOR_PAIR(currentBlock.color));
				mvprintw(currentBlock.position.y + y, 
					currentBlock.position.x + x, BLOCK_SIGN);
				attron(COLOR_PAIR(7));
			}
		}
	}
}

/* Graphics : frames */

void renderCanvas()
{
	clear();
	checkGameState();
	refresh();
}

void renderFrame()
{
	int x, y;
	
	for (y = 0; y <= CANVAS_HEIGHT + 1; y++)
	{
		for (x = 0; x <= CANVAS_WIDTH + 1; x++)
		{
			if (y == 0 || y == CANVAS_HEIGHT + 1)
			{
				canvas[x][y] = 1;
				mvprintw(y, x, "#");
			} else if (x == 0 || x == CANVAS_WIDTH + 1)
			{
				canvas[x][y] = 1;
				mvprintw(y, x, "|");
			}
		}
	}
}

void drawInstructions()
{
	mvprintw(1, CANVAS_INFO
		, "       * INFORMATION *");
	mvprintw(2, CANVAS_INFO
		, "*****************************");
		
	if (gameState == 0)
	{
		mvprintw(3, CANVAS_INFO
			, "|>     KEY UP = Rotate     <|");
		mvprintw(4, CANVAS_INFO
			, "|>   KEY DOWN = Speed up   <|");
		mvprintw(5, CANVAS_INFO
			, "|>   KEY LEFT = Move left  <|");
		mvprintw(6, CANVAS_INFO
			, "|>  KEY RIGHT = Move right <|");
		mvprintw(7, CANVAS_INFO
			, "|>        'p' = Pause game <|");
	} else
	{
		mvprintw(3, CANVAS_INFO
			, "|>  'p' = Unpause game     <|");
		mvprintw(4, CANVAS_INFO
			, "|>  'r' = Restart game     <|");
		mvprintw(5, CANVAS_INFO
			, "|>  'q' = Quit             <|");
		mvprintw(6, CANVAS_INFO
			, "|>                         <|");
		mvprintw(7, CANVAS_INFO
			, "|>                         <|");
	}
	
	mvprintw(8, CANVAS_INFO
		, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}

void renderNextBlockFrame()
{
	int i;

	mvprintw(CANVAS_NEXT_START_Y
		, CANVAS_NEXT_START_X
		, " * NEXT *");
	mvprintw(CANVAS_NEXT_START_Y + 1
		, CANVAS_NEXT_START_X
		, "*********");
	
	for (i = 0; i < 3; i++)
	{
		mvprintw(i + 3
			, CANVAS_NEXT_START_X
			, "|>");
		mvprintw(i + 3
			, CANVAS_NEXT_START_X + 7
			, "<|");
	}

	mvprintw(6
		, CANVAS_NEXT_START_X
		, "^^^^^^^^^");
}

void drawScoreboard()
{
	mvprintw(CANVAS_SCORE_Y
		, CANVAS_SCORE_X
		, "  * SCORE *");
	mvprintw(CANVAS_SCORE_Y + 1
		, CANVAS_SCORE_X
		, "*************");
	mvprintw(CANVAS_SCORE_Y + 2
		, CANVAS_SCORE_X
		, "|>         <|");
	
	move(CANVAS_SCORE_Y + 2
		, CANVAS_SCORE_X + 5);
	printw("%d", points);
	
	mvprintw(CANVAS_SCORE_Y + 3
		, CANVAS_SCORE_X
		, "^^^^^^^^^^^^^");
}

/* Graphics : Side content */

void drawNextBlock()
{
	int x, y;
	if (currentBlockInBatch == 6)
	{
		mvprintw(4, CANVAS_NEXT_START_X + 4, "?");
	} else
	{
		for (x = 0; x < 3; x++)
		{
			for (y = 0; y < 3; y++)
			{
				if (blockBatch[currentBlockInBatch]
					.blockType[x][y] == 1)
				{
					attron(COLOR_PAIR(blockBatch[currentBlockInBatch].color));				
					mvprintw(y + 3, 
						CANVAS_NEXT_START_X + x + 3
						, BLOCK_SIGN);
					attron(COLOR_PAIR(7));
				}
			}
		}
	}
}

/* Graphics : Extra screens */

void drawPauseScreen()
{
	mvprintw(CANVAS_HEIGHT / 2
		, (CANVAS_WIDTH / 2) - 4
		, "**********");
	mvprintw(CANVAS_HEIGHT / 2 + 1
		, (CANVAS_WIDTH / 2) - 4
		, "* PAUSE  *");
	mvprintw(CANVAS_HEIGHT / 2 + 2
		, (CANVAS_WIDTH / 2) - 4
		, "**********");
}

void drawGameOverScreen()
{
	mvprintw(CANVAS_HEIGHT / 2
		, (CANVAS_WIDTH / 2) - 4
		, "**********");
	mvprintw(CANVAS_HEIGHT / 2 + 1
		, (CANVAS_WIDTH / 2) - 4
		, "GAME OVER!");
	mvprintw(CANVAS_HEIGHT / 2 + 2
		, (CANVAS_WIDTH / 2) - 4
		, "**********");
}
