// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "global.h"
#include "board.h"
#include "hotspot.h"
#include "util.h"

#ifndef LINUX
#include "windows.h"
#endif



int get_global_width(board level, int row_num)
{
	int max = 0;
	int i,n;

	for (i = 0; i < row_num; i++)
	{
		n = (int)strlen((char*)level[i]);
		if (n > max)
			max = n;
	}

	return max; 
}

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15



/*
Set Attribute Mode	<ESC>[{attr1}; ...; {attrn}m
Sets multiple display attribute settings.The following lists standard attributes :
0	Reset all attributes
1	Bright
2	Dim
4	Underscore
5	Blink
7	Reverse
8	Hidden

Foreground Colours
30	Black
31	Red
32	Green
33	Yellow
34	Blue
35	Magenta
36	Cyan
37	White

Background Colours
40	Black
41	Red
42	Green
43	Yellow
44	Blue
45	Magenta
46	Cyan
47	White

printf("%c[%d;%dmHello World%c[%dm\n",27,1,33,27,0);

*/


/*
39	Default foreground color
30	Black
31	Red
32	Green
33	Yellow
34	Blue
35	Magenta
36	Cyan
37	Light gray
90	Dark gray
91	Light red
92	Light green
93	Light yellow
94	Light blue
95	Light magenta
96	Light cyan
97	White

49	Default background color
40	Black
41	Red
42	Green
43	Yellow
44	Blue
45	Magenta
46	Cyan
47	Light gray
100	Dark gray
101	Light red
102	Light green
103	Light yellow
104	Light blue
105	Light magenta
106	Light cyan
107	White
*/


int get_terminal_color_code(int color, int background)
{
	switch (color)
	{
	case BLACK:        return 30 + background * 10;
	case BLUE:         return 34 + background * 10;
	case GREEN:        return 32 + background * 10;
	case CYAN:         return 36 + background * 10;
	case RED:          return 31 + background * 10;
	case MAGENTA:	   return 35 + background * 10;
	case BROWN:	       return 33 + background * 10;
	case LIGHTGRAY:    return 37 + background * 10;
	case DARKGRAY:     return 90 + background * 10;
	case LIGHTBLUE:    return 94 + background * 10;
	case LIGHTGREEN:   return 92 + background * 10;
	case LIGHTCYAN:    return 96 + background * 10;
	case LIGHTRED:     return 91 + background * 10;
	case LIGHTMAGENTA: return 95 + background * 10;
	case YELLOW:	   return 93 + background * 10;
	case WHITE:        return 97 + background * 10;
	}
	exit_with_error("unsupported color");
	return 0;
}

void SetColorAndBackground(int ForgC, int BackC)
{
#ifndef LINUX
	WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);
#else
	ForgC = get_terminal_color_code(ForgC, 0);
	BackC = get_terminal_color_code(BackC, 1);
	printf("%c[%d;%dm", 27, ForgC, BackC);
#endif
}

void print_box()
{
#ifndef LINUX
	printf("%c", 254);
#else
	//	printf("%c%c%c", 0xe2,0x96,0xa0);
	printf("%c%c%c", 0xe2, 0x96, 0xae);
#endif
}


