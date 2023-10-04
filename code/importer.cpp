#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stb_image.h>

#include "algebra.h"
#include "os.h"
#include "common.h"
#include "gpu.h"
#include "animation.h"

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
    if(len > MAX_NAME_SIZE) {
        len = MAX_NAME_SIZE;
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

    // NOTE: Initiallize weights
    for(u32 i = 0; i < MAX_BONES_INFLUENCE; ++i) {
        vertex->weights[i] = 0;
        vertex->bones_id[i] = -1.0f;
    }
}

static void read_matrix(u8 **file, M4 *matrix) {
    matrix->m[0] = READ_F32(*file);
    matrix->m[1] = READ_F32(*file);
    matrix->m[2] = READ_F32(*file);
    matrix->m[3] = READ_F32(*file);
    
    matrix->m[4] = READ_F32(*file);
    matrix->m[5] = READ_F32(*file);
    matrix->m[6] = READ_F32(*file);
    matrix->m[7] = READ_F32(*file);
    
    matrix->m[8] = READ_F32(*file);
    matrix->m[9] = READ_F32(*file);
    matrix->m[10] = READ_F32(*file);
    matrix->m[11] = READ_F32(*file);
    
    matrix->m[12] = READ_F32(*file);
    matrix->m[13] = READ_F32(*file);
    matrix->m[14] = READ_F32(*file);
    matrix->m[15] = READ_F32(*file);
}

static void read_v3(u8 **file, V3 *vector) {
    vector->x = READ_F32(*file);
    vector->y = READ_F32(*file);
    vector->z = READ_F32(*file);
}

static void read_q4(u8 **file, Q4 *quat) {
    quat->w = READ_F32(*file);
    quat->x = READ_F32(*file);
    quat->y = READ_F32(*file);
    quat->z = READ_F32(*file);
}

static void add_weight_to_vertex(Vertex *vertex, u32 bone_id, f32 weight) {

    for(u32 i = 0; i < MAX_BONES_INFLUENCE; ++i) {
        if(vertex->bones_id[i] < 0) {
            vertex->bones_id[i] = bone_id;
            vertex->weights[i] = weight;
            return;
        }
    }
}

static u8 *read_entire_file(const char *path, u32 *file_size_ptr) {

    FILE *file = fopen(path, "rb");
    if(file == nullptr) {
        printf("Error: cannot open file\n");
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    u32 file_size = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);
    u8 *data = (u8 *)malloc(file_size + 1);
    fread(data, file_size, 1, file);
    data[file_size] = '\0';
    fclose(file);

    if(file_size_ptr != nullptr) {
        *file_size_ptr = file_size;
    }

    return data;
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
        
        char skeleton_name[256];
        read_string(&file, skeleton_name);
        u32 total_number_of_bones = READ_U32(file);
        
        printf("Skeleton name: %s, total bones: %d\n", skeleton_name, total_number_of_bones);
        printf("Loading vertex weights for skeleton ... \n");
        
        for(u32 mesh_index = 0; mesh_index < model->num_meshes; ++mesh_index) {
            Mesh *mesh = model->meshes + mesh_index; (void)mesh;

            u32 number_of_bones = READ_U32(file);
            
            for(u32 bone_index = 0; bone_index < number_of_bones; ++bone_index) {
                
                u32 current_bone_id = READ_U32(file);
                u32 num_weights = READ_U32(file);
                printf("%d) num_weights: %d\n", current_bone_id, num_weights);

                for(u32 weights_index = 0; weights_index < num_weights; ++weights_index) {
                    
                    u32 vertex_index = READ_U32(file);
                    f32 weight = READ_F32(file);
                    
                    ASSERT(vertex_index < mesh->num_vertices);
                    Vertex *vertex = mesh->vertices + vertex_index;
                    add_weight_to_vertex(vertex, current_bone_id, weight);
                
                }
            }
        }

        printf("All vertices ready for animation!\n");
    }
}

static void read_joint(u8 **file, Joint *joint) {
    
    joint->parent = READ_S32(*file);
    read_string(file, joint->name);
    read_matrix(file, &joint->local_transform);
    read_matrix(file, &joint->inv_bind_transform);
    
    printf("bone %s: parent index: %d\n", joint->name, joint->parent);
}


static void read_sample(u8 **file, AnimationSample *sample, u32 num_joints) {
    
    u32 num_animated_bones = READ_U32(*file);
    sample->local_poses = (JointPose *)malloc(sizeof(JointPose)*num_joints);

    for(u32 pose_index = 0; pose_index < num_joints; ++pose_index) {
        JointPose *pose = sample->local_poses + pose_index;
        pose->position = v3(0, 0, 1);
        pose->rotation = q4(1, 0, 0, 0);
        pose->scale = v3(1, 1, 1);
    }
    
    bool time_stamp_initialize = false;

    for(u32 animated_bone_index = 0; animated_bone_index < num_animated_bones; ++animated_bone_index) {
        u32 bone_index = READ_U32(*file);
        f32 time_stamp = READ_F32(*file);
        read_v3(file, &sample->local_poses[bone_index].position);
        read_q4(file, &sample->local_poses[bone_index].rotation);
        read_v3(file, &sample->local_poses[bone_index].scale);
        
        if(time_stamp_initialize == false) {
            sample->time_stamp = time_stamp;
            time_stamp_initialize = true;
        }
    }

}

