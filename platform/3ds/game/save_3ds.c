#include <stdio.h>
#include <unistd.h>

/* The save code's fclose (renamed in CMakeLists.txt): synced, so a power cut never leaves half a save. */
int ac3ds_fclose_durable(FILE* f);

int ac3ds_fclose_durable(FILE* f)
{
    if (f == NULL) {
        return EOF;
    }
    fflush(f);
    fsync(fileno(f)); /* harmless for files opened for reading */
    return fclose(f);
}
