#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/types.h>
#include <stdio.h>
#include <assert.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/vector3.h>
#include <assimp/material.h>
#include <assimp/mesh.h>

#define TWEEN_MAGIC ((unsigned int)('E'<<24)|('E'<<16)|('W'<<8)|'T')
#define TWEEN_FLAGS 0

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

static void print_bones(aiNode *node, unsigned int *index) {
    
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

int main(void) {

    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile("./data/model.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 

    if(scene == nullptr) {
        printf("Assimp error: %s\n", importer.GetErrorString());
        return 1;
    }
   
    FILE *file = fopen("./data/Angry.twa", "wb");

    if(file == nullptr) {
        printf("Cannot open specified file\n");
        return 1;
    }
    
    unsigned int magic = TWEEN_MAGIC;

    printf("Magic Number: %d\n", magic);
    fwrite(&magic, sizeof(unsigned int), 1, file);
    
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
    
    printf("Skeleton name: %s\n", root_node->mName.C_Str());
    write_string(root_node->mName, file);
    write_skeleton_hierarchy(root_node, file);

    printf("Number of animations: %d\n", scene->mNumAnimations);
    fwrite(&scene->mNumAnimations, sizeof(unsigned int), 1, file);
    

    for(unsigned int animation_index = 0; animation_index < scene->mNumAnimations; ++animation_index) {
        aiAnimation *animation = scene->mAnimations[animation_index];

        /* NOTE: The animations are spected to have to same number of keyframes for each bone */
        assert(animation->mNumChannels > 0);
        unsigned int num_keyframes = animation->mChannels[0]->mNumPositionKeys; 
        assert(num_keyframes == animation->mChannels[0]->mNumRotationKeys);

        float duration = animation->mDuration/1000.0f;
        printf("Animation name: %s, duration: %f\n", animation->mName.C_Str(), duration);
        write_string(animation->mName, file);
        fwrite(&duration, sizeof(float), 1, file);
        fwrite(&num_keyframes, sizeof(unsigned int), 1, file);

        for(unsigned int keyframe_index = 0; keyframe_index < num_keyframes; ++keyframe_index) {
            
            fwrite(&animation->mNumChannels, sizeof(unsigned int), 1, file);

            for(unsigned int bone_index = 0; bone_index < animation->mNumChannels; ++bone_index) {

                aiNodeAnim *node = animation->mChannels[bone_index];
                unsigned int id = find_bone_id(root_node, node->mNodeName);
                write_key_frame(id, node->mPositionKeys[keyframe_index], node->mRotationKeys[keyframe_index], node->mScalingKeys[keyframe_index], file);
            }
        }
    }

    fclose(file);

    printf("Animation file write perfectly\n");

    return 0;
}

int main2(void) {

    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile("./data/Angry.dae", 
            aiProcess_Triangulate           |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType); 
    
    if(scene == nullptr) {
        printf("Assimp error: %s\n", importer.GetErrorString());
        return 1;
    }
   
    FILE *file = fopen("./data/Angry.twm", "wb");

    if(file == nullptr) {
        printf("Cannot open specified file\n");
        return 1;
    }
    
    unsigned int magic = TWEEN_MAGIC;

    printf("Magic Number: %d\n", magic);
    fwrite(&magic, sizeof(unsigned int), 1, file);
    
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
        fwrite(&path.length, sizeof(unsigned int), 1, file);
        fwrite(path.data, sizeof(unsigned char), path.length, file);
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[i];

        for(unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            
            aiVector3D position = mesh->mVertices[j];
            aiVector3D normal = mesh->mNormals[j];
            aiVector3D texcoord = mesh->mTextureCoords[0][j];

            fwrite(&position, sizeof(float)*3, 1, file);
            fwrite(&normal, sizeof(float)*3, 1, file);
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
        
        printf("Skeleton name: %s\n", root_node->mName.C_Str());
        write_string(root_node->mName, file);

        unsigned int last_num_bones = 0;
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            aiMesh *mesh = scene->mMeshes[i];
            printf("Mesh: %s\n", mesh->mName.C_Str());
            for(unsigned int j = 0; j < mesh->mNumBones; ++j) {
                
                aiBone *bone = mesh->mBones[j];
                unsigned int id = find_bone_id(root_node, bone->mName);
                fwrite(&id, sizeof(unsigned int), 1, file);
                fwrite(&bone->mNumWeights, sizeof(unsigned int), 1, file);
                fwrite(bone->mWeights, sizeof(float), sizeof(float)*bone->mNumWeights, file);
                write_matrix(bone->mOffsetMatrix, file);

                printf("%d) bone: %s bone index: %d\n", last_num_bones+j, bone->mName.C_Str(), id);
            
            }
            last_num_bones += mesh->mNumBones;
        }
    }
    
    fclose(file);

    return 0;
}
