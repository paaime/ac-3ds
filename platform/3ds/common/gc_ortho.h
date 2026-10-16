#ifndef AC3DS_GC_ORTHO_H
#define AC3DS_GC_ORTHO_H

/* The SDK's C_MTXOrtho: depth to GX's [-1, 0], which the PICA200 shares (pc_mtx.c's [0, 1] gets clipped). */

void gc_ortho(float m[4][4], float top, float bottom, float left, float right, float near_z, float far_z);

#endif /* AC3DS_GC_ORTHO_H */
