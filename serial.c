#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"
#include "string_set.h"

#define	SERIAL_DEVICE_DIRECTORY	"/dev"
#define	SERIAL_DEVICE_REGEX	"^cu\\.(.+)$"
#define	SERIAL_DEVICE_FORMAT	SERIAL_DEVICE_DIRECTORY "/cu.%s"

bool
serial_port_open(struct serial_port *sport, const char *name)
{
	struct termios control;
	char path[MAXPATHLEN];
	int error;
	int fd;

	sport->sp_fd = -1;

	snprintf(path, sizeof path, SERIAL_DEVICE_FORMAT, name);
	fd = open(path, O_RDWR | /*O_NONBLOCK | */O_NOCTTY);
	if (fd == -1)
		return (false);
	sport->sp_fd = fd;

	error = ioctl(sport->sp_fd, TIOCEXCL);
	if (error != 0) {
		close(sport->sp_fd);
		sport->sp_fd = -1;
		return (false);
	}

	error = tcgetattr(sport->sp_fd, &control);
	if (error != 0) {
		close(sport->sp_fd);
		sport->sp_fd = -1;
		return (false);
	}

	cfmakeraw(&control);

	error = cfsetspeed(&control, B9600);
	if (error != 0) {
		close(sport->sp_fd);
		sport->sp_fd = -1;
		return (false);
	}

	control.c_cflag &= ~PARENB;
	control.c_cflag &= ~CSTOPB;
	control.c_cflag &= ~CSIZE;
	control.c_cflag |= CS8;
	control.c_cflag |= CLOCAL;

	control.c_iflag |= IXON | IXOFF;

	control.c_cc[VMIN] = 1;
	control.c_cc[VTIME] = 0;

	error = tcsetattr(sport->sp_fd, TCSAFLUSH, &control);
	if (error != 0) {
		close(sport->sp_fd);
		sport->sp_fd = -1;
		return (false);
	}

	return (true);
}

bool
serial_port_read(struct serial_port *sport, char *buf, size_t len)
{
	ssize_t rv;

	if (sport->sp_fd == -1)
		return (false);

	rv = read(sport->sp_fd, buf, len);
	if (rv == -1)
		return (false);
	if (len != (size_t)rv)
		return (serial_port_read(sport, buf + rv, len - rv));
	return (true);
}

bool
serial_port_write(struct serial_port *sport, const char *buf, size_t len)
{
	ssize_t rv;

	if (sport->sp_fd == -1)
		return (false);

	rv = write(sport->sp_fd, buf, len);
	if (rv == -1)
		return (false);
	if (len != (size_t)rv)
		return (false);
	return (true);
}

struct string_set *
serial_port_enumerate(void)
{
	struct string_set *ss;
	struct dirent *de;
	regex_t regex;
	int error;
	DIR *dir;

	error = regcomp(&regex, SERIAL_DEVICE_REGEX, REG_EXTENDED);
	if (error != 0)
		return (NULL);

	dir = opendir(SERIAL_DEVICE_DIRECTORY);
	if (dir == NULL) {
		regfree(&regex);
		return (NULL);
	}

	ss = string_set_create();
	if (ss == NULL) {
		regfree(&regex);
		closedir(dir);
		return (NULL);
	}

	while ((de = readdir(dir)) != NULL) {
		regmatch_t rm[2];

		error = regexec(&regex, de->d_name, sizeof rm / sizeof rm[0],
				rm, 0);
		if (error == REG_NOMATCH)
			continue;
		if (error != 0) {
			abort(); /* XXX Be more useful than that.  */
			continue;
		}
		de->d_name[rm[1].rm_eo] = '\0';
		string_set_add(ss, de->d_name + rm[1].rm_so);
	}

	/*
	 * Note that we return an empty string set if there were no appropriate
	 * devices found, while we return NULL if there was an error.
	 */
	regfree(&regex);
	closedir(dir);
	return (ss);
}
