#ifndef	SERIAL_H
#define	SERIAL_H

struct serial_port;
struct string_set;

struct serial_port {
	int sp_fd;
};

bool serial_port_open(struct serial_port *, const char *);
void serial_port_close(struct serial_port *);
bool serial_port_read(struct serial_port *, char *, size_t);
bool serial_port_write(struct serial_port *, const char *, size_t);
struct string_set *serial_port_enumerate(void);

#endif /* !SERIAL_H */
