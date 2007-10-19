#ifndef	EZ_WRITER_H
#define	EZ_WRITER_H

struct card_data;
struct serial_port;

bool ez_writer_initialize(struct serial_port *);
bool ez_writer_read(struct serial_port *, struct card_data *);

#endif /* !EZ_WRITER_H */
