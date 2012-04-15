#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int binyes()
{
	char *buf = NULL;
	size_t alloced = 0;
	size_t size = 0;
	char chunk[4096];
	ssize_t rd;

	int rval = 0;

	while (1)
	{
		rd = read(0, chunk, sizeof(chunk));
		if (rd == -1) {
			if (errno == EINTR)
				continue;
			// check for EAGAIN?
			perror("read");
			break;
		}
		if (rd == 0) // eof
			break;
		if (size + rd > alloced) {
			alloced = alloced ? 2*alloced : sizeof(chunk);
			buf = realloc(buf, alloced);
		}
		memcpy(buf + size, chunk, rd);
		size += rd;
	}

	while (1)
	{
		rd = write(1, buf, size);
		if (rd < 0) {
			if (errno == EINTR)
				continue;
			perror("write");
			rval = 1;
			break;
		}
		if (rd == 0) // eof
			break;
	}
	return rval;
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		FILE *target = stderr;
		int retval = 1;

		if (!strcmp(argv[1], "-h") ||
		    !strcmp(argv[1], "--help"))
		{
			target = stdout;
		    	retval = 0;
		}

		fprintf(target,
		"usage: %s\n"
		"no parameters\n"
		, argv[0]);
		exit(retval);
	}
	return binyes();
}
