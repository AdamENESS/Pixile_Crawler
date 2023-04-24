#include "CInstancedMazeMesh.h"
#include "CScene.h"
#include "CMeshAnimation.h"
#include "IAttributeManager.h"
#include "ISceneNodeCamera.h"
#include "IBasicRenderer.h"
#include "IMaterial.h"
#include "IMeshEffect.h"
#include "IUtilities.h"
#include "CFunctionObject.h"

std::vector<std::string> vecMazeMeshTypeName = { "Floor",
	"Wall",
	"Pillar",
	"WallWithFrame",
	"DoorShut",
	"DoorOpen",
	"DoorLocked",
	"DoorBroken",
	"DoorWithFrame",
	"Uneditable",
	"StairsUp",
	"StairsDown",
	"Room",
	"Solid"
};
//DECLARE_ATTRIBUTE_OBJECT(CSceneNodeMesh, ISceneNode);

CInstancedMazeMesh::CInstancedMazeMesh(void)
	: ISceneNode()
	, m_pAnimation(NULL)
	, m_pAnimationPrev(NULL)
	, m_pMeshEffect(NULL)
	, m_pOnFrameTrigger(NULL)
	, m_pOnAnimationEndTrigger(NULL)
	, m_fTime(0.0)
	, m_fAnimationSpeed(1.0f)
	, m_fAnimationBlendTime(1.0f)
	, m_fCurrentAnimationBlendTime(0.0f)
	, m_fCurrentAnimationTime(0.0f)
	, m_fCurrentAnimationPrevTime(0.0f)
	, m_ShowNormals(false)
	, m_bTriggerFired(false)
	, m_bGenNormals(false)
{
	SETCLASS("CInstancedMazeMesh");
	SetIsPickable(false);
	SetHasDynamicAttributes(true);

	memset(m_pMaterialRefs, 0, sizeof(CAtrReference*) * MAX_MESH_SUBSETS);
	memset(m_pMaterials, 0, sizeof(IMaterial*) * MAX_MESH_SUBSETS);

	for (int mIndex = 0; mIndex < eMazeMeshType_Count; mIndex++)
		m_pMesh[mIndex] = nullptr;

	m_BoundingBox = CBoundingBox(VECTOR3(-5, -5, -5), VECTOR3(-5, -5, -5));
}

CInstancedMazeMesh::~CInstancedMazeMesh(void)
{
	SAFE_RELEASE(m_pAnimationPrev);
}

void CInstancedMazeMesh::DeclareAttributes(IAttributeManager& atrMan)
{
	ISceneNode::DeclareAttributes(atrMan);
	for (int mIndex = 0; mIndex < eMazeMeshType_Count; mIndex++)
	{
		AddAttribute(new CAtrReference(vecMazeMeshTypeName[mIndex] + "_Mesh", this, (IIdentifiable**)&m_pMesh[mIndex],
			atrMan.GetTypeVector("CMesh"), true, &HandleMeshPreChange, &HandleMeshPostChange));
	}
	AddAttribute(new CAtrReference("Effect", this, (IIdentifiable**)&m_pMeshEffect,
		atrMan.GetTypeVector("IMeshEffect"), true));
	AddAttribute(new CAtrReference("Animation", this, (IIdentifiable**)&m_pAnimation,
		atrMan.GetTypeVector("CMeshAnimation"), true, &HandleAnimationPreChange, &HandleAnimationPostChange));
	AddAttribute(new CAtrFloat("Animation Speed", this, &m_fAnimationSpeed));
	AddAttribute(new CAtrFloat("Animation Blend Time", this, &m_fAnimationBlendTime, nullptr, 0.0f));

	auto showNormals = new CAtrBool("Show Normals", this, &m_ShowNormals);
	showNormals->SetPostChangeCallback(&CInstancedMazeMesh::HandleShowNormalsChange);
	AddAttribute(showNormals);

	std::vector<IAttributeDescription*> vecOnFrameOutputs;
	vecOnFrameOutputs.push_back(new CAtrDescString("Current Animation"));
	vecOnFrameOutputs.push_back(new CAtrDescString("Next Animation"));
	vecOnFrameOutputs.push_back(new CAtrDescFloat("Current Time"));
	vecOnFrameOutputs.push_back(new CAtrDescFloat("Remaining Time"));
	m_pOnFrameTrigger = new CAtrTrigger("On Frame", this, &vecOnFrameOutputs, atrMan);
	AddAttribute(m_pOnFrameTrigger);

	m_pOnAnimationEndTrigger = new CAtrTrigger("On Animation End", this, &vecOnFrameOutputs, atrMan);
	AddAttribute(m_pOnAnimationEndTrigger);

	//AddAttribute(new CAtrFunction("Generate Normals", this, GenerateNormals));
	AddAttribute(new CAtrBool("Generate Normals", this, &m_bGenNormals));
}

