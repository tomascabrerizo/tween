#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/types.h>
#include <cstring>
#include <stdio.h>
#include <assert.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/vector3.h>
#include <assimp/material.h>
#include <assimp/mesh.h>

#define TWEEN_MAGIC ((unsigned int)('E'<<24)|('E'<<16)|('W'<<8)|'T')

#define TWEEN_MODEL      (1 << 0)
#define TWEEN_SKELETON   (1 << 1)
#define TWEEN_ANIMATIONS (1 << 2)

static void write_key_frame(unsigned int id, aiVectorKey position_key, aiQuatKey rotation_key, aiVectorKey scaling_key, FILE* file) {
    assert(position_key.mTime == rotation_key.mTime && position_key.mTime == rotation_key.mTime);
    
    float time = position_key.mTime / 1000.0f;

    fwrite(&id, sizeof(unsigned int), 1, file);
    fwrite(&time, sizeof(float), 1, file);
    
    fwrite(&position_key.mValue.x, sizeof(float), 1, file);
    fwrite(&position_key.mValue.y, sizeof(float), 1, file);
    fwrite(&position_key.mValue.z, sizeof(float), 1, file);

    fwrite(&rotation_key.mValue.w, sizeof(float), 1, file);
    fwrite(&rotation_key.mValue.x, sizeof(float), 1, file);
    fwrite(&rotation_key.mValue.y, sizeof(float), 1, file);
    fwrite(&rotation_key.mValue.z, sizeof(float), 1, file);

    fwrite(&scaling_key.mValue.x, sizeof(float), 1, file);
    fwrite(&scaling_key.mValue.y, sizeof(float), 1, file);
    fwrite(&scaling_key.mValue.z, sizeof(float), 1, file);

}

static void write_string(aiString string, FILE *file) {
    fwrite(&string.length, sizeof(unsigned int), 1, file);
    fwrite(string.data, sizeof(char), string.length, file);
}

static void write_string_cstr(const char *string, FILE *file) {
    unsigned int len = strlen(string);
    fwrite(&len, sizeof(unsigned int), 1, file);
    fwrite(string, sizeof(char), len, file);
}

static void write_matrix(aiMatrix4x4 matrix, FILE *file) {

    fwrite(&matrix.a1, sizeof(float), 1, file);
    fwrite(&matrix.a2, sizeof(float), 1, file);
    fwrite(&matrix.a3, sizeof(float), 1, file);
    fwrite(&matrix.a4, sizeof(float), 1, file);

    fwrite(&matrix.b1, sizeof(float), 1, file);
    fwrite(&matrix.b2, sizeof(float), 1, file);
    fwrite(&matrix.b3, sizeof(float), 1, file);
    fwrite(&matrix.b4, sizeof(float), 1, file);

    fwrite(&matrix.c1, sizeof(float), 1, file);
    fwrite(&matrix.c2, sizeof(float), 1, file);
    fwrite(&matrix.c3, sizeof(float), 1, file);
    fwrite(&matrix.c4, sizeof(float), 1, file);

    fwrite(&matrix.d1, sizeof(float), 1, file);
    fwrite(&matrix.d2, sizeof(float), 1, file);
    fwrite(&matrix.d3, sizeof(float), 1, file);
    fwrite(&matrix.d4, sizeof(float), 1, file);
}

static void count_child_nodes(aiNode *node, unsigned int *num_bones) {
    ++(*num_bones);
    for(unsigned int i = 0; i < node->mNumChildren; ++i) {
        aiNode *child = node->mChildren[i];
        count_child_nodes(child, num_bones);
    }
}

static void write_skeleton_node(aiNode *node, FILE *file, unsigned int parent_offset, unsigned int *current_offset) {
    
    unsigned int offset = *current_offset;
    ++(*current_offset);
     
    fwrite(&parent_offset, sizeof(unsigned int), 1, file);
    write_string(node->mName, file);
    write_matrix(node->mTransformation, file);

    printf("%d) node: %s, parent offset: %d\n", offset, node->mName.C_Str(), parent_offset);

    for(unsigned int i = 0; i < node->mNumChildren; ++i) {
        aiNode *child = node->mChildren[i];
        write_skeleton_node(child, file, offset, current_offset);
    }
}

static void write_skeleton_hierarchy(aiNode *node, FILE *file) {

    unsigned int num_bones = 0;
    count_child_nodes(node, &num_bones);
    printf("Number of bones: %d\n", num_bones);
    fwrite(&num_bones, sizeof(unsigned int), 1, file);
    
    unsigned int start_offset = 0;
    write_skeleton_node(node, file, (unsigned int)-1, &start_offset);
}

void print_bones(aiNode *node, unsigned int *index) {
    
    printf("%d) %s\n", *index, node->mName.C_Str());

    for(unsigned int i = 0; i < node->mNumChildren; ++i) {
        ++(*index);
        aiNode *child = node->mChildren[i];
        print_bones(child, index);
    }
}

