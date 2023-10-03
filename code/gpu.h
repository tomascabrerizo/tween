#ifndef _GPU_H_
#define _GPU_H_

#include "x11_gl.h"
#include "animation.h"

u32 gpu_create_prorgam(char *vert, char *frag);

void gpu_destroy_program(void *program);

u32 gpu_create_texture(u32 *data, u32 width, u32 height);

void gpu_destroy_texture(void *texture);

void gpu_load_mesh_data(u32 *vao, u32 *vbo, u32 *ebo, Vertex *vertices, u32 num_vertices, u32 *indices, u32 num_indices);

#endif /* _GPU_H_ */
