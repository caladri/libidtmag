#include <sys/types.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>

#include "string_set.h"

struct string_set_item;

struct string_set {
	STAILQ_HEAD(, string_set_item) ss_items;
};

struct string_set_item {
	char *ssi_string;
	STAILQ_ENTRY(string_set_item) ssi_link;
};

struct string_set *
string_set_create(void)
{
	struct string_set *ss;

	ss = malloc(sizeof *ss);
	if (ss == NULL)
		return (NULL);

	STAILQ_INIT(&ss->ss_items);
	return (ss);
}

void
string_set_free(struct string_set *ss)
{
	struct string_set_item *ssi, *next;

#ifdef	STAILQ_FOREACH_SAFE
	STAILQ_FOREACH_SAFE(ssi, &ss->ss_items, ssi_link, next) {
#else
	ssi = STAILQ_FIRST(&ss->ss_items); 
	while (ssi != NULL) {
		next = STAILQ_NEXT(ssi, ssi_link);
#endif
		STAILQ_REMOVE(&ss->ss_items, ssi, string_set_item, ssi_link);

		free(ssi->ssi_string);
		free(ssi);
#ifndef	STAILQ_FOREACH_SAFE
		ssi = next;
#endif
	}
	free(ss);
}

void
string_set_add(struct string_set *ss, const char *src)
{
	struct string_set_item *ssi;
	char *string;

	ssi = malloc(sizeof *ssi);
	if (ssi == NULL)
		abort();

	string = strdup(src);
	if (string == NULL)
		abort();

	ssi->ssi_string = string;
	STAILQ_INSERT_TAIL(&ss->ss_items, ssi, ssi_link);
}

void
string_set_foreach(struct string_set *ss, string_set_iterator_t *iter, void *a)
{
	struct string_set_item *ssi;

#ifdef	STAILQ_FOREACH
	STAILQ_FOREACH(ssi, &ss->ss_items, ssi_link) {
#else
	for (ssi = STAILQ_FIRST(&ss->ss_items); ssi != NULL; ssi = STAILQ_NEXT(ssi, ssi_link)) {
#endif
		iter(a, ssi->ssi_string);
	}
}