static bool find_bone_id_internal(aiNode *node, aiString name, unsigned int *id) {
    
    if(node->mName == name) {
        return true;
    } else {

        for(unsigned int i = 0; i < node->mNumChildren; ++i) {
            ++(*id);
            aiNode *child = node->mChildren[i];
            if(find_bone_id_internal(child, name, id)) {
                return true;
            }
        }
    }

    return false;
}

static int find_bone_id(aiNode *node, aiString name) {
    unsigned id = 0;
    assert(find_bone_id_internal(node, name, &id) == true);
    return id;
}

static aiNode *find_bone(aiNode *node, aiString name) {
    if(node->mName == name) {
        return node;
    } else {
        for(unsigned int i = 0; i < node->mNumChildren; ++i) {
            aiNode * result = find_bone(node->mChildren[i], name);
            if(result != nullptr) {
                return result;
            }
        }
    }
    return nullptr;
}

static bool is_bone_name(const aiScene *scene, aiNode *node) {
    for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[i];

        for(unsigned int j = 0; j < mesh->mNumBones; ++j) {
            aiBone *bone = mesh->mBones[j];
            if(bone->mName == node->mName) {
                return true;
            }
        }
    }
    return false;
}

static aiNode *find_root_node(const aiScene *scene) {
    
    assert(scene->mNumMeshes > 0);
    assert(scene->mMeshes[0]->mNumBones > 0);
    aiString test_bone_name =  scene->mMeshes[0]->mBones[0]->mName;
    
    aiNode *node = find_bone(scene->mRootNode, test_bone_name);
    assert(node != nullptr);

    while(node->mParent && is_bone_name(scene, node->mParent)) {
        node = node->mParent; 
    }

    return node;
}

void write_skeleton(const aiScene *scene, FILE *file, const char *skeleton_name, unsigned int num_animations) {
    unsigned int flags = 0;
    if(scene->HasAnimations()) {
        flags |= TWEEN_ANIMATIONS;
    } else {
        assert(!"Invalid code path");
    }
    printf("Flags: %d\n", flags);
    fwrite(&flags, sizeof(unsigned int), 1, file);
    
    
    aiNode *root_node = find_root_node(scene);
    assert(root_node);
    
    printf("Skeleton name: %s\n", skeleton_name);
    write_string_cstr(skeleton_name, file);
    write_skeleton_hierarchy(root_node, file);

    printf("Number of animations: %d\n", num_animations);
    fwrite(&num_animations, sizeof(unsigned int), 1, file);
}

void write_animation(const aiScene *scene, FILE *file, const char *animation_name) {

    assert(scene->mNumAnimations > 0);
    aiAnimation *animation = scene->mAnimations[0];

    /* NOTE: The animations are spected to have to same number of keyframes for each bone */
    assert(animation->mNumChannels > 0);
    unsigned int num_keyframes = animation->mChannels[0]->mNumPositionKeys; 
    assert(num_keyframes == animation->mChannels[0]->mNumRotationKeys);

    float duration = animation->mDuration/1000.0f;
    printf("Animation name: %s, duration: %f\n", animation_name, duration);
    write_string_cstr(animation_name, file);
    fwrite(&duration, sizeof(float), 1, file);
    fwrite(&num_keyframes, sizeof(unsigned int), 1, file);
    
    aiNode *root_node = find_root_node(scene);

    for(unsigned int keyframe_index = 0; keyframe_index < num_keyframes; ++keyframe_index) {
        
        fwrite(&animation->mNumChannels, sizeof(unsigned int), 1, file);

        for(unsigned int bone_index = 0; bone_index < animation->mNumChannels; ++bone_index) {

            aiNodeAnim *node = animation->mChannels[bone_index];
            unsigned int id = find_bone_id(root_node, node->mNodeName);
            write_key_frame(id, node->mPositionKeys[keyframe_index], node->mRotationKeys[keyframe_index], node->mScalingKeys[keyframe_index], file);
        }
    }
}