void CInstancedMazeMesh::GenerateNormals(CFunctionObject* fn)
{
	//ns::debug::write
}

void CInstancedMazeMesh::HandleShowNormalsChange(IAttributeObject* parent, IAttribute* attr)
{
	auto val = *(bool*)attr->GetData()->GetRawData();
	ns::debug::write_line("CSceneNodeMesh::OnShowNormalsChanged: %d", (int)val);

	auto self = (CInstancedMazeMesh*)parent;

	if (!self->m_pMesh)
		return;
}

void CInstancedMazeMesh::DeclareAttributesFromXML(TiXmlElement* pNode, unsigned int& uiLargestAttributeID)
{
	DeclareAttributesFromXMLDefault(pNode, uiLargestAttributeID);
}

void CInstancedMazeMesh::PostLoad(void)
{
	ISceneNode::PostLoad();
	for (int mIndex = 0; mIndex < eMazeMeshType_Count; mIndex++)
	{
		if (!m_pMesh[mIndex])
			return;

		m_pMesh[mIndex]->AddMeshChangeListener(this, &HandleMeshPathChange);

		if (m_pMesh[mIndex]->GetDeviceMesh())
		{
			const std::vector<SDynamicAttributeDescription>& vecXMLLoadedAttributes = GetXMLLoadedAttributes();
			std::vector<SDynamicAttributeDescription>::const_iterator s_it;

			for (unsigned int i = 0; i < m_pMesh[mIndex]->GetDeviceMesh()->GetSubsetCount(); ++i)
			{
				std::string sName = vecMazeMeshTypeName[mIndex] + " - Material " + ToString(i);

				if (!m_pMaterialRefs[i])
				{
					m_pMaterialRefs[i] = new CAtrReference(sName, this, (IIdentifiable**)&m_pMaterials[i],
						GetAttributeManager()->GetTypeVector("IMaterial"));

					m_pMaterialRefs[i]->SetIsUniqueAttribute(true);
					m_pMaterialRefs[i]->SetAttributeManager(GetAttributeManager());

					for (s_it = vecXMLLoadedAttributes.begin(); s_it != vecXMLLoadedAttributes.end(); ++s_it)
					{
						if (s_it->sName == sName)
						{
							m_pMaterialRefs[i]->SetAttributeFromLoad(s_it->sValue);
							m_pMaterialRefs[i]->OffsetIDs(s_it->uiImportValueOffset, true);
							m_pMaterialRefs[i]->SetID(s_it->uiID);
							m_pMaterialRefs[i]->Resolve(*GetAttributeManager());
							break;
						}
					}

					AddAttribute(m_pMaterialRefs[i]);
				}
			}
		}
	}
	UpdateBoundingBox();
	AllocateCustomBoneTransforms();
}

