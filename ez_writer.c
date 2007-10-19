#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "card_data.h"
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

static bool ez_writer_read_until(struct serial_port *, char *, char, char, char *);
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

bool
ez_writer_read(struct serial_port *sport, struct card_data *cdata)
{
	char tuple[2];

	memset(cdata, '\0', sizeof *cdata);

	if (!EZ_WRITER_WRITE(sport, ez_writer_read_ascii_string))
		return (false);

	/*
	 * Read data block.
	 */
	if (!EZ_WRITER_READ(sport, tuple))
		return (false);

	if (tuple[0] != EZ_WRITER_ESCAPE || tuple[1] != 's')
		return (false);

	if (!EZ_WRITER_READ(sport, tuple))
		return (false);

	for (;;) {
		switch (tuple[0]) {
		case EZ_WRITER_ESCAPE:
			switch (tuple[1]) {
			case '1':
				if (!ez_writer_read_until(sport,
							  cdata->cd_track1,
							  EZ_WRITER_ESCAPE, '?',
							  tuple))
					return (false);
				break;
			case '2':
				if (!ez_writer_read_until(sport,
							  cdata->cd_track2,
							  EZ_WRITER_ESCAPE, '?',
							  tuple))
					return (false);
				break;
			case '3':
				if (!ez_writer_read_until(sport,
							  cdata->cd_track3,
							  EZ_WRITER_ESCAPE, '?',
							  tuple))
					return (false);
				break;
			default:
				return (false);
			}
			break;
		case '?':
			if (tuple[1] != '\x1c')
				return (false);
			/*
			 * Read status.
			 */
			return (true);
		default:
			return (false);
		}
	}
}

static bool
ez_writer_read_until(struct serial_port *sport, char *track, char end1, char end2, char *tuple)
{
	char byte[1];

	for (;;) {
		if (!EZ_WRITER_READ(sport, byte))
			return (false);
		if (byte[0] == end1 || byte[0] == end2) {
			tuple[0] = byte[0];
			if (!EZ_WRITER_READ(sport, byte))
				return (false);
			tuple[1] = byte[0];
			return (true);
		}
		*track++ = byte[0];
	}
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
