#ifndef	CARD_DATA_H
#define	CARD_DATA_H

struct card_data {
	char cd_track1[79];
	char cd_track2[40];
	char cd_track3[107];
};

void card_data_dump(struct card_data *);

#endif /* !CARD_DATA_H */