void write_model(const aiScene *scene, FILE *file) {

    unsigned int flags = 0;
    if(scene->HasMeshes()) {
        flags |= TWEEN_MODEL;
    }
    if(scene->HasAnimations()) {
        flags |= TWEEN_SKELETON;
    }
    printf("Flags: %d\n", flags);
    fwrite(&flags, sizeof(unsigned int), 1, file);
        

    printf("Number of meshes: %d\n", scene->mNumMeshes);
    fwrite(&scene->mNumMeshes, sizeof(unsigned int), 1, file);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[i];
        printf("vertices: %d\n", mesh->mNumVertices);
        fwrite(&mesh->mNumVertices, sizeof(unsigned int), 1, file);
        
        unsigned int num_indices = 0;
        for(unsigned int j = 0; j < mesh->mNumFaces; ++j) { 
            num_indices += mesh->mFaces[j].mNumIndices;
        }
        assert(num_indices > 0);
        printf("indices: %d\n", num_indices);
        fwrite(&num_indices, sizeof(unsigned int), 1, file);
        
        aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
        assert(mat->GetTextureCount(aiTextureType_DIFFUSE) == 1);
        aiString path;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
        printf("material: %s, length: %d\n", path.C_Str(), path.length);
        write_string(path, file);
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[i];

        for(unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            
            aiVector3D position = mesh->mVertices[j];
            aiVector3D normal = mesh->mNormals[j];
            aiVector3D texcoord = mesh->mTextureCoords[0][j];
            
            fwrite(&position.x, sizeof(float), 1, file);
            fwrite(&position.y, sizeof(float), 1, file);
            fwrite(&position.z, sizeof(float), 1, file);
            
            fwrite(&normal.x, sizeof(float), 1, file);
            fwrite(&normal.y, sizeof(float), 1, file);
            fwrite(&normal.z, sizeof(float), 1, file);
            
            fwrite(&texcoord.x, sizeof(float), 1, file);
            fwrite(&texcoord.y, sizeof(float), 1, file);

        }

        for(unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace face = mesh->mFaces[j];
            fwrite(face.mIndices, sizeof(unsigned int), face.mNumIndices, file);
        }
    }

    if(scene->HasAnimations()) {

        aiNode *root_node = find_root_node(scene);
        assert(root_node);
        
        unsigned int total_bones = 0;
        count_child_nodes(root_node, &total_bones);
        printf("Skeleton name: %s, total bones: %d\n", root_node->mName.C_Str(), total_bones);
        write_string(root_node->mName, file);
        fwrite(&total_bones, sizeof(unsigned int), 1, file);
        
        unsigned int last_num_bones = 0;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh *mesh = scene->mMeshes[i];
            printf("Mesh: %s, bones: %d\n", mesh->mName.C_Str(), mesh->mNumBones);
            fwrite(&mesh->mNumBones, sizeof(unsigned int), 1, file);
            
            for(unsigned int j = 0; j < mesh->mNumBones; ++j) {
                
                aiBone *bone = mesh->mBones[j];
                unsigned int id = find_bone_id(root_node, bone->mName);
                fwrite(&id, sizeof(unsigned int), 1, file);
                fwrite(&bone->mNumWeights, sizeof(unsigned int), 1, file);
                
                for(unsigned int weight_index = 0; weight_index < bone->mNumWeights; ++weight_index) {
                    fwrite(&bone->mWeights[weight_index].mVertexId, sizeof(unsigned int), 1, file);
                    fwrite(&bone->mWeights[weight_index].mWeight, sizeof(float), 1, file);

                }
                
                write_matrix(bone->mOffsetMatrix, file);
                
                printf("%d) bone: %s bone index: %d, num weights: %d\n", last_num_bones+j, bone->mName.C_Str(), id, bone->mNumWeights);
            
            }
            last_num_bones += mesh->mNumBones;
        }
    }
    
    printf("Model file write perfectly\n");

}

int main(void) {

    Assimp::Importer importer0;
    const aiScene *model = importer0.ReadFile("./data/Silly Dancing.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    if(model == nullptr) {
        printf("Assimp error: %s\n", importer0.GetErrorString());
        return 1;
    }
    
    Assimp::Importer importer1;
    const aiScene *skeleton = importer1.ReadFile("./data/Silly Dancing.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    if(skeleton == nullptr) {
        printf("Assimp error: %s\n", importer1.GetErrorString());
        return 1;
    }

    Assimp::Importer importer2;
    const aiScene *flair = importer2.ReadFile("./data/Flair.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    if(flair == nullptr) {
        printf("Assimp error: %s\n", importer2.GetErrorString());
        return 1;
    }

    Assimp::Importer importer3;
    const aiScene *sillydance = importer3.ReadFile("./data/Silly Dancing.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    if(sillydance == nullptr) {
        printf("Assimp error: %s\n", importer3.GetErrorString());
        return 1;
    }

    Assimp::Importer importer4;
    const aiScene *dancingtwerk = importer4.ReadFile("./data/Dancing Twerk.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    if(dancingtwerk == nullptr) {
        printf("Assimp error: %s\n", importer4.GetErrorString());
        return 1;
    }

    
    unsigned int magic = TWEEN_MAGIC;

    FILE *animation_file = fopen("./data/model.twa", "wb");
    if(animation_file == nullptr) {
        printf("Cannot open specified file\n");
        return 1;
    }
    
    printf("Magic Number: %d\n", magic);
    fwrite(&magic, sizeof(unsigned int), 1, animation_file);
    
    // NOTE: Write skeleton:
    write_skeleton(skeleton, animation_file, "constructor", 3);
    
    // NOTE: Write animation file
    write_animation(sillydance,   animation_file, "silly dance");
    write_animation(flair,   animation_file, "falir");
    write_animation(dancingtwerk, animation_file, "dancing twerk");
    
    printf("Skeleton and all animation written perfectly\n");

    fclose(animation_file);

    // NOTE: Write model file

    FILE *model_file = fopen("./data/model.twm", "wb");
    if(model_file == nullptr) {
        printf("Cannot open specified file\n");
        return 1;
    }

    printf("Magic Number: %d\n", magic);
    fwrite(&magic, sizeof(unsigned int), 1, model_file);
    write_model(model, model_file);
    
    fclose(model_file);

    return 0;
}
