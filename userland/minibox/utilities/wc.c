#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include "include.h"

int cmode, wmode, lmode;

void output(const char * str, size_t lines, size_t words, size_t chars)
{
	if(lmode)
		printf("%d", lines);

	if(wmode)
	{
		if(lmode) putchar(' ');

		printf("%d", words);
	}

	if(cmode)
	{
		if(lmode) putchar(' ');

		printf("%d", chars);
	}

	if(str) printf(" %s", str);

	putchar('\n');
}

void wc(int argc, char ** argv)
{
	int fd = fd_stdin;
	size_t total_chars, total_words, total_lines;
	total_chars = total_words = total_lines = 0;

	if(NULL == *argv)
		goto read_stdin;


	for(;*argv;argv++)
	{
        int new_word = 1;
		size_t chars = 0,
				words = chars,
				lines = chars;

		fd = open(*argv, O_RDONLY, 0);
read_stdin:
		for(char c;read(fd, &c, 1);)
		{
			if(cmode)
				chars++;

			if(wmode)
			{
                if(isspace(c)) {
                    new_word = 1;
                } else if(new_word) {
                    words++;
                    new_word = 0;
                }
			}

			if(lmode)
				if('\n' == c)
					lines++;
		}
		close(fd);
		if(lmode) total_lines += lines;
		if(wmode) total_words += words;
		if(cmode) total_chars += chars;

		output(*argv, lines, words, chars);
		if(NULL == *argv) return; // We read from stdin
	}

	if(argc > 1) output("total", total_lines, total_words, total_chars);
}

int wc_main(int argc, char ** argv)
{
	cmode = wmode = lmode = 0;
	for(argc--;*++argv && '-' == **argv;argc--)
		switch(*(*argv+1))
		{
		case 'l':
			lmode = 1;
			break;
		case 'w':
			wmode = 1;
			break;
		case 'c':
			cmode = 1;
			break;
		}

	if(!(lmode || cmode || wmode))
		cmode = wmode = lmode = 1;

	wc(argc, argv);
	return 0;
}
