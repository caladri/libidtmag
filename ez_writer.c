#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "card_data.h"
#include "ez_writer.h"
#include "serial.h"

#define	EZ_WRITER_ESCAPE	'\x1b'

static const char ez_writer_coercivity_high_string[] = {
	EZ_WRITER_ESCAPE,
	'x'
};

static const char ez_writer_coercivity_low_string[] = {
	EZ_WRITER_ESCAPE,
	'y'
};

static const char ez_writer_erase_string[] = {
	EZ_WRITER_ESCAPE,
	'c'
};

static const char ez_writer_present_string[] = {
	'9'
};

static const char ez_writer_ram_test_string[] = {
	EZ_WRITER_ESCAPE,
	'\x87'
};

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

static const char ez_writer_version_string[] = {
	EZ_WRITER_ESCAPE,
	'u'
};

static const char ez_writer_write_ascii_string[] = {
	EZ_WRITER_ESCAPE,
	'w'
};

#define	EZ_WRITER_READ(sport, buf)					\
	serial_port_read(sport, buf, sizeof buf / sizeof buf[0])

#define	EZ_WRITER_WRITE(sport, buf)					\
	serial_port_write(sport, buf, sizeof buf / sizeof buf[0])

static bool ez_writer_present(struct serial_port *);
static bool ez_writer_ram_test(struct serial_port *);
static bool ez_writer_read_track(struct serial_port *, char *, char *);
static bool ez_writer_reset_buffer(struct serial_port *);
static bool ez_writer_test(struct serial_port *);
static bool ez_writer_write_track(struct serial_port *, unsigned, const char *, size_t);

bool
ez_writer_initialize(struct serial_port *sport)
{
	if (!ez_writer_present(sport))
		return (false);

	if (!ez_writer_reset_buffer(sport))
		return (false);

	if (!ez_writer_test(sport))
		return (false);

	if (!ez_writer_ram_test(sport))
		return (false);

	if (!ez_writer_reset_buffer(sport))
		return (false);

	return (true);
}

bool
ez_writer_erase(struct serial_port *sport, unsigned mask)
{
	char erase_ports[1];
	char erase_response[2];

	/*
	 * We only have three tracks.  If the user thinks otherwise, they
	 * are a fool.
	 */
	if ((mask & ~(EZ_WRITER_TRACK_TO_BITMASK(1) |
		      EZ_WRITER_TRACK_TO_BITMASK(2) |
		      EZ_WRITER_TRACK_TO_BITMASK(3))) != 0)
		return (false);
	/*
	 * What could we possibly do to no tracks?
	 */
	if (mask == 0)
		return (false);

	if (!EZ_WRITER_WRITE(sport, ez_writer_erase_string))
		return (false);

	erase_ports[0] = mask;

	if (!EZ_WRITER_WRITE(sport, erase_ports))
		return (false);

	if (!EZ_WRITER_READ(sport, erase_response))
		return (false);

	if (erase_response[0] != EZ_WRITER_ESCAPE || erase_response[1] != '0')
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
			case 1:
				if (!ez_writer_read_track(sport,
							  cdata->cd_track1,
							  tuple))
					return (false);
				break;
			case 2:
				if (!ez_writer_read_track(sport,
							  cdata->cd_track2,
							  tuple))
					return (false);
				break;
			case 3:
				if (!ez_writer_read_track(sport,
							  cdata->cd_track3,
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
			 * Read status tuple.
			 */
			if (!EZ_WRITER_READ(sport, tuple))
				return (false);
			switch (tuple[0]) {
			case EZ_WRITER_ESCAPE:
				switch (tuple[1]) {
				case '0':
					return (true);
				default:
					return (false);
				}
			default:
				return (false);
			}
		default:
			return (false);
		}
	}
}

bool
ez_writer_version(struct serial_port *sport, char *buf, size_t len)
{
	char version_response[EZ_WRITER_VERSION_LENGTH];

	if (len != EZ_WRITER_VERSION_LENGTH + 1)
		return (false);

	if (!EZ_WRITER_WRITE(sport, ez_writer_version_string))
		return (false);

	if (!EZ_WRITER_READ(sport, version_response))
		return (false);

	memcpy(buf, version_response, EZ_WRITER_VERSION_LENGTH);
	buf[EZ_WRITER_VERSION_LENGTH] = '\0';
	return (true);
}

