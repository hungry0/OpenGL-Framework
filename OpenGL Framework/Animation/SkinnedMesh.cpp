#include "SkinnedMesh.h"

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define GLCheckError() (glGetError() == GL_NO_ERROR)
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }

glm::mat4 make_glm_matrix4(const aiMatrix3x3 & aiMatrix);
glm::mat4 make_glm_matrix4(aiMatrix4x4 & aiMatrix);
glm::mat4 make_glm_matrix4(const aiMatrix4x4 & aiMatrix4x4);


void VertexBoneData::AddBoneData(uint BoneID, float Weight)
{
    for (uint i = 0; i < sizeof(IDs) / sizeof(IDs[0]); i++)
    {
        if (std::abs(weights[i]) < 1e-6)
        {
            IDs[i] = BoneID;
            weights[i] = Weight;
            return;
        }
    }

    assert(0);
}

SkinnedMesh::SkinnedMesh()
{
    m_VAO = 0;
    memset(m_Buffers, 0, sizeof(m_Buffers));
    m_NumBones = 0;
    m_pScene = NULL;
}

SkinnedMesh::~SkinnedMesh()
{
    Clear();
}

bool SkinnedMesh::LoadMesh(const string & fileName)
{
    Clear();

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

    bool Ret = false;

    m_pScene = m_Importer.ReadFile(fileName.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

    if (!m_pScene)
    {
        cout << "the model has problem . " << endl;
        return false;
    }

    m_GlobalInverseTransform = make_glm_matrix4(m_pScene->mRootNode->mTransformation.Transpose());
    m_GlobalInverseTransform = glm::inverse(m_GlobalInverseTransform);
    
    Ret = InitFromScene(m_pScene, fileName);

    glBindVertexArray(0);

    return Ret;
}

void SkinnedMesh::Render()
{
    glBindVertexArray(m_VAO);

    for (uint i = 0; i < m_Entries.size(); i++)
    {
        const uint MaterialIndex = m_Entries[i].MaterialIndex;

        assert(MaterialIndex < m_Textures.size());

        if (m_Textures[MaterialIndex])
        {
            m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES,
            m_Entries[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(uint) * m_Entries[i].BaseIndex),
            m_Entries[i].BaseVertex);
    }

    glBindVertexArray(0);
}

void SkinnedMesh::Render(const Shader & shader)
{
    glBindVertexArray(m_VAO);

    for (uint i = 0; i < m_Entries.size(); i++)
    {
        const uint MaterialIndex = m_Entries[i].MaterialIndex;

        assert(MaterialIndex < m_Textures.size());

        if (m_Textures[MaterialIndex])
        {
            m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        }

        shader.setInt("gColorMap", 0);

        glDrawElementsBaseVertex(GL_TRIANGLES,
            m_Entries[i].NumIndices,
            GL_UNSIGNED_INT,
            (void*)(sizeof(uint) * m_Entries[i].BaseIndex),
            m_Entries[i].BaseVertex);
    }

    glBindVertexArray(0);
}

void SkinnedMesh::BoneTransform(float timeInSeconds, vector<glm::mat4> & transforms)
{
    glm::mat4 Identity = glm::mat4(1.0f);

    float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
    float TimeInTicks = timeInSeconds * TicksPerSecond;
    float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

    ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

    transforms.resize(m_NumBones);

    for (uint i = 0; i < m_NumBones; i++)
    {
        transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}

void SkinnedMesh::CalcInterpolatedScaling(aiVector3D & out, float animationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumScalingKeys == 1) {
        out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint ScalingIndex = FindScaling(animationTime, pNodeAnim);
    uint NextScalingIndex = (ScalingIndex + 1);

    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);

    float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (animationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;

    assert(Factor >= 0.0f && Factor <= 1.0f);

    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    out = Start + Factor * Delta;
}

void SkinnedMesh::CalcInterpolatedRotation(aiQuaternion & out, float animationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumRotationKeys == 1)
    {
        out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    uint RotationIndex = FindRotation(animationTime, pNodeAnim);
    uint NextRotationIndex = (RotationIndex + 1);

    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

    float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
    float Factor = (animationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;

    assert(Factor >= 0.0f && Factor <= 1.0f);

    const aiQuaternion& Start = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& End = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

    aiQuaternion::Interpolate(out, Start, End, Factor);
    out = out.Normalize();
}

/*
    插值，其实就是求的当前动画播放时间占前一个key和后一个key的比值插出来
*/

void SkinnedMesh::CalcInterpolatedPosition(aiVector3D & out, float animationTime, const aiNodeAnim* pNodeAnim)
{
    if (pNodeAnim->mNumPositionKeys == 1)
    {
        out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    uint PositionIndex = FindPosition(animationTime, pNodeAnim);
    uint NextPositionIndex = (PositionIndex + 1);

    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);

    float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
    float Factor = (animationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;

    assert(Factor >= 0.0f && Factor <= 1.0f);

    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;

    aiVector3D Delta = End - Start;

    out = Start + Factor * Delta;

}

glm::uint SkinnedMesh::FindScaling(float animationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        if (animationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
            return i;
        }
    }

    assert(0);

    return 0;
}

glm::uint SkinnedMesh::FindRotation(float animationTime, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
    {
        if (animationTime < pNodeAnim->mRotationKeys[i + 1].mTime)
        {
            return i;
        }
    }

    assert(0);

    return 0;
}

glm::uint SkinnedMesh::FindPosition(float animationTime, const aiNodeAnim* pNodeAnim)
{
    for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
    {
        if (animationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
            return i;
    }

    assert(0);

    return 0;
}

const aiNodeAnim* SkinnedMesh::FindNodeAnim(const aiAnimation* pAnimation, const string NodeName)
{
    for (uint i = 0; i < pAnimation->mNumChannels; i++)
    {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (string(pNodeAnim->mNodeName.data) == NodeName)
        {
            return pNodeAnim;
        }
    }

    return NULL;
}

void SkinnedMesh::ReadNodeHeirarchy(float animationTime, const aiNode* pNode, const glm::mat4 & ParentTransform)
{
    string NodeName(pNode->mName.data);

    const aiAnimation* pAnimation = m_pScene->mAnimations[0];
    auto transformation = pNode->mTransformation;
    glm::mat4 NodeTransformation= make_glm_matrix4(transformation.Transpose());

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim)
    {
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, animationTime, pNodeAnim);
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Scaling.x, Scaling.y, Scaling.z));

        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, animationTime, pNodeAnim);
        glm::mat4 RotationM = make_glm_matrix4(RotationQ.GetMatrix().Transpose());

        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, animationTime, pNodeAnim);
        glm::mat4 TranslationM = glm::mat4(1.0f);
        TranslationM = glm::translate(TranslationM, glm::vec3(Translation.x, Translation.y, Translation.z));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneMapping.find(NodeName) != m_BoneMapping.end())
    {
        uint BoneIndex = m_BoneMapping[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
    }

    for (uint i = 0; i < pNode->mNumChildren; i++)
    {
        ReadNodeHeirarchy(animationTime, pNode->mChildren[i], GlobalTransformation);
    }
}

