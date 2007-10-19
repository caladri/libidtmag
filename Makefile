PROG=	idt_test
SRCS+=	${PROG}.c
SRCS+=	card_data.c
SRCS+=	ez_writer.c
SRCS+=	serial.c
SRCS+=	string_set.c
NOMAN=	t
WARNS=	3

.include <bsd.prog.mk>

CFLAGS:=${CFLAGS:N-Wsystem-headers}
