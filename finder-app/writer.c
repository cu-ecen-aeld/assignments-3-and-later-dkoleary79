#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main( int argc, char** argv )
{
    int status = 0;
    char *filePath;
    char *writeStr;

    openlog( "writer", 0, LOG_USER );

    if( argc != 3 )
    {
        printf("!ERR: Expected two arguments\n");
        syslog(LOG_ERR, "Expected two arguments");
        status = 1;
    }
    else
    {
        filePath = argv[1];
        writeStr = argv[2];
    }

    if( status != 0 )
    {
        printf("Syntax: %s <filePath> <writeStr>\n", argv[0]);
    }
    else
    {
        FILE *f = fopen( filePath, "w" );
        if( f == NULL )
        {
            printf("Failed to open file: %s for writing\n", filePath);
            syslog(LOG_ERR, "Failed to open file: %s for writing", filePath);
            status = 1;
        }
        else
        {
            fwrite( writeStr, strlen(writeStr), 1, f );
            syslog(LOG_DEBUG, "Writing %s to %s", writeStr, filePath);
            fclose( f );
        }
    }

    return status;
}