glm::mat4 make_glm_matrix4(const aiMatrix3x3 & aiMatrix)
{
    glm::vec4 a(aiMatrix.a1, aiMatrix.a2, aiMatrix.a3, 0.0f);
    glm::vec4 b(aiMatrix.b1, aiMatrix.b2, aiMatrix.b3, 0.0f);
    glm::vec4 c(aiMatrix.c1, aiMatrix.c2, aiMatrix.c3, 0.0f);
    glm::vec4 d(0.0f, 0.0f, 0.0f, 1.0f);

    glm::mat4 mat = glm::mat4(a,b,c,d);

    return mat;
}

glm::mat4 make_glm_matrix4(aiMatrix4x4 & aiMatrix)
{
    glm::vec4 a(aiMatrix.a1, aiMatrix.a2, aiMatrix.a3, aiMatrix.a4);
    glm::vec4 b(aiMatrix.b1, aiMatrix.b2, aiMatrix.b3, aiMatrix.b4);
    glm::vec4 c(aiMatrix.c1, aiMatrix.c2, aiMatrix.c3, aiMatrix.c4);
    glm::vec4 d(aiMatrix.d1, aiMatrix.d2, aiMatrix.d3, aiMatrix.d4);

    glm::mat4 mat = glm::mat4(a, b, c, d);

    return mat;
}

glm::mat4 make_glm_matrix4(const aiMatrix4x4 & aiMatrix)
{
    glm::vec4 a(aiMatrix.a1, aiMatrix.a2, aiMatrix.a3, aiMatrix.a4);
    glm::vec4 b(aiMatrix.b1, aiMatrix.b2, aiMatrix.b3, aiMatrix.b4);
    glm::vec4 c(aiMatrix.c1, aiMatrix.c2, aiMatrix.c3, aiMatrix.c4);
    glm::vec4 d(aiMatrix.d1, aiMatrix.d2, aiMatrix.d3, aiMatrix.d4);

    glm::mat4 mat = glm::mat4(a, b, c, d);

    return mat;
}

