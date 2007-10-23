#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "card_data.h"
#include "ez_writer.h"
#include "serial.h"
#include "string_set.h"

struct pick_context {
	unsigned long pc_pick;
	unsigned long pc_counter;
	const char *pc_name;
};

struct print_context {
	unsigned long pc_counter;
	FILE *pc_handle;
};

static bool choose_serial_port(struct serial_port *, const char *);
static void pick_serial_port(void *, const char *);
static void print_serial_port(void *, const char *);

int
main(int argc, char *argv[])
{
	char version[EZ_WRITER_VERSION_LENGTH + 1];
	struct serial_port sport;
	bool doread, dowrite, doerase;
	struct card_data cdata;
	const char *portname;
	int ch;

	memset(&cdata, 0, sizeof cdata);
	doread = dowrite = doerase = false;
	portname = NULL;

	while ((ch = getopt(argc, argv, "1:2:3:rwe?")) != -1) {
		switch (ch) {
		case '1':
			strlcpy(cdata.cd_track1, optarg, sizeof cdata.cd_track1);
			break;
		case '2':
			strlcpy(cdata.cd_track2, optarg, sizeof cdata.cd_track2);
			break;
		case '3':
			strlcpy(cdata.cd_track3, optarg, sizeof cdata.cd_track3);
			break;
		case 'r':
			doread = true;
			break;
		case 'w':
			dowrite = true;
			break;
		case 'e':
			doerase = true;
			break;
		case '?':
		default: /* XXX usage */
			return (1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 1) {
		argc--;
		portname = *argv++;
	}

	if (argc != 0) /* XXX usage */
		return (1);

	if (!choose_serial_port(&sport, portname)) {
		fprintf(stderr, "Unable to attach serial port.\n");
		return (1);
	}

	if (!ez_writer_initialize(&sport)) {
		fprintf(stderr, "Unable to initialize EZ Writer.\n");
		return (1);
	}
	fprintf(stderr, "Initialized EZ Writer.\n");

	if (!ez_writer_version(&sport, version, EZ_WRITER_VERSION_LENGTH + 1)) {
		fprintf(stderr, "Unable to get EZ Writer version.\n");
		return (1);
	}
	fprintf(stderr, "Version: %.*s\n", EZ_WRITER_VERSION_LENGTH, version);

	if (doread) {
		fprintf(stderr,
			"Swipe a card to read when the LED changes color.\n");
		if (!ez_writer_read(&sport, &cdata)) {
			fprintf(stderr, "Failed to read a card.\n");
			return (1);
		}
		card_data_dump(&cdata);
	}

	if (doerase) {
		fprintf(stderr,
			"Swipe a card to erase when the LED changes color.\n");
		if (!ez_writer_erase(&sport,
				     EZ_WRITER_TRACK_TO_BITMASK(1) |
				     EZ_WRITER_TRACK_TO_BITMASK(2) |
				     EZ_WRITER_TRACK_TO_BITMASK(3))) {
			fprintf(stderr, "Failed to erase a card.\n");
			return (1);
		}
	}

	if (dowrite) {
		card_data_dump(&cdata);
		fprintf(stderr,
			"Swipe a card to write data to.\n");
		if (!ez_writer_write(&sport, true, &cdata)) {
			fprintf(stderr, "Failed to write a card.\n");
			return (1);
		}
	}

	return (0);
}

static bool
choose_serial_port(struct serial_port *sport, const char *portname)
{
	struct string_set *serial_ports;
	struct print_context pc;
	struct pick_context pk;
	char *line, *end;
	size_t len;

	pc.pc_counter = 0;
	pc.pc_handle = stdout;

	if (portname != NULL) {
		serial_ports = NULL;
		goto open_port;
	}

	serial_ports = serial_port_enumerate();
	string_set_foreach(serial_ports, print_serial_port, &pc);

	if (pc.pc_counter == 0) {
		fprintf(pc.pc_handle, "Sorry, no ports available.\n");
		string_set_free(serial_ports);
		return (false);
	}

	pk.pc_counter = 0;
	pk.pc_name = NULL;
	pk.pc_pick = 0;

	for (;;) {
		fprintf(pc.pc_handle, "Please enter the number corresponding with the port you would like to use.\n");
		line = fgetln(stdin, &len);
		if (line == NULL)
			continue;
		line[len - 1] = '\0';
		if (line[0] == '\0')
			continue;
		pk.pc_pick = strtoul(line, &end, 10);
		if (*end != '\0') {
			pk.pc_pick = 0;
			continue;
		}
		if (pk.pc_pick < 1 || pk.pc_pick > pc.pc_counter) {
			pk.pc_pick = 0;
			continue;
		}
		break;
	}

	string_set_foreach(serial_ports, pick_serial_port, &pk);
	if (pk.pc_name == NULL) {
		string_set_free(serial_ports);
		return (false);
	}

	portname = pk.pc_name;

open_port:
	fprintf(pc.pc_handle, "Serial port selected: %s\n", portname);

	if (!serial_port_open(sport, portname)) {
		if (serial_ports != NULL)
			string_set_free(serial_ports);
		return (false);
	}
	fprintf(pc.pc_handle, "Serial port opened successfully.\n");
	if (serial_ports != NULL)
		string_set_free(serial_ports);
	return (true);
}

static void
pick_serial_port(void *arg, const char *port)
{
	struct pick_context *pc;

	pc = arg;
	pc->pc_counter++;

	if (pc->pc_counter == pc->pc_pick)
		pc->pc_name = port;
}

static void
print_serial_port(void *arg, const char *port)
{
	struct print_context *pc;

	pc = arg;
	pc->pc_counter++;

	fprintf(pc->pc_handle, "Serial port %lu: %s\n", pc->pc_counter, port);
}
