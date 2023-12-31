// Festival Sokoban Solver
// Copyright 2018-2020 Yaron Shoham

#include <string.h>

#include "textfile.h"
#include "util.h"

char deadlock_patterns_data[300][10] = {

	"PATTERN",
	"4 3",
	"#$?",
	"# $",
	"# $",
	"?##",
	"",
	"PATTERN",
	"3 4",
	"?$$?",
	"$  #",
	"$##?",
	"",
	"PATTERN",
	"3 4",
	"?$$?",
	"$  #",
	"###?",
	"",
	"PATTERN ",
	"4 3",
	"##?",
	"$ #",
	"$ #",
	"$$?",
	"",
	"PATTERN ",
	"4 4",
	"?#??",
	"# $?",
	"#  $",
	"?###",	
	"",
	"PATTERN ",
	"4 4",
	"?#??",
	"# $?",
	"#  $",
	"?##$",
	"",
	"PATTERN ",
	"3 4",
	"####",
	"$  $",
	"?$#?",
	"",
	"PATTERN ",
	"4 4",
	"?#$#",
	"#  $",
	"#  $",
	"?###",
	"",
	"PATTERN",
	"4 3",
	"?#?",
	"$ $",
	"# #",
	"$$?",
	"",
	"PATTERN ",
	"4 4",
	"?#??",
	"# $#",
	"#  $",
	"?##?",
	"",
	"PATTERN",
	"4 4",
	"#$$?",
	"# $?",
	"#  #",
	"#$$?",
	"",
	"PATTERN",
	"3 4",
	"?$##",
	"#  $",
	"?$$#",
	"",
	"PATTERN",
	"3 4",
	"#$$?",
	"#  #",
	"#$$?",
	"",
	"PATTERN",
	"4 4",
	"$##?",
	"$  #",
	"$ $#",
	"?$$?",
	"",
	"PATTERN ",
	"4 3",
	"#$?",
	"# $",
	"# $",
	"#$?",
	"",
	"PATTERN ",
	"4 3",
	"?#?",
	"# $",
	"# $",
	"?#?",
	"",

	"EOF",
};

char pull_deadlock_patterns_data[300][10] = {
	"PATTERN",
	"3 4",
	"?##?",
	"#@@#",
	"?$$?",
	"",
	"PATTERN",
	"4 4",
	"?#??",
	"#@$?",
	"#@@$",
	"?##?",
	"",
	"PATTERN",
	"4 4",
	"?#??",
	"#@$?",
	"#@@$",
	"?#$?",
	"",
	"PATTERN",
	"4 4",
	"?##?",
	"#@@$",
	"#@$?",
	"?#$?",
	"",
	"PATTERN",
	"4 4",
	"?##?",
	"#@@$",
	"#@@$",
	"?##?",
	"",
	"PATTERN ",
	"3 4",
	"?$$?",
	"$@@$",
	"####",
	"",
	"PATTERN",
	"3 4",
	"?$#?",
	"#@@$",
	"?##?",
	"",
	"PATTERN",
	"3 4",
	"?$$?",
	"#@@$",
	"?##?",
	"",
	"PATTERN ",
	"4 4",
	"?$#?",
	"#@@$",
	"#@@#",
	"?##?",
	"EOF"
};

int textfile_pos;
int textfile_pull_mode;

int textfile_fgets(char *s, int max_size)
{
	if (textfile_pull_mode == 0)
		strncpy(s, deadlock_patterns_data[textfile_pos], max_size);
	else
		strncpy(s, pull_deadlock_patterns_data[textfile_pos], max_size);

	if (strncmp(s, "EOF", 3) == 0)
		return 0;

	textfile_pos++;

	if (textfile_pos > 200)
		exit_with_error("textfile overflow");

	return 1;
}

void open_text_file(int pull_mode)
{
	textfile_pos = 0;
	textfile_pull_mode = pull_mode;
}