bool SkinnedMesh::InitFromScene(const aiScene* pScene, const string FileName)
{
    m_Entries.resize(pScene->mNumMeshes);
    m_Textures.resize(pScene->mNumMaterials);

    vector<glm::vec3> Positions;
    vector<glm::vec3> Normals;
    vector<glm::vec2> TexCoords;

    vector<VertexBoneData> Bones;
    vector<uint> indices;

    uint NumVertices = 0;
    uint NumIndices = 0;

    for (uint i = 0; i < m_Entries.size(); i++)
    {
        m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
        m_Entries[i].BaseVertex = NumVertices;
        m_Entries[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices += m_Entries[i].NumIndices;
    }

    Positions.reserve(NumVertices);
    Normals.reserve(NumVertices);
    TexCoords.reserve(NumVertices);
    Bones.resize(NumVertices);
    indices.reserve(NumIndices);

    for (uint i = 0; i < m_Entries.size(); i++)
    {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitMesh(i, paiMesh, Positions, Normals, TexCoords, Bones, indices);
    }

    if (!InitMaterials(pScene, FileName))
    {
        return false;
    }

    // 顶点
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[VB_TYPES::POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // uv
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[VB_TYPES::TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // normal
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[VB_TYPES::NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(NORMAL_LOCATION);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[VB_TYPES::BONE_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Bones[0]) * Bones.size(), &Bones[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(BONE_ID_LOCATION);
    glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (void*)0);

    glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
    glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (void*) (4 * sizeof(int)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[VB_TYPES::INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(), &indices[0], GL_STATIC_DRAW);

    GLenum error_code;
    error_code = glad_glGetError();

    if (error_code != GL_NO_ERROR) {
        fprintf(stderr, "ERROR %d in %s\n", error_code, "init buffer");
    }

    return glad_glGetError() == 0;
}

void SkinnedMesh::InitMesh(uint MeshIndex, const aiMesh* paiMesh, vector<glm::vec3> & Positions, vector<glm::vec3> & Normals, vector<glm::vec2> & TexCoords, vector<VertexBoneData> & Bones, vector<unsigned int> & indices)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.f);

    for (uint i = 0; i < paiMesh->mNumVertices; i++)
    {
        const aiVector3D* pPos = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Positions.push_back(glm::vec3(pPos->x, pPos->y, pPos->z));
        Normals.push_back(glm::vec3(pNormal->x, pNormal->y, pNormal->z));
        TexCoords.push_back(glm::vec2(pTexCoord->x, pTexCoord->y));
    }

    LoadBones(MeshIndex, paiMesh, Bones);

    for (uint i = 0; i < paiMesh->mNumFaces; i++)
    {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);

        indices.push_back(Face.mIndices[0]);
        indices.push_back(Face.mIndices[1]);
        indices.push_back(Face.mIndices[2]);
    }
}

void SkinnedMesh::LoadBones(uint MeshIndex, const aiMesh* paiMesh, vector<VertexBoneData> & Bones)
{
    for (uint i = 0; i < paiMesh->mNumBones; i++)
    {
        uint BoneIndex = 0;
        string BoneName(paiMesh->mBones[i]->mName.data);

        if (m_BoneMapping.find(BoneName) == m_BoneMapping.end())
        {
            BoneIndex = m_NumBones;
            m_NumBones++;

            BoneInfo bi;
            m_BoneInfo.push_back(bi);
            m_BoneInfo[BoneIndex].BoneOffset = make_glm_matrix4(paiMesh->mBones[i]->mOffsetMatrix.Transpose());
            m_BoneMapping[BoneName] = BoneIndex;
        }
        else
        {
            BoneIndex = m_BoneMapping[BoneName];
        }

        for (uint j = 0; j < paiMesh->mBones[i]->mNumWeights; j++)
        {
            uint VertexID = m_Entries[MeshIndex].BaseVertex + paiMesh->mBones[i]->mWeights[j].mVertexId;
            float Weight = paiMesh->mBones[i]->mWeights[j].mWeight;
            Bones[VertexID].AddBoneData(BoneIndex, Weight);
        }
    }
}

bool SkinnedMesh::InitMaterials(const aiScene* pScene, const string FileName)
{
    string::size_type SlashIndex = FileName.find_last_of('/');
    string Dir;

    if (SlashIndex == string::npos)
    {
        Dir = ".";
    }
    else if (SlashIndex == 0)
    {
        Dir = "/";
    }
    else
    {
        Dir = FileName.substr(0, SlashIndex);
    }

    bool Ret = true;

    for (uint i = 0; i < pScene->mNumMaterials; i++)
    {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        m_Textures[i] = NULL;

        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString Path;

            if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
            {
                string p(Path.data);

                if (p.substr(0, 2) == ".\\")
                {
                    p = p.substr(2, p.size() - 2);
                }

                string FullPath = Dir + "/" + p;

                m_Textures[i] = new SkinnedTexture(GL_TEXTURE_2D, FullPath.c_str());

                if (!m_Textures[i]->Load()) {
                    printf("Error loading texture '%s'\n", FullPath.c_str());
                    delete m_Textures[i];
                    m_Textures[i] = NULL;
                    Ret = false;
                }
                else {
                    printf("%d - loaded texture '%s'\n", i, FullPath.c_str());
                }
            }
        }
    }

    return Ret;
}

void SkinnedMesh::Clear()
{
    for (uint i = 0; i < m_Textures.size(); i++) {
        SAFE_DELETE(m_Textures[i]);
    }

    if (m_Buffers[0] != 0) {
        glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
    }

    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}