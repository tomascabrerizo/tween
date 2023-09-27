#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stb_image.h>

#include "os.h"
#include "common.h"
#include "gpu.h"


#define TWEEN_MAGIC ((unsigned int)('E'<<24)|('E'<<16)|('W'<<8)|'T')

#define TWEEN_MODEL      (1 << 0)
#define TWEEN_SKELETON   (1 << 1)
#define TWEEN_ANIMATIONS (1 << 2)

#define READ_U64(buffer) *((u64 *)buffer); buffer += 8
#define READ_U32(buffer) *((u32 *)buffer); buffer += 4
#define READ_U16(buffer) *((u16 *)buffer); buffer += 2
#define READ_U8(buffer) *((u8 *)buffer); buffer += 1

#define READ_S64(buffer) *((s64 *)buffer); buffer += 8
#define READ_S32(buffer) *((s32 *)buffer); buffer += 4
#define READ_S16(buffer) *((s16 *)buffer); buffer += 2
#define READ_S8(buffer) *((s8 *)buffer); buffer += 1

#define READ_F32(buffer) *((f32 *)buffer); buffer += 4


static void read_string(u8 **file, char *buffer) {
    u32 len = READ_U32(*file);
    if(len > MESH_MAX_NAME_SIZE) {
        len = MESH_MAX_NAME_SIZE;
    }
    memcpy(buffer, *file, len);
    buffer[len] = '\0';
    
    *file += len;
}

static void read_vertex(u8 **file, Vertex *vertex) {
    // NOTE: Read position
    vertex->pos.x = READ_F32(*file);
    vertex->pos.y = READ_F32(*file);
    vertex->pos.z = READ_F32(*file);

    // NOTE: Skip normals for now
    (void)READ_F32(*file);
    (void)READ_F32(*file);
    (void)READ_F32(*file);
    
    // NOTE: Read texcoords
    vertex->uv.x = READ_F32(*file);
    vertex->uv.y = READ_F32(*file);

    // NOTE: Read color
    vertex->color.x = 1.0f;
    vertex->color.y = 1.0f;
    vertex->color.z = 1.0f;
}

static void read_tween_model_file(Model *model, u8 *file) {
    u32 magic = READ_U32(file);
    ASSERT(magic == TWEEN_MAGIC);

    u32 flags = READ_U32(file);

    if(flags & TWEEN_MODEL) {
        printf("Loading model file\n");
    }
    
    ASSERT((flags & TWEEN_SKELETON) && (flags & TWEEN_MODEL));

    model->num_meshes = READ_U32(file);
    model->meshes = (Mesh *)malloc(sizeof(Mesh)*model->num_meshes);
    printf("Number of meshes: %d\n", model->num_meshes);
    
    for(u32 mesh_index = 0; mesh_index < model->num_meshes; ++mesh_index) {
        Mesh *mesh = model->meshes + mesh_index;

        mesh->num_vertices = READ_U32(file);
        mesh->vertices = (Vertex *)malloc(sizeof(Vertex)*mesh->num_vertices);

        mesh->num_indices = READ_U32(file);
        mesh->indices = (u32 *)malloc(sizeof(u32)*mesh->num_indices);
        
        read_string(&file, mesh->material);

        printf("Num vertices: %d, indices: %d\n", mesh->num_vertices, mesh->num_indices);
        printf("Material path: %s\n", mesh->material);
    }

    for(u32 mesh_index = 0; mesh_index < model->num_meshes; ++mesh_index) {
        Mesh *mesh = model->meshes + mesh_index;
        for(u32 vertex_index = 0; vertex_index < mesh->num_vertices; ++vertex_index) {
            Vertex *vertex = mesh->vertices + vertex_index;
            read_vertex(&file, vertex);
        }

        for(u32 indice_index = 0; indice_index < mesh->num_indices; ++indice_index) {
            mesh->indices[indice_index] = READ_U32(file);
        }

    }
    
    if(flags & TWEEN_SKELETON) {
        printf("Loading skeleton for model\n");
    }

    char skeleton_name[256];
    read_string(&file, skeleton_name);
    printf("Skeleton name: %s\n", skeleton_name);


}


