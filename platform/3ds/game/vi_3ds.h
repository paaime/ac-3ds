#ifndef AC3DS_VI_3DS_H
#define AC3DS_VI_3DS_H

/* emu64's per-frame counters (pc_emu64_frame_*) for the last submitted frame. */
typedef struct {
    int cmds;          /* display-list commands processed */
    int tri_cmds;
    int vtx_cmds;      /* vertex loads */
    int dl_cmds;       /* gSPDisplayList calls */
    int cull_rejected; /* commands skipped by emu64's culling */
} Emu64FrameCounts;

Emu64FrameCounts vi3ds_last_emu64_counts(void);

#endif /* AC3DS_VI_3DS_H */
