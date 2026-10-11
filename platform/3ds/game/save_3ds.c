/* save_3ds.c - durable save writes on the SD card. pc_m_card.c's and pc_card.c's
 * fclose calls are routed here (CMakeLists.txt) to flush through the FS service
 * before the rename: a power cut leaves the old or the new save, never half of one. */
#include <stdio.h>
#include <unistd.h>

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
