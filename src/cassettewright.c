#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include "../argparse/argparse.h"

#include "cassettewright.h"

static const char *const usages[] = 
{
        "cassettewright [options] [[--] args]",
        "cassettewright [options]",
        NULL,
};

        int verbose = 0;
        int examine = 0;
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
            OPT_BOOLEAN('v', "verbose", &verbose, "write information to stderr"),
            OPT_BOOLEAN('x', "examine-bitstream", &examine, "writes bitstream to stderr for diagnosing signal loss"),
            OPT_END()
        };

        struct argparse argparse;
        argparse_init(&argparse, options, usages, 0);
        argparse_describe(&argparse, "\nConvert data to and from the Cassettewright format over STDIN and STDOUT.", "\nWrite mode will take data from STDIN and convert it to the Cassettewright tape format, as signed 16-bit PCM over STDOUT.\nRead mode will take signed 16-bit PCM Cassettewright tape format data from STDIN and convert it to plain bytes for output over STDOUT.\n\nNote: For good results from real cassette tapes, record at about 70-75% volume, and play back from tape loud enough to clip during output.");
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
                
                short polarity = read_polarity();
                
                // Now we can read data.
                short previous_sample = 0;
                int current_length = 0;
                short current_sample = 0;

                is_header_accepted = 0;
                header_register = 0;
                is_bit_sync = 0;
                bit_register = 0;
                int total_read = 0;

                // Read samples in 2 bytes at a time.
                while(fread(&current_sample, 2, 1, stdin) == 1) 
                {
                    total_read++;
                    current_sample = current_sample >> 1; // Shift down so that we don't blow out two's complement with an invert.
                    current_sample *= polarity;
                    current_length += 1;

                    // Is this a zero-crossing *at the point of positive-to-negative change*? 
                    // Note: we need some tolerances for a bad zero crossing;
                    // that is, if the zero crossing happens, but we haven't yet exceeded some count, 
                    // ignore it and *keep counting*.
                    if(previous_sample > 0 && current_sample <= 0)
                    {
                        // Remember: 1s are 2 cycles long of pos + neg, 0s are 1 cycle long of pos + neg.
                        // That means the sample windows are PCM_SAMPLES_PER_BIT * 4 and PCM_SAMPLES_PER_BIT * 2. 
                        // Due to analog noise, we can only guarantee that bits are 1s if they exceed PCM_SAMPLES_PER_BIT * 3 - to give leeway of tape echo/signal jitter from 2-length 0s. 
                        int bit_is_one = (current_length > PCM_SAMPLES_PER_BIT * 3);
                        read_bit(bit_is_one);
                        current_length = 0;
                    }

                    previous_sample = current_sample;
                }

        }
        else if (documode) 
        {
                printf("\nCassettewright Format Information");
                printf("\nExpressed as Signed 16-bit PCM (Endianness Machine-Dependent)");
                printf("\nCycle: 16 PCM samples.");
                printf("\n\tBits: 1 expressed as 2 positive cycles, 2 negative cycles.");
                printf("\n\t      0 expressed as 1 positice cycle, 1 negative cycle.");
                printf("\n\tBytes: Preceded by a 1 bit, followed by a 0 bit.");
                printf("\n\tPolarity Sync Pattern: 800 repeats of positive-negative-negative-negative cycles.");
                printf("\n\tHeader:");
                printf("\n\t* Lead-in; 64 bytes of 0xFF");
                printf("\n\t* Header; 0x04 0x20 0x06 0x09");
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
    write_byte_as_pcm(0x04);
    write_byte_as_pcm(0x20);
    write_byte_as_pcm(0x06);
    write_byte_as_pcm(0x09);
}

void write_byte_as_pcm(int byte) 
{
    write_bit_as_pcm(1);
    for(int i = 0; i < 8; i++)
    {
        write_bit_as_pcm((byte >> (7-i)) & 0x01);
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
        short posWrite = 0x7FFF;
        write(1, &posWrite, 2);
    }
}

void write_negative_cycle(int num)
{
    for(int negCount = 0; negCount < PCM_SAMPLES_PER_BIT * num; negCount++) 
    {
        short negWrite = -0x7FFF;
        write(1, &negWrite, 2);
    }
}