bool CInstancedMazeMesh::ProcessCullList(IGraphicsDevice& device, ISceneNodeProjection& camera)
{
	for (auto& mazeInstance : m_vecInstancedMazeMesh)
	{
		auto mIndex = mazeInstance.m_uiType;

		if (m_pMesh[mIndex]->GetDeviceMesh())
		{
			int i = 0;
			mazeInstance.m_location.SetRotation(mazeInstance.m_v3Rotation);
			mazeInstance.m_location.SetPosition(mazeInstance.m_v3Position);
			mazeInstance.m_location.SetScale(mazeInstance.m_v3Scale);
			mazeInstance.m_location.UpdateCombinedMatrix(GetCombinedMatrix());
			CBoundingBox transformedAABB = m_BoundingBox;
			transformedAABB.Transform(mazeInstance.m_location.GetCombinedMatrix());

			mazeInstance.visible = camera.TestFrustumAABB(transformedAABB);

		}
	}
	return false;

}
bool CInstancedMazeMesh::OnCull(IGraphicsDevice& device, ISceneNodeProjection& camera) const
{

	//const_cast<CInstancedMazeMesh*>(this)->ProcessCullList(device, camera);

	return false;
}


void CInstancedMazeMesh::OnPreCompose(IGraphicsDevice& device, CScene& scene)
{

	//if (m_pMesh)
	//{
	//	if (m_pMesh->GetDeviceMesh())
	//	{
	//		if (m_pAnimation && !m_vecCustomBoneTransforms.empty())
	//		{
	//			///* CJS: test - don't blend the animations
	//			if (m_pAnimationPrev)
	//			{
	//				ns::debug::write_line("CSceneNodeMesh::OnPreCompose %f %f", m_fCurrentAnimationBlendTime, m_fCurrentAnimationTime);

	//				m_pMesh->GetDeviceMesh()->SetAnimationCurrent(m_pAnimationPrev->GetDeviceAnimation());
	//				m_pMesh->GetDeviceMesh()->SetAnimationTimeCurrent(m_fCurrentAnimationPrevTime);
	//				m_pMesh->GetDeviceMesh()->SetAnimationNext(m_pAnimation->GetDeviceAnimation());
	//				m_pMesh->GetDeviceMesh()->SetAnimationTimeNext(m_fCurrentAnimationTime);
	//				m_pMesh->GetDeviceMesh()->SetAnimationBlendAmount(m_fCurrentAnimationBlendTime / m_fAnimationBlendTime);
	//			}
	//			else
	//				//*/
	//			{
	//				m_pMesh->GetDeviceMesh()->SetAnimationCurrent(m_pAnimation->GetDeviceAnimation());
	//				m_pMesh->GetDeviceMesh()->SetAnimationTimeCurrent(m_fCurrentAnimationTime);
	//				m_pMesh->GetDeviceMesh()->SetAnimationBlendAmount(0.0f);
	//			}

	//			int iCounter = 0;

	//			for (auto& it : m_pMesh->GetDeviceMesh()->GetSubsets())
	//			{
	//				it->GetCurrentBoneTransforms(m_vecCustomBoneTransforms[iCounter].second.data(),
	//					(unsigned int)m_vecCustomBoneTransforms[iCounter].second.size());
	//				++iCounter;
	//			}
	//		}
	//	}
	//}
}

void CInstancedMazeMesh::OnCompose(IGraphicsDevice& device, CScene& scene, const MATRIX& matrixWorld)
{
	for (int mIndex = 0; mIndex < eMazeMeshType_Count; mIndex++)
	{
		if (m_pMesh[mIndex])
		{
			if (m_pMesh && IsVisible())
			{
				m_pMesh[mIndex]->Compose(device, scene, *this);
			}
		}
	}

	for (auto& mazeInstance : m_vecInstancedMazeMesh)
	{
		if (!mazeInstance.visible)
			continue;

		auto mIndex = mazeInstance.m_uiType;

		if (m_pMesh[mIndex] && m_pMesh[mIndex]->GetDeviceMesh())
		{
			int i = 0;
			mazeInstance.m_location.SetRotation(mazeInstance.m_v3Rotation);
			mazeInstance.m_location.SetPosition(mazeInstance.m_v3Position);
			mazeInstance.m_location.SetScale(mazeInstance.m_v3Scale);
			mazeInstance.m_location.UpdateCombinedMatrix(GetCombinedMatrix());
			for (auto& it : m_pMesh[mIndex]->GetDeviceMesh()->GetSubsets())
			{


				if (m_pMeshEffect && m_pMeshEffect->GetMaterial())
					scene.AddToRenderList(m_pMeshEffect->GetMaterial(), this, it, &mazeInstance.m_location, true);
				else if (m_pMaterials[i] != nullptr)
					scene.AddToRenderList(m_pMaterials[i]->GetDeviceMaterial(), this, it, &mazeInstance.m_location, false);
				else
					scene.AddToRenderList(device.GetDefaultMaterial(), this, it, &mazeInstance.m_location, false);

				++i;
			}
		}
	}
}



