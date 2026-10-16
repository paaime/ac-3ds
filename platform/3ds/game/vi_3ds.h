#ifndef AC3DS_VI_3DS_H
#define AC3DS_VI_3DS_H

typedef struct {
    int cmds;
    int tri_cmds;
    int vtx_cmds;
    int dl_cmds;
    int cull_rejected;
} Emu64FrameCounts;

Emu64FrameCounts vi3ds_last_emu64_counts(void);

#endif /* AC3DS_VI_3DS_H */
