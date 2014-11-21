#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

typedef struct Position
{
	int x;
	int y;
} Position;

typedef struct Block
{
	int blockType[3][3];
	Position position;
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

int isCollision();
int isGameMoving();

void moveLeft();
void moveDown();
void moveRight();
void resetBlock();
void renderBlock();
void rotateBlock();
void renderFrame();
void createFrame();
void renderCanvas();
void spawnNewBlock();
void checkGameState();
void fetchUserInput();
void drawPauseScreen();
void setBlockInCanvas();
void printCanvasBlocks();
void drawGameOverScreen();
void checkRowCompletion();
void createNewBlockBatch();
void moveRowsAbove(int completedRow);

Block createBlock(int blockNumber);


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
 /* Canvas logic */
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

void checkGameState()
{
	renderFrame();

	switch (gameState)
	{
	case 0: // Game play
		printCanvasBlocks();
		renderBlock();
		break;
	case 1: // Game pause
		drawPauseScreen();
		break;
	case 2: // Game over
		drawGameOverScreen();
		break;
	}
	//drawInstructions();
	//drawScoreboard();
}

void drawPauseScreen()
{
	mvprintw(CANVAS_HEIGHT / 2, 0
		, "|±±±±±±±±±±|");
	mvprintw(CANVAS_HEIGHT / 2 + 1, 0
		, "|¥ PAUSE  ¥|");
	mvprintw(CANVAS_HEIGHT / 2 + 2, 0
		, "|¥¥¥¥¥¥¥¥¥¥|");
}

void drawGameOverScreen()
{
	mvprintw(CANVAS_HEIGHT / 2, 0
		, "|±±±±±±±±±±|");
	mvprintw(CANVAS_HEIGHT / 2 + 1, 0
		, "|GAME OVER!|");
	mvprintw(CANVAS_HEIGHT / 2 + 2, 0
		, "|¥¥¥¥¥¥¥¥¥¥|");
}

void createFrame()
{
	noecho();
	initscr();
	curs_set(FALSE);
	srand(time(NULL));
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
}

void checkRowCompletion()
{
	int x, y;
	int isFullLine = 0;

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
				moveRowsAbove(y);
			}
			isFullLine = 0;
		}
	}
}

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
				mvprintw(y, x, "X");
			}
		}
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
				// resetGame();
				break;
		}
	}

	if (isCollision())
	{
		currentBlock = orgBlock;
	}
}

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

/* Block logic */

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
					[currentBlock.position.y + y] = 1;
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
				mvprintw(currentBlock.position.y + y, 
					currentBlock.position.x + x, "X");
			}
		}
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
				= tempBlock.blockType[2 - j][i];
		}
	}
}

Block createBlock(int blockNumber)
{
	int b[3][3];
	
	Block tempBlock;
	
	switch (blockNumber)
	{
	case 0:
		b[0][0] = 0;	b[0][1] = 1;	b[0][2] = 0;	// .#.
		b[1][0] = 0;	b[1][1] = 1;	b[1][2] = 0;	// .#.
		b[2][0] = 0;	b[2][1] = 1;	b[2][2] = 0;	// .#.
		break;
	case 1:
		b[0][0] = 0;	b[0][1] = 0;	b[0][2] = 0;	// ...
		b[1][0] = 0;	b[1][1] = 1;	b[1][2] = 0;	// .#.
		b[2][0] = 1;	b[2][1] = 1;	b[2][2] = 1;	// ###
		break;
	case 2:
		b[0][0] = 0;	b[0][1] = 0;	b[0][2] = 0;	// ...
		b[1][0] = 0;	b[1][1] = 1;	b[1][2] = 1;	// .##
		b[2][0] = 0;	b[2][1] = 1;	b[2][2] = 1;	// .##
		break;
	case 3:
		b[0][0] = 0;	b[0][1] = 0;	b[0][2] = 0;	// ...
		b[1][0] = 1;	b[1][1] = 1;	b[1][2] = 0;	// ##.
		b[2][0] = 0;	b[2][1] = 1;	b[2][2] = 1;	// .##
		break;
	case 4:	
		b[0][0] = 0;	b[0][1] = 0;	b[0][2] = 0;	// ...
		b[1][0] = 0;	b[1][1] = 1;	b[1][2] = 1;	// .##
		b[2][0] = 1;	b[2][1] = 1;	b[2][2] = 0;	// ##.
		break;
	case 5:	
		b[0][0] = 0;	b[0][1] = 0;	b[0][2] = 1;	// ..#
		b[1][0] = 0;	b[1][1] = 0;	b[1][2] = 1;	// ..#
		b[2][0] = 0;	b[2][1] = 1;	b[2][2] = 1;	// .##
		break;
	case 6:	
		b[0][0] = 1;	b[0][1] = 0;	b[0][2] = 0;	// #..
		b[1][0] = 1;	b[1][1] = 0;	b[1][2] = 0;	// #..
		b[2][0] = 1;	b[2][1] = 1;	b[2][2] = 0;	// ##.
		break;
	}

	memcpy(tempBlock.blockType, b, sizeof(Block));

	tempBlock.position.x = (CANVAS_WIDTH / 2) - 1;
	tempBlock.position.y = 2;
	
	return tempBlock;
}

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