void CInstancedMazeMesh::OnUpdate(IGraphicsDevice& device, CScene& scene, float dt)
{
	m_fTime += dt * m_fAnimationSpeed;

	if (m_pAnimation)
		m_fCurrentAnimationTime += dt * m_pAnimation->GetDeviceAnimation().fTimeScale * m_fAnimationSpeed;

	// if we are transitioning between two animations
	if (m_pAnimationPrev)
	{
		m_fCurrentAnimationPrevTime += dt * m_pAnimationPrev->GetDeviceAnimation().fTimeScale * m_fAnimationSpeed;
		m_fCurrentAnimationBlendTime += dt;

		if (m_fCurrentAnimationBlendTime > m_fAnimationBlendTime)
		{
			SAFE_RELEASE(m_pAnimationPrev);
			m_fCurrentAnimationBlendTime = 0.0f;
		}
	}

	//if (!m_pOnAnimationEndTrigger->GetIDs().empty())
	{
		std::string sCurrentAnimation, sNextAnimation;
		float fTotalAnimationTime, fRemainingAnimationTime;

		if (m_pAnimation)
		{
			sCurrentAnimation = m_pAnimation->GetName();
			sNextAnimation = m_pAnimation->GetName();

			if (m_pAnimation->GetDeviceAnimation().fFrameRate == 0.0f || m_pAnimation->GetDeviceAnimation().fTimeScale == 0.0f)
			{
				fTotalAnimationTime = 0.0f;
				fRemainingAnimationTime = 0.0f;
			}
			else
			{
				fTotalAnimationTime = (m_pAnimation->GetDeviceAnimation().iEndKey - m_pAnimation->GetDeviceAnimation().iStartKey) /
					m_pAnimation->GetDeviceAnimation().fFrameRate / m_pAnimation->GetDeviceAnimation().fTimeScale;

				fRemainingAnimationTime = fTotalAnimationTime == 0.0f ? 0.0f : fTotalAnimationTime - (m_fCurrentAnimationTime / m_pAnimation->GetDeviceAnimation().fTimeScale);

				if (fRemainingAnimationTime <= 0.f && !m_bTriggerFired)
				{
					m_bTriggerFired = true;

					if (m_pOnAnimationEndTrigger /* && !m_pOnAnimationEndTrigger->GetIDs().empty()*/)
					{
						m_pOnAnimationEndTrigger->SetOutput(0, &sCurrentAnimation);
						m_pOnAnimationEndTrigger->SetOutput(1, &sNextAnimation);
						m_pOnAnimationEndTrigger->SetOutput(2, &m_fCurrentAnimationTime);
						m_pOnAnimationEndTrigger->SetOutput(3, &fRemainingAnimationTime);
						m_pOnAnimationEndTrigger->Fire();
					}
				}
			}
		}
	}

	if (!m_pOnFrameTrigger->GetIDs().empty())
	{
		std::string sCurrentAnimation, sNextAnimation;
		float fTotalAnimationTime, fRemainingAnimationTime;

		if (m_pAnimation)
		{
			sCurrentAnimation = m_pAnimation->GetName();
			sNextAnimation = m_pAnimation->GetName();

			if (m_pAnimation->GetDeviceAnimation().fFrameRate == 0.0f || m_pAnimation->GetDeviceAnimation().fTimeScale == 0.0f)
			{
				fTotalAnimationTime = 0.0f;
				fRemainingAnimationTime = 0.0f;
			}
			else
			{
				fTotalAnimationTime = (m_pAnimation->GetDeviceAnimation().iEndKey - m_pAnimation->GetDeviceAnimation().iStartKey) /
					m_pAnimation->GetDeviceAnimation().fFrameRate / m_pAnimation->GetDeviceAnimation().fTimeScale;

				fRemainingAnimationTime = fTotalAnimationTime == 0.0f ? 0.0f : fTotalAnimationTime - (m_fCurrentAnimationTime / m_pAnimation->GetDeviceAnimation().fTimeScale);
			}
		}
		else
		{
			sCurrentAnimation = "";
			sNextAnimation = "";
			fRemainingAnimationTime = 0.0f;
		}

		m_pOnFrameTrigger->SetOutput(0, &sCurrentAnimation);
		m_pOnFrameTrigger->SetOutput(1, &sNextAnimation);
		m_pOnFrameTrigger->SetOutput(2, &m_fCurrentAnimationTime);
		m_pOnFrameTrigger->SetOutput(3, &fRemainingAnimationTime);
		m_pOnFrameTrigger->Fire();
	}
}

