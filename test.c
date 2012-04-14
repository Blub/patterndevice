#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "src/patterndevice.h"
// <patterndevice.h>

int main()
{
	int fd;
	char buf[] = "abcdefghijklmnopqrstuvwxyz";
	char buf2[32+1];
	ssize_t wr;
	int i;

	printf("Opening /dev/pattern ...\n");
	fd = open("/dev/pattern", O_RDWR);
	if (fd < 0) {
		perror("/dev/pattern");
		exit(1);
	}

	printf("Writing alphabet: %s\n", buf);
	if ( (wr = write(fd, buf, sizeof(buf)-1)) != sizeof(buf)-1) {
		perror("write");
		printf("written %i\n", (int)wr);
		close(fd);
		exit(1);
	}
	printf("written: %i\n", (int)wr);

	memset(buf, 0, sizeof(buf));
	wr = read(fd, buf, sizeof(buf)-1);
	if (wr != sizeof(buf)-1) {
		perror("read");
		close(fd);
		exit(1);
	}
	printf("Trying a full 26 character read...\n");
	printf("First: %s\n", buf);

	for (i = 0; i < 5; ++i) {
		wr = read(fd, buf2, sizeof(buf2)-1);
		buf2[sizeof(buf2)-1] = 0;
		printf("read %i\n", (int)wr);
		printf("%s\n", buf2);
	}

	i = 1;
	if (ioctl(fd, FIOC_SET_KEEP_OFFSET, &i) != 0) {
		perror("ioctl");
		close(fd);
		exit(1);
	}
	i = 0;
	if (ioctl(fd, FIOC_GET_KEEP_OFFSET, &i) != 0) {
		perror("ioctl");
		close(fd);
		exit(1);
	}
	printf("Got: %i\n", i);

	for (i = 0; i < 5; ++i) {
		wr = read(fd, buf2, sizeof(buf2)-1);
		buf2[sizeof(buf2)-1] = 0;
		printf("read %i\n", (int)wr);
		printf("%s\n", buf2);
	}

	printf("closing\n");
	close(fd);

	exit(0);
	return 0;
}
