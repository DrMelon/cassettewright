#include <stdio.h>
#include "../argparse/argparse.h"

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
                printf("Lol do write\n");
        }
        else if (readmode)
        {
                printf("Lol do read\n");
        }
        else
        {
                fprintf(stderr, "Incorrect usage!\n");
        }
                
        
         
        return 0;
}
