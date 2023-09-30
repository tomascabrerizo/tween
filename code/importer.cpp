#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stb_image.h>

#include "algebra.h"
#include "os.h"
#include "common.h"
#include "gpu.h"
#include "animator.h"

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
        
        model->num_inv_bind_transform = total_number_of_bones;
        model->inv_bind_transform = (M4 *)malloc(sizeof(M4)*model->num_inv_bind_transform);
        // NOTE: Initializer all matrices to the identity
        for(u32 bone_index = 0; bone_index < total_number_of_bones; ++bone_index) {
            model->inv_bind_transform[bone_index] = m4_identity();
        }

        for(u32 mesh_index = 0; mesh_index < model->num_meshes; ++mesh_index) {
            Mesh *mesh = model->meshes + mesh_index; (void)mesh;

            u32 number_of_bones = READ_U32(file);
            printf("number of bones: %d\n", number_of_bones);
            
            for(u32 bone_index = 0; bone_index < number_of_bones; ++bone_index) {
                
                u32 current_bone_id = READ_U32(file);
                u32 num_weights = READ_U32(file);
                printf("current_bone id: %d, num_weights: %d\n", current_bone_id, num_weights);

                for(u32 weights_index = 0; weights_index < num_weights; ++weights_index) {
                    u32 vertex_index = READ_U32(file);
                    f32 weight = READ_F32(file);
                    ASSERT(vertex_index < mesh->num_vertices);
                    Vertex *vertex = mesh->vertices + vertex_index;
                    add_weight_to_vertex(vertex, current_bone_id, weight);
                }
                ASSERT(current_bone_id < total_number_of_bones);
                read_matrix(&file, &model->inv_bind_transform[current_bone_id]);
            }
        }

        printf("All vertices ready for animation!\n");
    }
}

static void read_bone(u8 **file, Bone *bone) {
    
    bone->parent = READ_S32(*file);
    read_string(file, bone->name);
    read_matrix(file, &bone->transformation);
    
    printf("bone %s: parent index: %d\n", bone->name, bone->parent);
}

static void read_keyframe(u8 **file, KeyFrame *frame) {
    frame->num_animated_bones = READ_U32(*file);
    
    frame->bone_ids    = (u32 *)malloc(sizeof(u32)*frame->num_animated_bones);
    frame->time_stamps = (f32 *)malloc(sizeof(f32)*frame->num_animated_bones);
    frame->positions   = (V3 *)malloc(sizeof(V3)*frame->num_animated_bones);
    frame->rotations   = (Q4 *)malloc(sizeof(Q4)*frame->num_animated_bones);
    frame->scales      = (V3 *)malloc(sizeof(V3)*frame->num_animated_bones);
    
    for(u32 animated_bone_index = 0; animated_bone_index < frame->num_animated_bones; ++animated_bone_index) {
        frame->bone_ids[animated_bone_index] = READ_U32(*file);
        frame->time_stamps[animated_bone_index] = READ_F32(*file);
        read_v3(file, &frame->positions[animated_bone_index]);
        read_q4(file, &frame->rotations[animated_bone_index]);
        read_v3(file, &frame->scales[animated_bone_index]);
    }

}

static void read_tween_skeleton_file(Skeleton *skeleton, u8 *file) {

    u32 magic = READ_U32(file);
    ASSERT(magic == TWEEN_MAGIC);

    u32 flags = READ_U32(file);

    if(flags & TWEEN_ANIMATIONS) {
        printf("Loading Animation file\n");
    }
    
    ASSERT(flags & TWEEN_ANIMATIONS);

    read_string(&file, skeleton->name);
    skeleton->num_bones = READ_U32(file);
    skeleton->bones = (Bone *)malloc(sizeof(Bone)*skeleton->num_bones);
    printf("Loaded skeleton name: %s, number of bones: %d\n", skeleton->name, skeleton->num_bones);

    for(u32 bone_index = 0; bone_index < skeleton->num_bones; ++bone_index) {
        Bone *bone = skeleton->bones + bone_index;
        read_bone(&file, bone);
    }
    
    skeleton->num_animations = READ_U32(file);
    skeleton->animations = (Animation *)malloc(sizeof(Animation)*skeleton->num_animations);

    printf("Number of animations: %d\n", skeleton->num_animations);

    for(u32 animation_index = 0; animation_index < skeleton->num_animations; ++animation_index) {
        Animation *animation = skeleton->animations + animation_index; 
        read_string(&file, animation->name);
        animation->duration = READ_F32(file);
        animation->num_frames = READ_U32(file);
        animation->frames = (KeyFrame *)malloc(sizeof(KeyFrame)*animation->num_frames);
        
        for(u32 frame_index = 0; frame_index < animation->num_frames; ++frame_index) {
            KeyFrame *frame = animation->frames + frame_index;
            read_keyframe(&file, frame);
        }

        printf("Animation name: %s, duration: %f, keyframes: %d\n", animation->name, animation->duration, animation->num_frames);

    }

    printf("Animation complete loading perfectly!\n");
    
    skeleton->active_animation = -1;
    skeleton->last_animation = -1;
    
    // TODO: find a better way to save the current keyframe

    KeyFrame *frame = &skeleton->transition_keyframe;

    frame->bone_ids    = (u32 *)malloc(sizeof(u32)*skeleton->num_bones);
    frame->time_stamps = (f32 *)malloc(sizeof(f32)*skeleton->num_bones);
    frame->positions   = (V3 *)malloc(sizeof(V3)*skeleton->num_bones);
    frame->rotations   = (Q4 *)malloc(sizeof(Q4)*skeleton->num_bones);
    frame->scales      = (V3 *)malloc(sizeof(V3)*skeleton->num_bones);
}

