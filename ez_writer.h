#ifndef	EZ_WRITER_H
#define	EZ_WRITER_H

struct card_data;
struct serial_port;

bool ez_writer_initialize(struct serial_port *);
bool ez_writer_erase(struct serial_port *, unsigned);
bool ez_writer_read(struct serial_port *, struct card_data *);
bool ez_writer_write(struct serial_port *, bool, const struct card_data *);

#endif /* !EZ_WRITER_H */
