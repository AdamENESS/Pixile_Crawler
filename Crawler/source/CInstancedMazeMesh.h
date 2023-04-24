#pragma once

#include "ISceneNode.h"
#include "CMesh.h"
#include "CCommonMesh.h"
#include <vector>

class IMeshEffect;
class CMeshAnimation;
enum EMazeMeshType
{
	eMazeMeshType_Floor = 0,
	eMazeMeshType_Wall,
	eMazeMeshType_Pillar,
	eMazeMeshType_WallWithFrame,
	eMazeMeshType_DoorShut,
	eMazeMeshType_DoorOpen,
	eMazeMeshType_DoorLocked,
	eMazeMeshType_DoorBroken,
	eMazeMeshType_DoorWithFrame,
	eMazeMeshType_Uneditable,
	eMazeMeshType_StairsUp,
	eMazeMeshType_StairsDown,
	eMazeMeshType_Room,
	eMazeMeshType_Solid,
	eMazeMeshType_Count
};

/**/

struct SInstancedMazeMesh
{
	VECTOR3 m_v3Position{ 0.0f, 0.0f, 0.0f };
	VECTOR3 m_v3Rotation{ 0.0f, 0.0f, 0.0f };
	VECTOR3 m_v3Scale{ 0.0f, 0.0f, 0.0f };
	ILocation m_location;
	uint32_t m_uiType{};
	bool visible{ true };

};

class CInstancedMazeMesh : public ISceneNode
{
public:
	CInstancedMazeMesh(void);
	virtual ~CInstancedMazeMesh(void);

	CMesh* GetMesh() { return m_pMesh[0]; }
	CMesh* GetMesh(EMazeMeshType type) { return m_pMesh[type]; }
	void SetMesh(CMesh* pMesh, EMazeMeshType type) { m_pMesh[type] = pMesh; }
	void SetMesh(CMesh* pMesh) { m_pMesh[0] = pMesh; }
	void SetMesh(const std::string& sMeshPath, EMazeMeshType type);


	virtual const std::vector<MATRIX>* GetCustomBoneTransforms(IDeviceMeshSubset* pSubset);
	const MATRIX* GetCustomBoneTransform(unsigned int uiBoneIndex);

	bool AddNewMeshInstance(float posx, float posy, float posz, uint32_t rot, uint32_t type);

	void SetWallPosAndRotation(uint32_t rot, float& posx, float& posz);
	void SetPillarPosAndRotation(uint32_t rot, float& posx, float& posz);
	void UpdateMeshInstanceVisibility(int locX, int locY, int facing);
protected:
	void OnPreCompose(IGraphicsDevice& device, CScene& scene) override;
	void OnCompose(IGraphicsDevice& device, CScene& scene, const MATRIX& matrixWorld) override;
	void OnUpdate(IGraphicsDevice& device, CScene& scene, float dt) override;
	void OnRender(IGraphicsDevice& device, ISceneNodeProjection& camera, IDeviceMeshSubset* pSubset) override;
	bool OnPick(const VECTOR3& v3Origin, const VECTOR3& v3Direction, float& fHitDistance, const ISceneNodeProjection& camera) override;
	void DeclareAttributes(IAttributeManager& atrMan) override;
	void DeclareAttributesFromXML(TiXmlElement* pNode, unsigned int& uiLargestAttributeID) override;
	void PostLoad(void) override;
	virtual bool OnCull(IGraphicsDevice& device, ISceneNodeProjection& camera) const;

	bool ProcessCullList(IGraphicsDevice& device, ISceneNodeProjection& camera);

private:
	static void HandleMeshPathChange(IAttributeObject* pThis);
	static void HandleMeshPreChange(IAttributeObject* pThis, IAttribute* p);
	static void HandleMeshPostChange(IAttributeObject* pThis, IAttribute* p);

	static void HandleAnimationPreChange(IAttributeObject* pThis, IAttribute* p);
	static void HandleAnimationPostChange(IAttributeObject* pThis, IAttribute* p);
	static void HandleShowNormalsChange(IAttributeObject* parent, IAttribute* attr);

	void UpdateMaterialReferences(void);
	void RemoveAllMaterialReferences(void);
	void UpdateBoundingBox(void);
	void AllocateCustomBoneTransforms(void);
	void RenderNormals();

	bool ExposeDynamicAttributes(void) override;

	bool PointInRect(float x, float z, int locX, int locY, int param5, int param6);
	void SetStairsPosAndRotation(uint32_t rot, float& posx, float& posz, bool direction);
protected:
	IMeshEffect* GetMeshEffect(void) { return m_pMeshEffect; }
	IMaterial* GetMaterial(int index) { return m_pMaterials[index]; }
private:
	std::vector<std::pair<IDeviceMeshSubset*, std::vector<MATRIX>>> m_vecCustomBoneTransforms;
	std::vector<MATRIX*> m_vecIndividualBoneTransforms;

	CMesh* m_pMesh[eMazeMeshType_Count];
	CMeshAnimation* m_pAnimation;
	CMeshAnimation* m_pAnimationPrev;
	IMeshEffect* m_pMeshEffect;
	CAtrTrigger* m_pOnFrameTrigger;
	CAtrTrigger* m_pOnAnimationEndTrigger;

	CAtrReference* m_pMaterialRefs[MAX_MESH_SUBSETS];
	IMaterial* m_pMaterials[MAX_MESH_SUBSETS];

	float m_fTime;
	float m_fAnimationSpeed;
	float m_fAnimationBlendTime;
	float m_fCurrentAnimationBlendTime;
	float m_fCurrentAnimationTime;
	float m_fCurrentAnimationPrevTime;
	bool m_ShowNormals;
	bool m_bTriggerFired;
	bool m_bGenNormals;

	static void GenerateNormals(CFunctionObject* fun);

	std::vector<SInstancedMazeMesh> m_vecInstancedMazeMesh;
};
