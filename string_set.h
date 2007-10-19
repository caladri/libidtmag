#ifndef	STRING_SET_H
#define	STRING_SET_H

struct string_set;

typedef	void string_set_iterator_t(void *, const char *);

struct string_set *string_set_create(void);
void string_set_free(struct string_set *);

void string_set_add(struct string_set *, const char *);
void string_set_foreach(struct string_set *, string_set_iterator_t *, void *);

#endif /* !STRING_SET_H */