int main(void) {

    FILE *file_model = fopen("./data/model.twm", "rb");

    if(file_model == nullptr) {
        printf("Error: cannot open file\n");
        return 1;
    }

    fseek(file_model, 0, SEEK_END);
    u64 file_size = (u64)ftell(file_model);
    fseek(file_model, 0, SEEK_SET);
    
    u8 *model_data = (u8 *)malloc(file_size + 1);
    fread(model_data, file_size, 1, file_model);
    model_data[file_size] = '\0';

    fclose(file_model);
    
    Model model;
    read_tween_model_file(&model, model_data);
    printf("File read perfectly\n");
    

    os_initialize();
       
    u32 window_w = 1280;
    u32 window_h = 720;
    OsWindow *window = os_window_create((char *)"Importer Test", 10, 10, window_w, window_h);
    os_gl_create_context(window);

    // NOTE: Upload model data to GPU
    for(u32 mesh_index = 0; mesh_index < model.num_meshes ; ++mesh_index) {

        Mesh *mesh = model.meshes + mesh_index;

        gpu_load_mesh_data(&mesh->vao, &mesh->vbo, &mesh->ebo, mesh->vertices, mesh->num_vertices, mesh->indices, mesh->num_indices);

        char diffuse_material_path_cstr[4096];
        sprintf(diffuse_material_path_cstr, "%s%s", "./data/", mesh->material); 
        printf("material loaded: %s\n", diffuse_material_path_cstr);

        stbi_set_flip_vertically_on_load(true);
        s32 w, h, n;
        u32 *bitmap = (u32 *)stbi_load(diffuse_material_path_cstr, &w, &h, &n, 4);
        mesh->texture = gpu_create_texture(bitmap, w, h);
        stbi_image_free(bitmap);
    }

    // NOTE: Create GPU shaders
    u32 program = gpu_create_prorgam((char *)"./shaders/vert_simple.glsl", (char *)"./shaders/frag_simple.glsl");
    glUseProgram(program);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);  

    u64 miliseconds_per_frame = 16;
    f32 seconds_per_frame = (f32)miliseconds_per_frame / 1000.0f;
    u64 last_time = os_get_ticks();
    
    while(!window->should_close) {
        
        window_w = window_width(window);
        window_h = window_height(window);

        glViewport(0, 0, window_w, window_h);

        os_window_poll_events(window);
        (void)seconds_per_frame;

        glUseProgram(program);

        M4 v = m4_identity();
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, true, v.m);
        
        f32 aspect = (f32)window_w/(f32)window_h;
        M4 p = m4_perspective2(to_rad(80), aspect, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, true, p.m);
        
        M4 m = m4_mul(m4_translate(v3(0, -2.5f, -4)), m4_mul(m4_rotate_x(to_rad(-90)), m4_scale(.6f)));
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, true, m.m);
;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(u32 i = 0; i < model.num_meshes; ++i) {
            Mesh *mesh = model.meshes + i;
            
            glBindVertexArray(mesh->vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

            glBindTexture(GL_TEXTURE_2D, mesh->texture);
            glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, 0);
        }
        
        os_gl_swap_buffers(window);

        u64 current_time = os_get_ticks();
        u64 delta_time = current_time - last_time; (void)delta_time;
        if(delta_time < miliseconds_per_frame) {
            os_sleep(miliseconds_per_frame - delta_time);
            current_time = os_get_ticks(); 
            delta_time = current_time - last_time;
        }
        last_time = current_time; (void)last_time;
    }
    
    os_gl_destroy_context(window);
    os_window_destroy(window);
    
    printf("Program terminate perfectly\n");;;

    return 0;
}
