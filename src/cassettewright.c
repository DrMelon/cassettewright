#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "../argparse/argparse.h"

#include "cassettewright.h"

static const char *const usages[] = 
{
        "cassettewright [options] [[--] args]",
        "cassettewright [options]",
        NULL,
};

int main(int argc, const char **argv)
{
        int writemode = 0;
        int readmode = 0; 


        struct argparse_option options[] = 
        {
            OPT_HELP(),
            OPT_GROUP("Mode Options"),
            OPT_BOOLEAN('w', "write", &writemode, "write data to tape format", NULL, 0, 0),
            OPT_BOOLEAN('r', "read", &readmode, "read data from tape format", NULL, 0, 0),
        };

        struct argparse argparse;
        argparse_init(&argparse, options, usages, 0);
        argparse_describe(&argparse, "\nConvert data to and from the Cassettewright format over STDIN and STDOUT.", "\nWrite mode will take data from STDIN and convert it to the Cassettewright tape format, as signed 16-bit PCM over STDOUT.\nRead mode will take signed 16-bit PCM Cassettewright tape format data from STDIN and convert it to plain bytes for output over STDOUT.");
        argc = argparse_parse(&argparse, argc, argv);

        if (writemode) 
        {
                // Rough steps:
                // 1. Write polarity sync pattern.
                // 2. Write lead-in pattern.
                // 3. Write header pattern.
                // 4. Write data.
                //

                // 1. Write polarity sync pattern.
                write_polarity_sync();
                // 2. Write lead-in pattern.
                write_lead_in();
                // 3. Write header.
                write_header();

                // 4. Write data. 
                // Reading STDIN until EOF. 
                int read_chr;
                while((read_chr = getchar()) != EOF) 
                {
                    write_byte_as_pcm(read_chr);
                }
        }
        else if (readmode)
        {
        }
        else
        {
                fprintf(stderr, "Incorrect usage!\n");
        }
         
        return 0;
}

void write_polarity_sync()
{
    for(int patternCount = 0; patternCount < POLARITY_SYNC_WRITE_COUNT; patternCount++)
    {
        write_positive_cycle(POLARITY_SYNC_PATTERN_NUM_POS);
        write_negative_cycle(POLARITY_SYNC_PATTERN_NUM_NEG);
    }
}

void write_lead_in() 
{
    for(int i = 0; i < LEAD_IN_NUM_BYTES; i++) 
    {
        write_byte_as_pcm(0xFF);
    }
}

void write_header()
{
    write_byte_as_pcm(0x0A);
    write_byte_as_pcm(0x0C);
    write_byte_as_pcm(0x0A); 
    write_byte_as_pcm(0x0B);
    write_byte_as_pcm(0x06);
    write_byte_as_pcm(0x09);
}

void write_byte_as_pcm(int byte) 
{
    write_bit_as_pcm(1);
    for(int i = 0; i < 7; i++)
    {
        write_bit_as_pcm(byte >> i & 0b00000001);
    }
    write_bit_as_pcm(0);
}

void write_bit_as_pcm(int bit)
{
    if (bit == 1) 
    {
        // Bits with 1 are lower frequency waves (double the cycle length).
        write_positive_cycle(2);
        write_negative_cycle(2);
    }
    if (bit == 0)
    {
        write_positive_cycle(1);
        write_negative_cycle(1);
    }
}

void write_positive_cycle(int num)
{
    for(int posCount = 0; posCount < PCM_SAMPLES_PER_BIT * num; posCount++)
    {
        int posWrite = 0x7FFF;
        write(1, &posWrite, 2);
    }
}

void write_negative_cycle(int num)
{
    for(int negCount = 0; negCount < PCM_SAMPLES_PER_BIT * num; negCount++) 
    {
        int negWrite = -0x7FFF;
        write(1, &negWrite, 2);
    }
}