#ifdef THREADS
pthread_mutex_t show_board_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void show_board(board b_in)
{
	int i, j;
	board b;

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&show_board_mutex);
#endif

	copy_board(b_in, b);

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (b[i][j] & DEADLOCK_ZONE)
			{
				b[i][j] &= ~DEADLOCK_ZONE;
				b[i][j] |= BASE;
			}
		}


	printf("  ");
	for (i = 0; i < width; i++)
		printf("%d", i % 10);
	printf("\n");

	for (i = 0; i < height; i++)
	{
		printf("%d ", i % 10);

		for (j = 0; j < width; j++)
		{
			switch (b[i][j])
			{

			case  0:

				SetColorAndBackground(WHITE, WHITE);
				printf(" ");
				break;

			case WALL:
				SetColorAndBackground(CYAN, CYAN);
				printf(" ");
				break;

			case BOX:
				SetColorAndBackground(DARKGRAY, WHITE);
		
				print_box();
				break;

			case TARGET:

				SetColorAndBackground(RED, RED);
				printf(" ");
				break;

			case SOKOBAN:

				SetColorAndBackground(BLUE, WHITE);
				printf(".");
				break;

			case (TARGET | BOX) :

				SetColorAndBackground(DARKGRAY, RED);
				print_box();
				break;

			case (TARGET | SOKOBAN) :

				SetColorAndBackground(BLUE, RED);
				printf(".");
				break;

			case (BASE):
				SetColorAndBackground(BLUE, BLUE);
				printf(" ");
				break;

			case (BASE | SOKOBAN) :
				SetColorAndBackground(LIGHTBLUE, BLUE);
				printf(".");
				break;

			case (BASE | BOX) :
				SetColorAndBackground(DARKGRAY, BLUE);
				print_box();
				break;

			case (BASE | TARGET | BOX) :
				SetColorAndBackground(DARKGRAY, MAGENTA);
				print_box();
				break;

			case (BASE | TARGET ) :
				SetColorAndBackground(DARKGRAY, MAGENTA);
				printf(" ");
				break;
			case (BASE | TARGET | SOKOBAN) :
				SetColorAndBackground(LIGHTBLUE, MAGENTA);
				printf(".");
				break;

			default:
				printf("value: %d\n", b[i][j]);
				exit_with_error("unknown board value");
			}
		}
		SetColorAndBackground(WHITE, BLACK);
		printf("\n");
	}
	SetColorAndBackground(WHITE, BLACK);

	printf("\n");

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&show_board_mutex);
#endif

}


void print_in_color(const char *txt, const char *color)
{
#ifdef THREADS
	if (cores_num > 1) pthread_mutex_lock(&show_board_mutex);
#endif

	if (strcmp(color, "red") == 0)
		SetColorAndBackground(RED, BLACK);
	if (strcmp(color, "blue") == 0)
		SetColorAndBackground(LIGHTBLUE, BLACK);

	printf("%s", txt);
	SetColorAndBackground(WHITE, BLACK);

#ifdef THREADS
	if (cores_num > 1) pthread_mutex_unlock(&show_board_mutex);
#endif
}

void save_board_to_file(board b, FILE *fp)
{
	int i, j;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (b[i][j] == 0) fprintf(fp, " ");
			if (b[i][j] == WALL) fprintf(fp, "#");
			if (b[i][j] == BOX) fprintf(fp, "$");
			if (b[i][j] == TARGET) fprintf(fp, ".");
			if (b[i][j] == (BOX | TARGET)) fprintf(fp, "*");
			if (b[i][j] == SOKOBAN) fprintf(fp, "@");
			if (b[i][j] == (SOKOBAN | TARGET)) fprintf(fp, "+");
		}
		fprintf(fp, "\n");
	}
}

void get_board_from_text(board level, int row_num, board b, int *w)
{
	int i, j, k;
	int h;

	*w = get_global_width(level, row_num);
	h = row_num;

	// clear all characters after "\n"
	for (i = 0; i < h; i++)
		for (j = 0; j < (*w); j++)
		{
			if (level[i][j] < 32) // not printable
			{
				for (k = j; k < (*w); k++)
					level[i][k] = ' ';
				break;
			}
		}

	for (i = 0; i < h; i++)
	{
		for (j = 0; j < *w; j++)
		{
			b[i][j] = 0;

			if (level[i][j] == '_') b[i][j] = SPACE;
			if (level[i][j] == '-') b[i][j] = SPACE;
			if (level[i][j] == ' ') b[i][j] = SPACE;
			if (level[i][j] == '#') b[i][j] = WALL;
			if (level[i][j] == '$') b[i][j] = BOX;
			if (level[i][j] == '.') b[i][j] = TARGET;
			if (level[i][j] == '*') b[i][j] = BOX | TARGET;
			if (level[i][j] == '@') b[i][j] = SOKOBAN;
			if (level[i][j] == '+') b[i][j] = SOKOBAN | TARGET;
		}
	}
}


int has_only_legal_characters(char *s)
{
	char legal[10] = "_- #$.*@+";
	int i, j;
	int n = (int)strlen(s);

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < 9; j++)
			if (s[i] == legal[j])
				break;
		if (j == 9)
			return 0;
	}
	return 1;
}

