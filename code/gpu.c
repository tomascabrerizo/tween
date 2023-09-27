#include "gpu.h"
#include "os.h"

u32 gpu_create_prorgam(char *vert, char *frag) {
    
    OsFile *file_vert = os_file_read_entire(vert);
    OsFile *file_frag = os_file_read_entire(frag);
    
    char *v_src = (char *)file_vert->data;
    char *f_src = (char *)file_frag->data;

    int success;
    static char infoLog[512];

    unsigned int v_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v_shader, 1, (const char **)&v_src, NULL);
    glCompileShader(v_shader);
    glGetShaderiv(v_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(v_shader, 512, NULL, infoLog);
        printf("[VERTEX SHADER ERROR]: %s\n", infoLog);
    }

    unsigned int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f_shader, 1, (const char **)&f_src, NULL);
    glCompileShader(f_shader);
    glGetShaderiv(f_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(f_shader, 512, NULL, infoLog);
        printf("[FRAGMENT SHADER ERROR]: %s\n", infoLog);
    }
    
    u32 program = glCreateProgram();
    glAttachShader(program, v_shader);
    glAttachShader(program, f_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("[SHADER PROGRAM ERROR]: %s\n", infoLog);
    }

    glDeleteShader(v_shader);
    glDeleteShader(f_shader);

    os_file_free(file_vert);
    os_file_free(file_frag);

    return program;
}

void gpu_destroy_program(void *program) {
    glDeleteProgram((u64)program);
}

u32 gpu_create_texture(u32 *data, u32 width, u32 height) {
    
    u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void gpu_destroy_texture(void *texture) {
    u32 id = (u64)texture;
    glDeleteTextures(1, &id);
}

void gpu_load_mesh_data(u32 *vao, u32 *vbo, u32 *ebo, Vertex *vertices, u32 num_vertices, u32 *indices, u32 num_indices) {
    
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);
    
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, num_vertices*sizeof(Vertex), vertices, GL_STATIC_DRAW); 
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET_OF(Vertex, pos)); 

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET_OF(Vertex, uv)); 

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET_OF(Vertex, color)); 

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), OFFSET_OF(Vertex, bones_id)); 

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), OFFSET_OF(Vertex, weights)); 

    glGenBuffers(1, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices*sizeof(u32), indices, GL_STATIC_DRAW); 

    glBindVertexArray(0);

}