static void calculate_prev_and_next_animation_keyframs(Animation *animation, KeyFrame *prev, KeyFrame *next, f32 animation_time) {
    (void)prev; (void)next; (void)animation_time;
    
    u32 prev_frame_index = 0;
    u32 next_frame_index = 1;

    for(u32 frame_index = 1; frame_index < animation->num_frames; ++frame_index) {
        KeyFrame *frame = animation->frames + frame_index;
        ASSERT(frame->num_animated_bones > 0);
        f32 frame_time_stamp = frame->time_stamps[0];

        if(frame_time_stamp > animation_time) {
            next_frame_index = frame_index;
            break;
        }
    }
    
    ASSERT(next_frame_index > 0);
    prev_frame_index = (next_frame_index - 1);

    *prev = animation->frames[prev_frame_index];
    *next = animation->frames[next_frame_index];
}

static void calculate_current_animation_keyframe(Animation *animation, KeyFrame *current_key_frame, f32 animation_time) {

        KeyFrame prev, next;
        calculate_prev_and_next_animation_keyframs(animation, &prev, &next, animation_time);

        f32 prev_frame_time_stamp = prev.time_stamps[0];
        f32 next_frame_time_stamp = next.time_stamps[0];

        f32 progression = (animation_time - prev_frame_time_stamp) / (next_frame_time_stamp - prev_frame_time_stamp);

        current_key_frame->num_animated_bones = prev.num_animated_bones;

        for(u32 keyframe_bone_index = 0; keyframe_bone_index < prev.num_animated_bones; ++keyframe_bone_index) {

            current_key_frame->bone_ids[keyframe_bone_index] = prev.bone_ids[keyframe_bone_index];
            current_key_frame->time_stamps[keyframe_bone_index] = lerp(prev.time_stamps[keyframe_bone_index], next.time_stamps[keyframe_bone_index], progression);

            current_key_frame->positions[keyframe_bone_index] = v32_lerp(prev.positions[keyframe_bone_index], next.positions[keyframe_bone_index], progression);
            current_key_frame->scales[keyframe_bone_index] = v32_lerp(prev.scales[keyframe_bone_index], next.scales[keyframe_bone_index], progression);
            current_key_frame->rotations[keyframe_bone_index] = q4_slerp(prev.rotations[keyframe_bone_index], next.rotations[keyframe_bone_index], progression);
        }

}

static void calculate_bones_transform(KeyFrame *prev, KeyFrame *next, Model *model, Skeleton *skeleton, float animation_time, M4 *final_transforms) {
    
        f32 prev_frame_time_stamp = prev->time_stamps[0];
        f32 next_frame_time_stamp = next->time_stamps[0];

        f32 progression = (animation_time - prev_frame_time_stamp) / (next_frame_time_stamp - prev_frame_time_stamp);
        
        ASSERT(prev->num_animated_bones == next->num_animated_bones);

        for(u32 keyframe_bone_index = 0; keyframe_bone_index < prev->num_animated_bones; ++keyframe_bone_index) {
            
            V3 final_position = v32_lerp(prev->positions[keyframe_bone_index], next->positions[keyframe_bone_index], progression);
            V3 final_scale    = v32_lerp(prev->scales[keyframe_bone_index], next->scales[keyframe_bone_index], progression);
            Q4 final_rotation = q4_slerp(prev->rotations[keyframe_bone_index], next->rotations[keyframe_bone_index], progression);

            (void)final_scale;
            
            ASSERT(prev->bone_ids[keyframe_bone_index] == next->bone_ids[keyframe_bone_index]);
            u32 bone_id = prev->bone_ids[keyframe_bone_index];
            final_transforms[bone_id] = m4_mul(m4_translate(final_position), m4_mul(q4_to_m4(final_rotation), m4_scale_v3(final_scale)));
        }
        
        for(u32 bone_index = 0; bone_index < skeleton->num_bones; ++bone_index) {
            Bone *bone = skeleton->bones + bone_index;
            if(bone->parent == ((u32)-1)) {
                final_transforms[bone_index] = m4_mul(m4_identity(), final_transforms[bone_index]);
            } else {
                ASSERT(bone->parent < bone_index);
                final_transforms[bone_index] = m4_mul(final_transforms[bone->parent], final_transforms[bone_index]);

            }
        }

        ASSERT(skeleton->num_bones == model->num_inv_bind_transform);
        
        for(u32 bone_index = 0; bone_index < skeleton->num_bones; ++bone_index) {
            final_transforms[bone_index] = m4_mul(final_transforms[bone_index], model->inv_bind_transform[bone_index]);
        }
}

