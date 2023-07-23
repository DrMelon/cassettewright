#ifndef CASSETTEWRIGHT_H
#define CASSETTEWRIGHT_H

void write_polarity_sync();
void write_lead_in();
void write_header();
void write_bit_as_pcm(int bit);
void write_byte_as_pcm(int byte);
void write_positive_cycle();
void write_negative_cycle();

void read_pcm_sample(int pcm_sample);

#define PCM_SAMPLES_PER_BIT 16
#define POLARITY_SYNC_PATTERN_NUM_POS 1
#define POLARITY_SYNC_PATTERN_NUM_NEG 3
#define POLARITY_SYNC_DESIRED_COUNT 10
#define POLARITY_SYNC_WRITE_COUNT 200
#define POLARITY_SYNC_CHECK_WINDOW 200
#define LEAD_IN_NUM_BYTES 16

#endif //CASSETTEWRIGHT_H