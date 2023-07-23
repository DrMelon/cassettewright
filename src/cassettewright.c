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
        int documode = 0;

        struct argparse_option options[] = 
        {
            OPT_HELP(),
            OPT_GROUP("Mode Options"),
            OPT_BOOLEAN('w', "write", &writemode, "write data to tape format", NULL, 0, 0),
            OPT_BOOLEAN('r', "read", &readmode, "read data from tape format", NULL, 0, 0),
            OPT_BOOLEAN('d', "documentation", &documode, "print documentation about the format", NULL, 0, 0),
            OPT_END()
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
                // Rough steps: 
                // 1. Begin reading stdin in steps of 2 (to get 16-bit ints) 
                // 2. Evaluate if currrent int is positive or negative 
                // 3. Create rolling buffer of positive and negative readings for polarity sync
                //    a. Polarity sync buffer matches need to be counted *within current search window*.
                //    b. Search window is over pos/neg cycles of a fixed length. 
                //    c. Once threshold number of sync buffer matches met, we now know we're synced up. Record the polarity.
                // 4. When sync is achieved, we know the step and offset to start reading bytes 
                // 5. Start consuming stdin packets and building up whole bytes, inverting if polarity is inverted
                //    a. Remember! Need to read zero-crossings; if the current sample and previous sample's sign don't match, it happened.
                // 6. Read lead-in and header - evaluate buffer until header match acquired.
                // 7. Once header match acquired, we can safely read whole bytes out to stdout. 
        }
        else if (documode) 
        {
                printf("\nCassettewright Format Information");
                printf("\nExpressed as Signed 16-bit PCM (Endianness Machine-Dependent)");
                printf("\nCycle: 16 PCM samples.");
                printf("\n\tBits: 1 expressed as 2 positive cycles, 2 negative cycles.");
                printf("\n\t      0 expressed as 1 positice cycle, 1 negative cycle.");
                printf("\n\tBytes: Preceded by a 1 bit, followed by a 0 bit.");
                printf("\n\tPolarity Sync Pattern: 200 repeats of positive-negative-negative-negative cycles.");
                printf("\n\tHeader:");
                printf("\n\t* Lead-in; 16 bytes of 0xFF");
                printf("\n\t* Header; 0x0A 0x0C 0x0A 0x0B 0x06 0x09");
                printf("\n");
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