bool
ez_writer_write(struct serial_port *sport, bool hico, const struct card_data *cdata)
{
	static const char data_block_begin[] = {
		EZ_WRITER_ESCAPE, 's'
	};
	static const char data_block_end[] = {
		'?', '\x1c'
	};
	char write_response[2];
	char coercivity_response[2];

	if (hico) {
		if (!EZ_WRITER_WRITE(sport, ez_writer_coercivity_high_string))
			return (false);
	} else {
		if (!EZ_WRITER_WRITE(sport, ez_writer_coercivity_low_string))
			return (false);
	}

	if (!EZ_WRITER_READ(sport, coercivity_response))
		return (false);

	if (coercivity_response[0] != EZ_WRITER_ESCAPE ||
	    coercivity_response[1] != '0')
		return (false);

	if (!EZ_WRITER_WRITE(sport, ez_writer_write_ascii_string))
		return (false);

	if (!EZ_WRITER_WRITE(sport, data_block_begin))
		return (false);

	if (!ez_writer_write_track(sport, '\x1', cdata->cd_track1,
				   sizeof cdata->cd_track1 / sizeof cdata->cd_track1[0]))
		return (false);

	if (!ez_writer_write_track(sport, '\x2', cdata->cd_track2,
				   sizeof cdata->cd_track2 / sizeof cdata->cd_track2[0]))
		return (false);

	if (!ez_writer_write_track(sport, '\x3', cdata->cd_track3,
				   sizeof cdata->cd_track3 / sizeof cdata->cd_track3[0]))
		return (false);

	if (!EZ_WRITER_WRITE(sport, data_block_end))
		return (false);

	if (!EZ_WRITER_READ(sport, write_response))
		return (false);

	if (write_response[0] != EZ_WRITER_ESCAPE || write_response[1] != '0')
		return (false);
	return (true);
}

static bool
ez_writer_present(struct serial_port *sport)
{
	char present_response[2];

	if (!EZ_WRITER_WRITE(sport, ez_writer_present_string))
		return (false);

	if (!EZ_WRITER_READ(sport, present_response))
		return (false);

	if (present_response[0] != EZ_WRITER_ESCAPE ||
	    present_response[1] != '4')
		return (false);
	return (true);
}

static bool
ez_writer_ram_test(struct serial_port *sport)
{
	char test_response[2];

	if (!EZ_WRITER_WRITE(sport, ez_writer_ram_test_string))
		return (false);

	if (!EZ_WRITER_READ(sport, test_response))
		return (false);

	if (test_response[0] != EZ_WRITER_ESCAPE || test_response[1] != '0')
		return (false);
	return (true);
}

static bool
ez_writer_read_track(struct serial_port *sport, char *track, char *tuple)
{
	char byte[1];

	tuple[0] = tuple[1] = '\0';

	for (;;) {
		if (!EZ_WRITER_READ(sport, byte))
			return (false);

		if (byte[0] == EZ_WRITER_ESCAPE) {
			/*
			 * We've hit an escape, which is probably indicating
			 * that this track is empty.  Make sure that that's
			 * the case, and then return the next tuple.
			 */
			if (!EZ_WRITER_READ(sport, byte))
				return (false);
			if (byte[0] != '*')
				return (false);
			goto next_tuple;
		}

		*track++ = byte[0];

		if (byte[0] == '?') {
			/*
			 * We've hit the terminator for this track.  Return the
			 * next tuple.
			 */
			goto next_tuple;
		}
	}

next_tuple:
	if (!EZ_WRITER_READ(sport, byte))
		return (false);
	tuple[0] = byte[0];

	if (!EZ_WRITER_READ(sport, byte))
		return (false);
	tuple[1] = byte[0];

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

static bool
ez_writer_write_track(struct serial_port *sport, unsigned track, const char *trackdata, size_t len)
{
	char track_begin[] = {
		EZ_WRITER_ESCAPE, track & (EZ_WRITER_TRACK_TO_BITMASK(1) |
					   EZ_WRITER_TRACK_TO_BITMASK(2) |
					   EZ_WRITER_TRACK_TO_BITMASK(3))
	};

	if (!EZ_WRITER_WRITE(sport, track_begin))
		return (false);

	/*
	 * If we are not using up a complete field, set the len to the length
	 * we are using.  Note that we are requiring the trackdata to be ASCII
	 * NUL terminated.
	 */
	if (memchr(trackdata, '\0', len) != NULL)
		len = strlen(trackdata);

	/*
	 * If this track is empty, write nothing, rather than ^[*, which is
	 * what comes on read for a null track.
	 */
	if (len == 0)
		return (true);

	/*
	 * This is ridiculous.
	 *
	 * A read will give the start and end characters, but they do not seem
	 * to be necessary (indeed, they are not wanted) on write.  Awful.
	 *
	 * Compensate by skipping the start and end characters.
	 */
	if (!serial_port_write(sport, trackdata + 1, len - 2))
		return (false);

	return (true);
}
