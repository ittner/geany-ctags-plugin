/*
  A little "shell" application to grab the tty name of a console..
  The tty name is written to the file specified in argv[1] of the
  command line. After that the program just runs in a loop that
  calls nanosleep() until some external force causes it to exit.
*/

#include <stdio.h>
#include <time.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	FILE *f;
	char *tty = NULL;
	if (argc != 2)
	{
		return 1;
	}
	if (!isatty(0))
	{
		return 1;
	}
	tty = ttyname(0);
	if (!(tty && *tty))
	{
		return 1;
	}
	f = fopen(argv[1], "w");
	if (!f)
	{
		return 1;
	}
	fprintf(f, "%s", tty);
	if (fclose(f) != 0)
	{
		return 1;
	}
	while (1)
	{
		struct timespec req = { 1, 0 }, rem;
		nanosleep(&req, &rem);
	}
	return 0;
}
