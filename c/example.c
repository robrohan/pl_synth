#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define PL_SYNTH_IMPLEMENTATION
#include "pl_synth.h"


// WAV writer ------------------------------------------------------------------

#define CHUNK_ID(S) \
	(((unsigned int)(S[3])) << 24 | ((unsigned int)(S[2])) << 16 | \
	 ((unsigned int)(S[1])) <<  8 | ((unsigned int)(S[0])))

void fwrite_u32_le(unsigned int v, FILE *fh) {
	uint8_t buf[sizeof(unsigned int)];
	buf[0] = 0xff & (v      );
	buf[1] = 0xff & (v >>  8);
	buf[2] = 0xff & (v >> 16);
	buf[3] = 0xff & (v >> 24);
	int wrote = fwrite(buf, sizeof(unsigned int), 1, fh);
	assert(wrote);
}

void fwrite_u16_le(unsigned short v, FILE *fh) {
	uint8_t buf[sizeof(unsigned short)];
	buf[0] = 0xff & (v      );
	buf[1] = 0xff & (v >>  8);
	int wrote = fwrite(buf, sizeof(unsigned short), 1, fh);
	assert(wrote);
}

int wav_write(const char *path, short *samples, int samples_len, short channels, int samplerate) {
	unsigned int data_size = samples_len * channels * sizeof(short);
	short bits_per_sample = 16;

	/* Lifted from https://www.jonolick.com/code.html - public domain
	Made endian agnostic using fwrite() */
	FILE *fh = fopen(path, "wb");
	assert(fh);
	fwrite("RIFF", 1, 4, fh);
	fwrite_u32_le(data_size + 44 - 8, fh);
	fwrite("WAVEfmt \x10\x00\x00\x00\x01\x00", 1, 14, fh);
	fwrite_u16_le(channels, fh);
	fwrite_u32_le(samplerate, fh);
	fwrite_u32_le(channels * samplerate * bits_per_sample/8, fh);
	fwrite_u16_le(channels * bits_per_sample/8, fh);
	fwrite_u16_le(bits_per_sample, fh);
	fwrite("data", 1, 4, fh);
	fwrite_u32_le(data_size, fh);
	fwrite((void*)samples, data_size, 1, fh);
	fclose(fh);
	return data_size  + 44 - 8;
}



// Song data -------------------------------------------------------------------

pl_synth_song_t song = {
	.row_len = 8481,
	.num_tracks = 4,
	.tracks = (pl_synth_track_t[]){
		{
			.synth = {7,0,0,0,121,1,7,0,0,0,91,3,0,100,1212,5513,100,0,6,19,3,121,6,21,0,1,1,29,0},
			.sequence_len = 12,
			.sequence = (uint8_t[]){1,2,1,2,1,2,0,0,1,2,1,2},
			.patterns = (pl_synth_pattern_t[]){
				{.notes = {138,145,138,150,138,145,138,150,138,145,138,150,138,145,138,150,136,145,138,148,136,145,138,148,136,145,138,148,136,145,138,148}},
				{.notes = {135,145,138,147,135,145,138,147,135,145,138,147,135,145,138,147,135,143,138,146,135,143,138,146,135,143,138,146,135,143,138,146}}
			},
		},
		{
			.synth = {7,0,0,0,192,1,6,0,9,0,192,1,25,137,1111,16157,124,1,982,89,6,25,6,77,0,1,3,69,0},
			.sequence_len = 12,
			.sequence = (uint8_t[]){0,0,1,2,1,2,3,3,3,3,3,3},
			.patterns = (pl_synth_pattern_t[]){
				{.notes = {138,138,0,138,140,0,141,0,0,0,0,0,0,0,0,0,136,136,0,136,140,0,141}},
				{.notes = {135,135,0,135,140,0,141,0,0,0,0,0,0,0,0,0,135,135,0,135,140,0,141,0,140,140}},
				{.notes = {145,0,0,0,145,143,145,150,0,148,0,146,0,143,0,0,0,145,0,0,0,145,143,145,139,0,139,0,0,142,142}}
			}
		},
		{
			.synth = {7,0,0,1,255,0,7,0,0,1,255,0,0,100,0,3636,174,2,500,254,0,27,0,0,0,0,0,0,0},
			.sequence_len = 12,
			.sequence = (uint8_t[]){1,1,1,1,0,0,1,1,1,1,1,1},
			.patterns = (pl_synth_pattern_t[]){
				{.notes = {135,135,0,135,139,0,135,135,135,0,135,139,0,135,135,135,0,135,139,0,135,135,135,0,135,139,0,135,135,135,0,135}}
			}
		},
		{
			.synth = {8,0,0,1,200,0,7,0,0,0,211,3,210,50,200,6800,153,4,11025,254,6,32,5,61,0,1,4,60,0},
			.sequence_len = 12,
			.sequence = (uint8_t[]){1,1,1,1,0,0,1,1,1,1,1,1},
			.patterns = (pl_synth_pattern_t[]){
				{.notes = {0,0,0,0,140,0,0,0,0,0,0,0,140,0,0,0,0,0,0,0,140,0,0,0,0,0,0,0,140}}
			}
		}
	}
};

// int main(int argc, char **argv) {
int main(void) {
	// Initialize the instrument lookup table
	void *synth_tab = malloc(PL_SYNTH_TAB_SIZE);
	pl_synth_init(synth_tab);

	// Determine the number of samples needed for the song
	int num_samples = pl_synth_song_len(&song);
	printf("generating %d samples\n", num_samples);

	// Allocate buffers
	int buffer_size = num_samples * 2 * sizeof(int16_t);
	int16_t *output_samples = malloc(buffer_size);
	int16_t *temp_samples = malloc(buffer_size);

	// Generate
	pl_synth_song(&song, output_samples, temp_samples);

	// Temp buffer not needed anymore
	free(temp_samples);

	// Write the generated samples to example.wav
	printf("writing example.wav\n");
	wav_write("example.wav", output_samples, num_samples, 2, 44100);

	free(output_samples);
	free(synth_tab);

	return 0;
}