void CInstancedMazeMesh::OnRender(IGraphicsDevice& device, ISceneNodeProjection& camera, IDeviceMeshSubset* pSubset)
{
	if (pSubset)
	{
		if (m_pMeshEffect)
		{
			m_pMeshEffect->PreRender(device, camera, *pSubset);

			device.RenderMeshSubset(*pSubset);

			if (m_ShowNormals)
				RenderNormals();

			m_pMeshEffect->PostRender(device, camera, *pSubset);
		}
	}
}

void CInstancedMazeMesh::RenderNormals()
{
	/*auto lines = std::vector<VECTOR3>();
	auto mesh = m_pMesh->GetDeviceMesh();

	for (auto& subset : mesh->GetSubsets())
	{
		//var normals = subset.
	}*/
}

bool CInstancedMazeMesh::OnPick(const VECTOR3& v3Origin, const VECTOR3& v3Direction, float& fHitDistance, const ISceneNodeProjection& camera)
{
	bool bIntersection = false;

	if (m_pMesh)
	{
		IBasicRenderer* pBasicRenderer = GetGraphicsDevice()->GetBasicRenderer();

		VECTOR3 v3TransformedOrigin, v3TransformedDirection;
		MATRIX matInversed;

		// convert ray to model space
		XMVECTOR vDeterminant;
		matInversed = XMMatrixInverse(&vDeterminant, GetCombinedMatrix());
		v3TransformedOrigin = XMVector3TransformCoord(v3Origin, matInversed);
		v3TransformedDirection = XMVector3Normalize(XMVector3TransformNormal(v3Direction, matInversed));

		XNA::OrientedBox box;
		XMStoreFloat3(&box.Center, m_BoundingBox.GetCenter());
		XMStoreFloat3(&box.Extents, m_BoundingBox.GetDimensions() / 2.0f);
		XMStoreFloat4(&box.Orientation, VECTOR4(0.0f, 0.0f, 0.0f, 1.0f));
		bIntersection = IntersectRayOrientedBox(v3TransformedOrigin, v3TransformedDirection, &box, &fHitDistance) ? true : false;

		if (bIntersection)
		{
			// convert hit distance to world space
			VECTOR3 v3LocalHitLocation = v3TransformedOrigin + v3TransformedDirection * fHitDistance;
			VECTOR3 v3WorldHitLocation = XMVector3TransformCoord(v3LocalHitLocation, this->GetCombinedMatrix());
			fHitDistance = XMVector3Length(v3WorldHitLocation - v3Origin).x;
		}
	}

	return bIntersection;
}

void CInstancedMazeMesh::SetMesh(const std::string& sMeshPath, EMazeMeshType type)
{

}