void update_animation(Model *model, Skeleton *skeleton, M4 *final_transforms, f32 dt) {
    
    if(skeleton->active_animation == -1) return;

    static float animation_time = 0;
    animation_time += dt;

    f32 transition_duration = 0.5f;
    static f32 transition_time = 0;
    static u32 transition_in_process = false;
    static KeyFrame transition_key_frame_end;

    if((skeleton->last_animation != -1) && (skeleton->active_animation != skeleton->last_animation) && transition_in_process == false) {
        transition_in_process = true;

        Animation *animation0 = &skeleton->animations[skeleton->last_animation];
        Animation *animation1 = &skeleton->animations[skeleton->active_animation];

        ASSERT(animation1->num_frames > 0);
        calculate_current_animation_keyframe(animation0, &skeleton->transition_keyframe, animation_time);
        transition_key_frame_end = animation1->frames[0];
        
        // TODO: This is a total hack NEED TO BE REFACTOR
        skeleton->transition_keyframe.time_stamps[0] = 0;
        transition_key_frame_end.time_stamps[0] = transition_duration;

    } 

    if(transition_in_process == true) {

        transition_time += dt;
        calculate_bones_transform(&skeleton->transition_keyframe, &transition_key_frame_end, model, skeleton, transition_time, final_transforms);

        if(transition_time >= transition_duration) {
            
            animation_time = 0;
            transition_time = 0;
            transition_in_process = false;
            
            skeleton->last_animation = skeleton->active_animation;
        }

    } else {

        ASSERT(skeleton->num_animations > 0);
        Animation *animation = &skeleton->animations[skeleton->active_animation];

        if(animation_time > animation->duration) {
            animation_time = 0;
        }
        
        (void)animation, (void)model, (void)final_transforms, (void)dt;

        KeyFrame prev, next;
        calculate_prev_and_next_animation_keyframs(animation, &prev, &next, animation_time);
        calculate_bones_transform(&prev, &next, model, skeleton, animation_time, final_transforms);

        skeleton->last_animation = skeleton->active_animation;
    }

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
    read_tween_skeleton_file(&skeleton, animation_file);
    
    Animator animator;
    animator.initialize(model, skeleton);
       
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
    

    while(!window->should_close) {

        os_window_poll_events(window);

        if(os_keyboard[(u32)'1']) {
            animator.play((char *)"silly dance", 1, false);
        }
        if(os_keyboard[(u32)'2']) {
            animator.play((char *)"falir", .1f, false);
        }
        if(os_keyboard[(u32)'3']) {
            animator.play((char *)"dancing twerk", 1, false);
        }

        animator.update(seconds_per_frame);
        
        window_w = window_width(window);
        window_h = window_height(window);

        glViewport(0, 0, window_w, window_h);

        glUseProgram(program);

        M4 v = m4_identity();
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, true, v.m);
        
        f32 aspect = (f32)window_w/(f32)window_h;
        M4 p = m4_perspective2(to_rad(80), aspect, 0.1f, 1000.0f);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, true, p.m);

        for(u32 i = 0;  i < skeleton.num_bones; ++i) {
            char bone_matrix_name[1024];
            sprintf(bone_matrix_name, "bone_matrix[%d]", i);
            M4 bone_matrix = animator.final_transforms[i];
            glUniformMatrix4fv(glGetUniformLocation(program, bone_matrix_name), 1, true, bone_matrix.m);
        }

        static f32 angle = 0;
#if 0
        M4 m = m4_mul(m4_translate(v3(0, -1, -2)), m4_mul(m4_mul(m4_rotate_y(to_rad(angle)) ,m4_rotate_x(to_rad(-90))), m4_scale(.2)));
#else
        M4 m = m4_mul(m4_translate(v3(0, -1, -2)), m4_mul(m4_rotate_y(to_rad(angle)), m4_scale(.012f)));
#endif
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, true, m.m);

        angle += (20 * seconds_per_frame);
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
    
    os_gl_destroy_context(window);
    os_window_destroy(window);
    
    printf("Program terminate perfectly\n");;;

    return 0;
}