// Read funcs 
int read_polarity() 
{
    int polarity = 0;
    short previous_sample = 0;
    short current_sample = 0;

    char sample_window[POLARITY_SYNC_CHECK_WINDOW + 1] = "";
    int window_pos = 0;
    int sample_len = 0;

    char pattern_to_check[POLARITY_SYNC_PATTERN_NUM_POS + POLARITY_SYNC_PATTERN_NUM_NEG + 1] = "";

    int num_matches_found = 0;

    int total_read = 0;

    while(fread(&current_sample, 2, 1, stdin) == 1 && polarity == 0)
    {
        total_read++;
        // A positive number * a negative number will always be negative. 
        bool crossed_zero = ((previous_sample * current_sample) < 0);
        sample_len++; 

        if(crossed_zero) 
        {
            // Is this a positive or negative sample? 
            for(int i = 0; i < sample_len / (PCM_SAMPLES_PER_BIT-1); i++)
            {
                if(current_sample > 0) 
                {
                    sample_window[window_pos] = 'p';
                }
                else
                {
                    sample_window[window_pos] = 'n';
                }
                window_pos++;
                window_pos = window_pos % POLARITY_SYNC_CHECK_WINDOW;
            }
            //printf("%s\n", sample_window);
            
            // If we're at the border of one bit window, check and see 
            // if we have matched our pattern the requisite number of times.
            if(sample_len / (PCM_SAMPLES_PER_BIT-1) > 0) 
            {
               // Check for normal pattern.
               num_matches_found = 0;
               int pattern_index = 0; 
               int chrmatch = 0;
               for(int i = 0; i < POLARITY_SYNC_PATTERN_NUM_NEG; i++)
               {
                   pattern_to_check[i] = 'n';
               }
               for(int i = POLARITY_SYNC_PATTERN_NUM_NEG; i < POLARITY_SYNC_PATTERN_NUM_NEG + POLARITY_SYNC_PATTERN_NUM_POS; i++)
               {
                   pattern_to_check[i] = 'p';
               }

               for(int window_chr = 0; window_chr < POLARITY_SYNC_CHECK_WINDOW;)
               {
                   pattern_index = 0;
                   chrmatch = 0;
                   while((sample_window[window_chr] == pattern_to_check[pattern_index]))
                   {
                        chrmatch++;
                        window_chr++;
                        pattern_index++;
                   }
                   if(chrmatch == POLARITY_SYNC_PATTERN_NUM_NEG + POLARITY_SYNC_PATTERN_NUM_POS)
                   {
                        num_matches_found++;
                        chrmatch = 0;
                   }
                   else 
                   {
                        window_chr++;
                   }
               }

               
               if(num_matches_found >= POLARITY_SYNC_DESIRED_COUNT)
               {
                   if(verbose) 
                   {
                        fprintf(stderr, "Discovered positive polarity at index %d.\n", total_read);
                   }
                   polarity = 1;
                   break;
               }


               // Check for inverted pattern.
               num_matches_found = 0;
               pattern_index = 0; 
               chrmatch = 0;
               for(int i = 0; i < POLARITY_SYNC_PATTERN_NUM_NEG; i++)
               {
                   pattern_to_check[i] = 'p';
               }
               for(int i = POLARITY_SYNC_PATTERN_NUM_NEG; i < POLARITY_SYNC_PATTERN_NUM_NEG + POLARITY_SYNC_PATTERN_NUM_POS; i++)
               {
                   pattern_to_check[i] = 'n';
               }

               for(int window_chr = 0; window_chr < POLARITY_SYNC_CHECK_WINDOW;)
               {
                   pattern_index = 0;
                   chrmatch = 0;
                   while((sample_window[window_chr] == pattern_to_check[pattern_index]))
                   {
                        chrmatch++;
                        window_chr++;
                        pattern_index++;
                   }
                   if(chrmatch == POLARITY_SYNC_PATTERN_NUM_NEG + POLARITY_SYNC_PATTERN_NUM_POS)
                   {
                        num_matches_found++;
                        chrmatch = 0;
                   }
                   else 
                   {
                        window_chr++;
                   }
               }
               if(num_matches_found >= POLARITY_SYNC_DESIRED_COUNT)
               {
                   if(verbose) 
                   {
                           fprintf(stderr, "Discovered negative polarity at index %d.\n", total_read);
                   }
                   polarity = -1;
                   break;
               }
            }


            sample_len = 0;


        }
        
        previous_sample = current_sample;
    }

    if(polarity == 0 && verbose) 
    {
        fprintf(stderr, "Failed to acquire polarity lock.\n");
    }
    return polarity;
}
int total_bit_count = 0;
void read_bit(int bit) 
{
    if(examine) 
    {
        fprintf(stderr, "%d", bit);
    }

    total_bit_count++;
    // A bit just came in, 0 or 1.
    // Shunt it on to the register, and keep it in the 1024 range. 
    // This lets us read the bit register as a short.
    bit_register = (((bit_register << 1) & 0x3FF) | (bit));
    
    // We need to determine a few things here. 
    if(!is_bit_sync) 
    {
        // If the topmost bit of the bit_register is 1, and the bottom bit is 0,
        // then we have captured a byte and we have what is called "bit sync". 
        // This is because of the way we wrapped our bytes. 
        // Our lead-in of 0xFF helps us latch onto this before reading the header.
        if((bit_register >> 9 > 0) && ((bit_register & 1) == 0))
        {
             is_bit_sync = true;
             bit_per_byte_count = 0; // Ready for next byte 
             if(verbose) 
             {
                  fprintf(stderr, "Found bit sync at %d\n", total_bit_count);
             }
        }
        else
        {
             header_register = 0; // Reset header. 
        }
    }
    else 
    {
        // Currently reading a new byte. 
        bit_per_byte_count++;
        if(bit_per_byte_count == 10)
        {
            // A count of 10 means we've read a full byte of bits plus wrap bits.
            bit_per_byte_count = 0; // Reset. 

            // However, if it doesn't look like a byte, we've hit a snag and have lost synchronization. We'll need to skip to the next byte and reset our sync. 
            // Note: This drops the connection entirely as it dumps the header. Commenting this out should allow resumption of stream with corruption.
            if (!((bit_register >> 9 > 0) && ((bit_register & 1) == 0)))
            {
                is_bit_sync = 0;
                //header_register = 0;
                //is_header_accepted = 0;
                if(verbose) 
                {
                    fprintf(stderr, "Lost bit sync at %d\n", total_bit_count);
                }
            }
            else if(is_header_accepted) 
            {
                // If the header was previously accepted, we can send the current byte to STDOUT. 
                int nextByte = ((bit_register >> 1) & 0xFF);
                write(1, &nextByte, 1);
            }
            else 
            {
                // If we have a full byte, but haven't accepted the header yet,
                // push the current byte on to the header register and check for our header!
                int nextByte = ((bit_register >> 1) & 0xFF);
                header_register = ((header_register << 8) | nextByte) & 0xFFFFFFFF;
                // If header register matches header... we did it! Sync complete.
                if(header_register == 0x04200609) 
                {
                    is_header_accepted = 1;
                    if(verbose) 
                    {
                        fprintf(stderr, "Got header lock at bit %d\n", total_bit_count);
                    }
                }
            }
        }
    }  
}