int has_whitespaces(char *s)
{
	int i;
	int n = (int)strlen(s);

	for (i = 0; i < n; i++)
		if ((s[i] == ' ') || (s[i] == '\t'))
			return 1;
	return 0;
}

int is_empty_row(char *s)
{
	// few sokoban levels have empty rows as a decorative element
	int n = (int)strlen(s);

	if (n < 5) return 0;
	
	if (has_only_legal_characters(s) == 0) return 0;
	if (has_whitespaces(s)) return 0;

	return 1;
}


int is_sokoban_line(char *s)
{
	int n;
	n = (int)strlen(s);

	if (n < 3) return 0;

	if (strstr(s, "#") == 0)
		if (is_empty_row(s) == 0)
			return 0;

	if (has_only_legal_characters(s) == 0)
		return 0;

	return 1;
}

void set_board_and_title(board text_level, int row_num, board b)
{
	get_board_from_text(text_level, row_num, b, &width);
	height = row_num;
}


int load_level_from_file(board b, int level_number) // level number is one-based!
{
	// loads a sokoban level from a .sok or .txt format
	// returns the number of levels in the file

	FILE *fp;
	board text_level;
	int row_num;
	char s[1000];
	int in_level = 0;
	int current_level_num = 0; // how many levels were already read
	char filename[3000];
	char tmp_title[1000] = "None";
	int ok = 0;

	if (level_number <= 0)
		exit_with_error("Level number must be positive");

#ifndef LINUX
	sprintf(filename, "%s\\levels\\%s.sok", global_dir, global_level_set_name);
#else
	sprintf(filename, "%s/levels/%s.sok", global_dir, global_level_set_name);
#endif

	if (strstr(global_level_set_name, ".") ||
		strstr(global_level_set_name, "\\") ||
		strstr(global_level_set_name, "/"))
		strcpy(filename, global_level_set_name);

	
	
//	sprintf(filename, "c:\\data\\debug.txt"); level_number = 1;

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		my_getch();
		exit(0);
	}

	while (fgets(s, 1000, fp))
	{
		remove_newline(s);

		if (strstr(s, "'"))
			strcpy(tmp_title, s);

		if (is_sokoban_line(s))
		{
			if (in_level == 0)
			{
				in_level = 1;
				row_num = 0;
				ok = 1;
			}

			if (strlen(s) >= MAX_SIZE)
			{
				s[MAX_SIZE - 1] = 0;
				ok = 0;
				if (verbose >= 4)
					printf("width is too big! level %d\n", current_level_num + 1);
				strcpy(global_fail_reason, "Size is too big");
			}

			strcpy((char*)text_level[row_num], s);
			row_num++;

			if (row_num >= MAX_SIZE)
			{
				ok = 0;
				if (verbose >= 4)
					printf("height is too big! level %d\n", current_level_num + 1);
				strcpy(global_fail_reason, "Size is too big");
				row_num--;
			}
		}
		else // not a sokoban line
		{
			if (in_level)
			{
				current_level_num++;

				if (s[0] == ';')
				{
					strcpy(tmp_title, s + 1);
					remove_preceeding_spaces(tmp_title);
				}

				if (current_level_num == level_number)
				{
					if (ok)
						set_board_and_title(text_level, row_num, b);
					else
						height = width = 0;

					strcpy(level_title, tmp_title);
					strcpy(tmp_title, "None");
				}
			}
			in_level = 0;
		}

		if (strncmp(s, "Title:", 6) == 0)
		{
			if (current_level_num == level_number)
				strcpy(level_title, s + 7);
		}

		if (strncmp(s, "Title : ", 7) == 0)
		{
			if (current_level_num == level_number)
				strcpy(level_title, s + 8);
		}
	}

	if (in_level)
	{
		current_level_num++;

		if (current_level_num == level_number)
		{
			if (ok)
				set_board_and_title(text_level, row_num, b);
			else
				height = width = 0;

			strcpy(level_title, tmp_title);
		}
	}

	fclose(fp);
	
	return current_level_num;
}

void save_debug_board(board b)
{
	FILE *fp;

	fp = fopen("c:\\data\\debug.txt", "w");
	if (fp == NULL)
		exit_with_error("can't save");

	save_board_to_file(b, fp);

	fclose(fp);
}
