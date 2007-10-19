#include <sys/types.h>
#include <stdio.h>

#include "card_data.h"

static void card_data_dump_track(const char *, const char *, size_t);

void
card_data_dump(struct card_data *cdata)
{
	printf("ISO Card Data:\n");
	card_data_dump_track("Track 1", cdata->cd_track1,
			     sizeof cdata->cd_track1 / sizeof cdata->cd_track1[0]);
	card_data_dump_track("Track 2", cdata->cd_track2,
			     sizeof cdata->cd_track2 / sizeof cdata->cd_track2[0]);
	card_data_dump_track("Track 3", cdata->cd_track3,
			     sizeof cdata->cd_track3 / sizeof cdata->cd_track3[0]);
}

static void
card_data_dump_track(const char *name, const char *data, size_t len)
{
	if (data[0] == '\0')
		return;
	printf("%s (ASCII)\t%.*s\n", name, (int)len, data);
}
