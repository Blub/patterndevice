#define FUSE_USE_VERSION 29

#include <cuse_lowlevel.h>
#include <fuse_opt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>

#include "patterndevice.h"

/*********************************************************************/
/* Usage information
 */

static const char *arg0;

struct pattern_dev_param {
	unsigned major;
	unsigned minor;
	char     *dev_name;
	int      is_help;
};

static const struct fuse_opt copts[] = {
#define COPT(t, p) { t, offsetof(struct pattern_dev_param, p), 1 }
	COPT("-M %u", major),
	COPT("--maj=%u", major),
	COPT("-m %u", minor),
	COPT("--min=%u", minor),
	COPT("-n %s", dev_name),
	COPT("--name=%s", dev_name),
	FUSE_OPT_KEY("-h", 0),
	FUSE_OPT_KEY("--help", 0),
#undef COPT
	FUSE_OPT_END
};

static void usage(const char *arg0, FILE *target, int exitstatus)
{
	fprintf(target,
	"usage: %s [options]\n"
	"  -h, --help       show this message\n"
	"  -n, --name=NAME  device name (mandatory)\n"
	"  -M, --maj=MAJ    major device number\n"
	"  -m, --min=MIN    minor device number\n"
	, arg0);
	if (exitstatus >= 0)
		exit(exitstatus);
}

/*********************************************************************/
/* Internal data
 */

struct open_handle {
	//fuse_req_t req;
	uint64_t fh;
	struct open_handle *next;
	char   *data;
	size_t size;
	off_t  off;
	char   keep_offset;
};
typedef struct open_handle handle_t;

static uint64_t hids = 0;
static handle_t *handles = NULL;
static pthread_mutex_t hmutex = PTHREAD_MUTEX_INITIALIZER;

static handle_t* handle_new()
{
	handle_t *nh;
	nh = (handle_t*)malloc(sizeof(*nh));
	if (!nh)
		return NULL;

	nh->fh = 0;
	nh->next = NULL;
	nh->data = NULL;
	nh->size = 0;
	nh->off  = 0;
	nh->keep_offset = 0;

	return nh;
}

static uint64_t patdev_file_insert(handle_t *nh)
{
	pthread_mutex_lock(&hmutex);
	nh->fh = ++hids;
	nh->next = handles;
	handles = nh;
	pthread_mutex_unlock(&hmutex);
	return nh->fh;
}

static handle_t* patdev_file_get(uint64_t fh)
{
	handle_t *h;
	pthread_mutex_lock(&hmutex);
	for (h = handles; h && h->fh != fh; h = h->next);
	pthread_mutex_unlock(&hmutex);
	return h;
}

/*********************************************************************/
/* File Operations
 */

/* Open */
static void patdev_open(fuse_req_t req, struct fuse_file_info *fi)
{
	handle_t *nh = handle_new();
	if (!nh) {
		fuse_reply_err(req, ENOMEM);
		return;
	}

	fi->fh = patdev_file_insert(nh);

	fuse_reply_open(req, fi);
}

/* Close */
static void patdev_close(fuse_req_t req, struct fuse_file_info *fi)
{
	handle_t **next, *h;

	pthread_mutex_lock(&hmutex);
	for (next = &handles; *next; next = &( (*next)->next )) {
		if ((*next)->fh == fi->fh)
			break;
	}
	if (*next) {
		h = *next;
		*next = h->next;
		free(h);
	}
	pthread_mutex_unlock(&hmutex);
	fuse_reply_err(req, 0);
}

