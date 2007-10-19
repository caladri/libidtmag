#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>

#include "ez_writer.h"
#include "serial.h"

#define	EZ_WRITER_ESCAPE	'\x1b'

static const char ez_writer_read_ascii_string[] = {
	EZ_WRITER_ESCAPE,
	'r'
};

static const char ez_writer_reset_buffer_string[] = {
	EZ_WRITER_ESCAPE,
	'a'
};

static const char ez_writer_test_string[] = {
	EZ_WRITER_ESCAPE,
	'e'
};

#define	EZ_WRITER_READ(sport, buf)					\
	serial_port_read(sport, buf, sizeof buf / sizeof buf[0])

#define	EZ_WRITER_WRITE(sport, buf)					\
	serial_port_write(sport, buf, sizeof buf / sizeof buf[0])

static bool ez_writer_reset_buffer(struct serial_port *);
static bool ez_writer_test(struct serial_port *);

bool
ez_writer_initialize(struct serial_port *sport)
{
	if (!ez_writer_reset_buffer(sport))
		return (false);

	if (!ez_writer_test(sport))
		return (false);

	if (!ez_writer_reset_buffer(sport))
		return (false);

	return (true);
}

static bool
ez_writer_reset_buffer(struct serial_port *sport)
{
	if (!EZ_WRITER_WRITE(sport, ez_writer_reset_buffer_string))
		return (false);

	sleep(1);

	return (true);
}

static bool
ez_writer_test(struct serial_port *sport)
{
	char test_response[2];

	if (!EZ_WRITER_WRITE(sport, ez_writer_test_string))
		return (false);

	if (!EZ_WRITER_READ(sport, test_response))
		return (false);

	if (test_response[0] != EZ_WRITER_ESCAPE || test_response[1] != 'y')
		return (false);
	return (true);
}