const std::vector<MATRIX>* CInstancedMazeMesh::GetCustomBoneTransforms(IDeviceMeshSubset* pSubset)
{
	for (auto& it : m_vecCustomBoneTransforms)
	{
		if (it.first == pSubset)
			return &it.second;
	}

	return NULL;
}

const MATRIX* CInstancedMazeMesh::GetCustomBoneTransform(unsigned int uiBoneIndex)
{
	if (uiBoneIndex < m_vecIndividualBoneTransforms.size())
		return m_vecIndividualBoneTransforms[uiBoneIndex];
	else
		return NULL;
}

void CInstancedMazeMesh::HandleMeshPathChange(IAttributeObject* pThis)
{
	CInstancedMazeMesh* pMe = (CInstancedMazeMesh*)pThis;

	pMe->UpdateMaterialReferences();
	//if (pMe->m_pMesh && pMe->m_pMesh->GetDeviceMesh())
	//{
		//pMe->m_BoundingBox = pMe->m_pMesh->GetDeviceMesh()->GetBoundingBox();
		//pMe->AllocateCustomBoneTransforms();
	//}
}

void CInstancedMazeMesh::UpdateBoundingBox(void)
{

	m_BoundingBox = CBoundingBox(VECTOR3(-5, -5, -5), VECTOR3(-5, -5, -5));

}

void CInstancedMazeMesh::AllocateCustomBoneTransforms(void)
{
	/*
	m_vecCustomBoneTransforms.clear();
	m_vecIndividualBoneTransforms.clear();

	if (m_pMesh && m_pMesh->GetDeviceMesh())
	{
		for (auto& it : m_pMesh->GetDeviceMesh()->GetSubsets())
		{
			std::vector<MATRIX> vecTransforms(it->GetInternalBoneCount());
			m_vecCustomBoneTransforms.push_back(std::make_pair(it, vecTransforms));

			for (auto& m : m_vecCustomBoneTransforms.back().second)
			{
				m_vecIndividualBoneTransforms.push_back(&m);
			}
		}
	}*/
}

void CInstancedMazeMesh::HandleMeshPreChange(IAttributeObject* pThis, IAttribute* p)
{
	CInstancedMazeMesh* pMe = (CInstancedMazeMesh*)pThis;

	//if (pMe->m_pMesh)
		//pMe->m_pMesh->RemoveMeshChangeListener(pMe, &HandleMeshPathChange);
}

void CInstancedMazeMesh::HandleMeshPostChange(IAttributeObject* pThis, IAttribute* p)
{
	CInstancedMazeMesh* pMe = (CInstancedMazeMesh*)pThis;

	pMe->UpdateMaterialReferences();
	pMe->UpdateBoundingBox();
	pMe->AllocateCustomBoneTransforms();

	//if (pMe->m_pMesh)
	//	pMe->m_pMesh->AddMeshChangeListener(pMe, &HandleMeshPathChange);
}

void CInstancedMazeMesh::HandleAnimationPreChange(IAttributeObject* pThis, IAttribute* p)
{
	CInstancedMazeMesh* pMe = (CInstancedMazeMesh*)pThis;
	SAFE_ADDREF(pMe->m_pAnimation);
	SAFE_RELEASE(pMe->m_pAnimationPrev);
	pMe->m_pAnimationPrev = pMe->m_pAnimation;
	pMe->m_fCurrentAnimationBlendTime = 0.0f;
	pMe->m_fCurrentAnimationPrevTime = pMe->m_fCurrentAnimationTime;
	pMe->m_fCurrentAnimationTime = 0.0f;
	pMe->m_bTriggerFired = 0.f;
}

void CInstancedMazeMesh::HandleAnimationPostChange(IAttributeObject* pThis, IAttribute* p)
{
	CInstancedMazeMesh* pMe = (CInstancedMazeMesh*)pThis;

	/*if (pMe->m_pAnimationPrev && pMe->m_pAnimationPrev->GetDeviceAnimation().pAudio)
		pMe->m_pAnimationPrev->GetDeviceAnimation().pAudio->ObjectStop();*/

	if (pMe->m_pAnimation && pMe->m_pAnimation->GetDeviceAnimation().pAudio)
		pMe->m_pAnimation->GetDeviceAnimation().pAudio->ObjectPlay();
}

