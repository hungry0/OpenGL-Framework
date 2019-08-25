#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <vector>
#include <map>

#include "SkinnedTexture.h"
#include "Shader.h"

typedef uint32_t uint;

using namespace std;

#define NUM_BONES_PER_VEREX 4
#define INVALID_MATERIAL 0xFFFFFFFF

enum VB_TYPES {
    INDEX_BUFFER,
    POS_VB,
    NORMAL_VB,
    TEXCOORD_VB,
    BONE_VB,
    NUM_VBs
};

struct BoneInfo
{
    glm::mat4 BoneOffset;
    glm::mat4 FinalTransformation;

    BoneInfo()
    {
        BoneOffset = glm::mat4();
        FinalTransformation = glm::mat4();
    }
};

struct VertexBoneData 
{
    uint IDs[NUM_BONES_PER_VEREX];
    float weights[NUM_BONES_PER_VEREX];

    VertexBoneData()
    {
        Reset();
    }

    void Reset()
    {
        memset(IDs, 0, sizeof(IDs));
        memset(weights, 0, sizeof(weights));
    }

    void AddBoneData(uint BoneID, float Weight);
};

struct MeshEntry
{
    unsigned int NumIndices{ 0 };
    unsigned int BaseVertex{ 0 };
    unsigned int BaseIndex{ 0 };
    unsigned int MaterialIndex{ INVALID_MATERIAL };
};


class SkinnedMesh
{
public:
    SkinnedMesh();
    ~SkinnedMesh();

    bool LoadMesh(const string & fileName);

    void Render();

    void Render(const Shader & shader);

    inline uint NumBones() const
    {
        return m_NumBones;
    }

    void BoneTransform(float timeInSeconds, vector<glm::mat4> & transforms);

    void CalcInterpolatedScaling(aiVector3D & out, float animationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion & out, float animationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D & out, float animationTime, const aiNodeAnim* pNodeAnim);

    uint FindScaling(float animationTime, const aiNodeAnim* pNodeAnim);
    uint FindRotation(float animationTime, const aiNodeAnim* pNodeAnim);
    uint FindPosition(float animationTime, const aiNodeAnim* pNodeAnim);

    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string NodeName);

    void ReadNodeHeirarchy(float animationTime, const aiNode* pNode, const glm::mat4 & ParentTransform);

    bool InitFromScene(const aiScene* pScene, const string FileName);

    void InitMesh(uint MeshIndex, const aiMesh* paiMesh, vector<glm::vec3> & Positions, vector<glm::vec3> & Normals, vector<glm::vec2> & TexCoords,
        vector<VertexBoneData> & Bones, vector<unsigned int> & indices);

    void LoadBones(uint MeshIndex, const aiMesh* paiMesh, vector<VertexBoneData> & Bones);

    bool InitMaterials(const aiScene* pScene, const string FileName);

    void Clear();

private:

    GLuint m_VAO;
    GLuint m_Buffers[NUM_VBs];

    vector<MeshEntry> m_Entries;
    vector<SkinnedTexture*> m_Textures;

    map<string, uint> m_BoneMapping;
    uint m_NumBones;
    vector<BoneInfo> m_BoneInfo;
    glm::mat4 m_GlobalInverseTransform;

    const aiScene* m_pScene;
    Assimp::Importer m_Importer;
};