/* Write */
static void patdev_write(fuse_req_t req, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
{
	handle_t *h;
	char *localbuf;

	// A zero-size write is invalid
	if (!size) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	// try allocating and copying first and then do the mutex crap
	localbuf = (char*)malloc(size);
	if (!localbuf) {
		fuse_reply_err(req, ENOMEM);
		return;
	}
	memcpy(localbuf, buf, size);

	// Find the entry
	h = patdev_file_get(fi->fh);

	// If there is no entry, return ENOENT
	if (!h) {
		free(localbuf);
		fuse_reply_err(req, ENOENT);
		return;
	}

	// if there's data already, return EINVAL
	if (h->data) {
		free(localbuf);
		fuse_reply_err(req, EINVAL);
		return;
	}

	// finally: store the data
	h->data = localbuf;
	h->size = size;

	/*
	// Repeat the pattern to fill the expanded buffer
	if (size < sizeof(h->expanded))
	{
		size_t o;
		size_t rem = sizeof(h->expanded) % size;
		size_t full = sizeof(h->expanded) - rem;
		for (o = 0; o < full; o += size) {
			memcpy(h->expanded + o, buf, size);
		}
		if (rem)
			memcpy(h->expanded + o, buf, rem);
	}
	*/
	fuse_reply_write(req, size);
}

/* Read */
static void patdev_read(fuse_req_t req, size_t size, off_t off, struct fuse_file_info *fi)
{
	handle_t *h;
	char *localbuf;
	size_t left;
	size_t inoff;

	// Find
	h = patdev_file_get(fi->fh);

	// If there is no entry, return ENOENT
	if (!h) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	off += h->off;

	inoff = off % h->size;
	left = h->size - inoff;
	if (size <= left) {
		fuse_reply_buf(req, h->data + inoff, size);
		if (!h->keep_offset)
			h->off = (h->off + size) % h->size;
		return;
	}

	localbuf = (char*)malloc(size);
	if (!localbuf) {
		fuse_reply_err(req, ENOMEM);
		return;
	}

	// first, the partial chunk at the beginning
	memcpy(localbuf, h->data + inoff, left);
	// now, repeat from the start
	{
		size_t written = left;
		while (written < size)
		{
			if (size - written < h->size)
			{
				// last chunk
				memcpy(localbuf + written, h->data, size-written);
				fuse_reply_buf(req, localbuf, size);
				free(localbuf);
				if (!h->keep_offset)
					h->off = (h->off + size) % h->size;
				return;
			}
			memcpy(localbuf + written, h->data, h->size);
			written += h->size;
		}
		fuse_reply_buf(req, localbuf, size);
		free(localbuf);
		if (!h->keep_offset)
			h->off = (h->off + size) % h->size;
	}
}

/* Poll - writable at the beginning, then only readable */
static void patdev_poll(fuse_req_t req, struct fuse_file_info *fi, struct fuse_pollhandle *ph)
{
	handle_t *h;
	(void)fi;

	// Find
	h = patdev_file_get(fi->fh);

	// If there is no entry, return ENOENT
	if (!h) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	if (h->data)
		fuse_reply_poll(req, POLLIN);
	else
		fuse_reply_poll(req, POLLOUT);
}

/* IOCTL to change read behaviour */
static void patdev_ioctl(fuse_req_t req, int cmd, void *arg,
                         struct fuse_file_info *fi, unsigned flags,
                         const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{
	handle_t *h;
	if (flags & FUSE_IOCTL_COMPAT) {
		fuse_reply_err(req, ENOSYS);
		return;
	}

	switch (cmd) {
		case FIOC_GET_KEEP_OFFSET:
			if (!out_bufsz) {
				struct iovec iov = { arg, sizeof(char) };
				fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
			} else {
				h = patdev_file_get(fi->fh);
				if (!h) {
					fuse_reply_err(req, ENOENT);
					return;
				}
				fuse_reply_ioctl(req, 0, &h->keep_offset, sizeof(h->keep_offset));
			}
			break;
		case FIOC_SET_KEEP_OFFSET:
			if (!in_bufsz) {
				struct iovec iov = { arg, sizeof(char) };
				fuse_reply_ioctl_retry(req, &iov, 1, NULL, 0);
			} else {
				h = patdev_file_get(fi->fh);
				if (!h) {
					fuse_reply_err(req, ENOENT);
					return;
				}
				h->keep_offset = (*(char*)in_buf) ? 1 : 0;
				fuse_reply_ioctl(req, 0, NULL, 0);
			}
			break;
		default:
			fuse_reply_err(req, EINVAL);
			break;
	}
}

static const struct cuse_lowlevel_ops pattern_dev_ops = {
	.open  = patdev_open,
	.read  = patdev_read,
	.write = patdev_write,
	.release = patdev_close,
	.ioctl = patdev_ioctl,
	.poll  = patdev_poll,
};

/*********************************************************************/
/* Main
 */

static int process_arg(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	struct pattern_dev_param *param = data;
	// ignore those:
	(void) outargs;
	(void) arg;

	if (key)
		return 1;
	param->is_help = 1;
	// for now exit with usage:
	usage(arg0, stderr, 1);
	return fuse_opt_add_arg(outargs, "-ho");
}

int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct pattern_dev_param param = { 0, 0, NULL, 0 };
	char   dev_name[128] = "DEVNAME=";
	const char *dev_info_argv[] = { dev_name };
	struct cuse_info ci;

	arg0 = argv[0];

	if (fuse_opt_parse(&args, &param, copts, process_arg)) {
		fprintf(stderr, "failed to parse option\n");
		exit(1);
	}

	if (param.is_help)
		usage(arg0, stdout, 0);
	if (!param.dev_name)
		usage(arg0, stderr, 1);

	strncat(dev_name, param.dev_name, sizeof(dev_name)-9);

	memset(&ci, 0, sizeof(ci));
	ci.dev_major = param.major;
	ci.dev_minor = param.minor;
	ci.dev_info_argc = 1;
	ci.dev_info_argv = dev_info_argv;
	ci.flags = CUSE_UNRESTRICTED_IOCTL;

	return cuse_lowlevel_main(args.argc, args.argv, &ci, &pattern_dev_ops, NULL);
}