void CInstancedMazeMesh::UpdateMaterialReferences(void)
{
	for (int mIndex = 0; mIndex < eMazeMeshType_Count; mIndex++)
	{
		if (m_pMesh[mIndex] && m_pMesh[mIndex]->GetDeviceMesh())
		{
			const std::vector<IDeviceMeshSubset*>& vecSubsets = m_pMesh[mIndex]->GetDeviceMesh()->GetSubsets();

			for (unsigned int i = 0; i < MAX_MESH_SUBSETS; ++i)
			{
				if (i < vecSubsets.size())
				{
					if (!m_pMaterialRefs[i])
					{
						std::string sName = vecMazeMeshTypeName[mIndex] + " - Material " + ToString(i);

						m_pMaterialRefs[i] = new CAtrReference(sName, this, (IIdentifiable**)&m_pMaterials[i],
							GetAttributeManager()->GetTypeVector("IMaterial"));
						m_pMaterialRefs[i]->SetAttributeManager(GetAttributeManager());
						m_pMaterialRefs[i]->SetIsUniqueAttribute(true);

						AddAttribute(m_pMaterialRefs[i]);
					}
				}
				else
				{
					if (m_pMaterialRefs[i])
					{
						RemoveAttribute(m_pMaterialRefs[i]);
						m_pMaterialRefs[i] = nullptr;
					}

					m_pMaterials[i] = nullptr;
				}
			}
		}
		else
			RemoveAllMaterialReferences();
	}
}

void CInstancedMazeMesh::RemoveAllMaterialReferences(void)
{
	for (unsigned int i = 0; i < MAX_MESH_SUBSETS; ++i)
	{
		if (m_pMaterialRefs[i])
		{
			RemoveAttribute(m_pMaterialRefs[i]);
			m_pMaterialRefs[i] = nullptr;
		}

		m_pMaterials[i] = nullptr;
	}
}

bool CInstancedMazeMesh::ExposeDynamicAttributes(void)
{
	const std::vector<SDynamicAttributeDescription>& vecXMLLoadedAttributes = GetXMLLoadedAttributes();
	std::vector<SDynamicAttributeDescription>::const_iterator s_it;
	//unsigned int uiMaterialNo = 0;

	for (s_it = vecXMLLoadedAttributes.begin(); s_it != vecXMLLoadedAttributes.end(); ++s_it)
	{
		size_t noPos = s_it->sName.find("Material ");

		if (noPos != s_it->sName.npos)
		{
			std::string sNo = s_it->sName.substr(9);
			unsigned int  uiMaterialNo = FromString<unsigned int>(sNo);
			m_pMaterialRefs[uiMaterialNo] = new CAtrReference(s_it->sName, this, (IIdentifiable**)&m_pMaterials[uiMaterialNo],
				GetAttributeManager()->GetTypeVector("IMaterial"));
			m_pMaterialRefs[uiMaterialNo]->SetAttributeManager(GetAttributeManager());
			m_pMaterialRefs[uiMaterialNo]->SetIsUniqueAttribute(true);
			m_pMaterialRefs[uiMaterialNo]->SetAttributeFromLoad(s_it->sValue);
			m_pMaterialRefs[uiMaterialNo]->OffsetIDs(s_it->uiImportValueOffset, true);
			m_pMaterialRefs[uiMaterialNo]->SetID(s_it->uiID);

			AddAttribute(m_pMaterialRefs[uiMaterialNo]);
		}
	}

	return ISceneNode::ExposeDynamicAttributes();
}

bool CInstancedMazeMesh::PointInRect(float x, float z, int locX, int locY, int width, int height)
{
	// calculate if the point is in the rectangle
	if (x >= locX - width && x <= locX + width && z >= locY - width && z <= locY + height)
		return true;
	else
		return false;
}