static void read_tween_skeleton_file(Skeleton *skeleton, AnimationClip **animations, u32 *num_animations, u8 *file) {

    u32 magic = READ_U32(file);
    ASSERT(magic == TWEEN_MAGIC);

    u32 flags = READ_U32(file);

    if(flags & TWEEN_ANIMATIONS) {
        printf("Loading Animation file\n");
    }
    
    ASSERT(flags & TWEEN_ANIMATIONS);

    read_string(&file, skeleton->name);
    skeleton->num_joints = READ_U32(file);
    skeleton->joints = (Joint *)malloc(sizeof(Joint)*skeleton->num_joints);
    printf("Loaded skeleton name: %s, number of joints: %d\n", skeleton->name, skeleton->num_joints);

    for(u32 joint_index = 0; joint_index < skeleton->num_joints; ++joint_index) {
        Joint *joint = skeleton->joints + joint_index;
        read_joint(&file, joint);
    }
    
    u32 animations_array_size = READ_U32(file);
    AnimationClip *animations_array = (AnimationClip *)malloc(sizeof(AnimationClip)*animations_array_size);

    printf("Number of animations: %d\n", animations_array_size);

    for(u32 animation_index = 0; animation_index < animations_array_size; ++animation_index) {
        AnimationClip *animation = animations_array + animation_index; 
        animation->skeleton = skeleton;

        read_string(&file, animation->name);
        animation->duration = READ_F32(file);
        animation->num_samples = READ_U32(file);
        animation->samples = (AnimationSample *)malloc(sizeof(AnimationSample)*animation->num_samples);
        
        for(u32 sample_index = 0; sample_index < animation->num_samples; ++sample_index) {
            AnimationSample *sample = animation->samples + sample_index;
            read_sample(&file, sample, skeleton->num_joints);
        }

        printf("Animation name: %s, duration: %f, keyframes: %d\n", animation->name, animation->duration, animation->num_samples);

    }
    
    *animations = animations_array;
    *num_animations = animations_array_size;

    printf("Animation complete loading perfectly!\n");
}

int main(void) {

    os_initialize();

    u8 *model_file = read_entire_file("./data/model.twm", nullptr);
    Model model;
    read_tween_model_file(&model, model_file);
    printf("File read perfectly\n");
    
    printf("\n---------------------------\n");

    u8 *animation_file = read_entire_file("./data/model.twa", nullptr);
    
    Skeleton skeleton;
    AnimationClip *animations = nullptr;
    u32 num_animations = 0;
    read_tween_skeleton_file(&skeleton, &animations, &num_animations, animation_file);

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
    u32 program = gpu_create_prorgam((char *)"./shaders/vert.glsl", (char *)"./shaders/frag.glsl");
    glUseProgram(program);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_MULTISAMPLE);  

    u64 miliseconds_per_frame = 16;
    f32 seconds_per_frame = (f32)miliseconds_per_frame / 1000.0f;
    u64 last_time = os_get_ticks();

    AnimationSet set;
    set.initialize(animations, num_animations);
    set.set_root_joint("punch", "mixamorig1_Spine");

    set.play("idle", 1, true);
    set.play("walking", 1, true);

    f32 player_speed = 0;
    
    while(!window->should_close) {

        os_window_poll_events(window);
        
        if(os_keyboard[(u32)'w']) {
            player_speed = CLAMP(player_speed + seconds_per_frame, 0, 1);
        } else {
            player_speed = CLAMP(player_speed - seconds_per_frame, 0, 1);
        }

        if(os_keyboard[(u32)'1']) {
            if(set.animation_finish("punch") == true) {
                printf("punch!\n");
                set.play_smooth("punch", 0.5);
            }
        }
        
        set.update_weight("walking", player_speed);

        set.update(seconds_per_frame);
        
        window_w = window_width(window);
        window_h = window_height(window);

        glViewport(0, 0, window_w, window_h);

        glUseProgram(program);

        M4 v = m4_identity();
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, true, v.m);
        
        f32 aspect = (f32)window_w/(f32)window_h;
        M4 p = m4_perspective2(to_rad(80), aspect, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, true, p.m);

        for(u32 i = 0;  i < set.skeleton->num_joints; ++i) {
            char bone_matrix_name[1024];
            sprintf(bone_matrix_name, "bone_matrix[%d]", i);
            M4 bone_matrix = set.final_transform_matrices[i];
            glUniformMatrix4fv(glGetUniformLocation(program, bone_matrix_name), 1, true, bone_matrix.m);
        }
        static f32 angle = 0;
        M4 m = m4_mul(m4_translate(v3(0, -1, -2)), m4_mul(m4_rotate_y(to_rad(angle)), m4_scale(.012f)));
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, true, m.m);

        angle += (14 * seconds_per_frame);
        if(angle >= 360) {
            angle = 0;
        }

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

        //printf("ms: %f, fps: %f\n", (f32)delta_time, ((f32)1/delta_time) * 1000);
    }
    
    set.terminate();

    os_gl_destroy_context(window);
    os_window_destroy(window);
    
    printf("Program terminate perfectly\n");;;

    return 0;
}