void CInstancedMazeMesh::SetStairsPosAndRotation(uint32_t rot, float& posx, float& posz, bool direction)
{
	switch (rot)
	{
	case 0:
		posx += 0;  posz += 2.5;
		break;

	case 1:
		posx += 2.5; posz += 0;

		break;

	case 2:
		posx += 0; posz += -2.5;
		break;

	case 3:
		posx += -2.5; posz += 0;

		break;
	default:
		break;

	}
}

bool CInstancedMazeMesh::AddNewMeshInstance(float posx, float posy, float posz, uint32_t rot, uint32_t type)
{
	auto fRot = rot * 90.f;
	switch (type)
	{
	case eMazeMeshType_Floor:

		break;
	case eMazeMeshType_Wall:
		SetWallPosAndRotation(rot, posx, posz);
		break;
	case eMazeMeshType_Pillar:
		SetPillarPosAndRotation(rot, posx, posz);
		break;
	case eMazeMeshType_StairsDown:
		SetStairsPosAndRotation(rot, posx, posz, false);
		//fRot += 180;
		break;
	case eMazeMeshType_StairsUp:
		SetStairsPosAndRotation(rot, posx, posz, true);

		break;
	default:
		break;
	}

	for (auto& mazeInstance : m_vecInstancedMazeMesh)
	{
		if (mazeInstance.m_v3Position.x == posx
			&& mazeInstance.m_v3Position.y == posy
			&& mazeInstance.m_v3Position.z == posz
			//&& mazeInstance.m_v3Rotation.y == fRot
			&& mazeInstance.m_uiType == type)
		{
			return false;
		}
	}
	SInstancedMazeMesh newInstance;
	newInstance.m_v3Position = VECTOR3(posx, posy, posz);
	newInstance.m_v3Rotation = VECTOR3(0, fRot, 0);
	newInstance.m_uiType = type;
	if (type == eMazeMeshType_Floor)
	{
		newInstance.m_v3Scale = VECTOR3(1.25, 1, 1.25);
		if (posy != 0)
			newInstance.m_v3Rotation = VECTOR3(0, fRot, 180);
	}
	else if (type == eMazeMeshType_StairsUp || type == eMazeMeshType_StairsDown)
	{
		newInstance.m_v3Scale = VECTOR3(1.25, 1, 1.25);
	}
	else
	{
		if (rot == 0 || rot == 1)
			newInstance.m_v3Scale = VECTOR3(-1, 1, -1);
		else
			newInstance.m_v3Scale = VECTOR3(1, 1, 1);
	}
	m_vecInstancedMazeMesh.push_back(newInstance);
	// Add proper true false, if the type has already been added.

	return true;
}

void CInstancedMazeMesh::SetWallPosAndRotation(uint32_t rot, float& posx, float& posz)
{
	switch (rot)
	{
	case 0:
		posx += 0;
		posz += 2.5;
		break;

	case 1:
		posx += 2.5;
		posz += 0;

		break;

	case 2:
		posx += 0;
		posz += -2.5;
		break;

	case 3:
		posx += -2.5;
		posz += 0;

		break;
	default:
		break;

	}
}

void CInstancedMazeMesh::SetPillarPosAndRotation(uint32_t rot, float& posx, float& posz)
{
	switch (rot)
	{
	case 0:
		posx += 2.5;
		posz += 2.5;
		break;

	case 1:
		posx += 2.5;
		posz += -2.5;

		break;

	case 2:
		posx += -2.5;
		posz += -2.5;
		break;

	case 3:
		posx += -2.5;
		posz += 2.5;

		break;
	default:
		break;

	}
}

void CInstancedMazeMesh::UpdateMeshInstanceVisibility(int locX, int locY, int facing)
{
	for (auto& mazeInstance : m_vecInstancedMazeMesh)
	{

		if (PointInRect(mazeInstance.m_v3Position.x, mazeInstance.m_v3Position.z, locX * 5, locY * 5, 30, 30))
		{
			mazeInstance.visible = true;
		}
		else
		{
			mazeInstance.visible = false;
		}
	}
}
