#include "CMazeGenerator.h"
#include "IAttributeManager.h"
#include <nlohmann/json.hpp>
#include "IEngine.h"
#include <corecrt_math_defines.h>
#include "CFunctionObject.h"
#include "CScene.h"
//#define USE_E131


CMazeGenerator::CMazeGenerator()
	: m_pCellTemplate(nullptr)
	, m_pRootNode(nullptr)
	, m_mapDataFile(nullptr)
{
	SETCLASS("CMazeGenerator");

}

CMazeGenerator::~CMazeGenerator()
{
}

void CMazeGenerator::OnLostDevice(void)
{
	IAttributeObject::OnLostDevice();
}

void CMazeGenerator::OnResetDevice(IGraphicsDevice& device)
{
	IAttributeObject::OnResetDevice(device);
}
void CMazeGenerator::DeclareAttributes(IAttributeManager& atrMan)
{
	IAttributeObject::DeclareAttributes(atrMan);
	std::vector<std::string> vecFileTypes;
	vecFileTypes.emplace_back("jsn");
	vecFileTypes.emplace_back("json");
	AddAttribute(new CAtrFile("Maze File", this, &m_sMazeDataFile, vecFileTypes));


	std::vector<IAttributeDescription*> vecGenInputs;
	vecGenInputs.push_back(new CAtrDescUInt("Seed", 0, MAXUINT32));
	AddAttribute(new CAtrFunction("Generate Random Map", this, &FunctionGenerateMap, &vecGenInputs));

	AddAttribute(new CAtrReference("Template Cell", this, (IIdentifiable**)&m_pCellTemplate, atrMan.GetTypeVector("CScene")));
	AddAttribute(new CAtrReference("Maze Root Node", this, (IIdentifiable**)&m_pRootNode, atrMan.GetTypeVector("ISceneNode")));

	AddAttribute(new CAtrInt("Maze Width", this, &m_mazeData.width));
	AddAttribute(new CAtrInt("Maze Height", this, &m_mazeData.height));
}

void CMazeGenerator::InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent)
{
	IAttributeObject::InitialiseAttributeObject(atrMan, pParent);
	//atrMan.AddTickableObject(this);

}

void CMazeGenerator::PostLoad(void)
{
	IAttributeObject::PostLoad();


}

void CMazeGenerator::UninitialiseAttributeObject(IAttributeManager& atrMan)
{
	IAttributeObject::UninitialiseAttributeObject(atrMan);
	//	atrMan.RemoveTickableObject(this);
}

void CMazeGenerator::FunctionGenerateMap(CFunctionObject* p)
{
	CMazeGenerator* pMe = static_cast<CMazeGenerator*>(p->GetFunctionParent());
	if (p)
	{
		if (!pMe->m_mazeData.wallData.empty())
		{
			// Clear Maze.
			for (int y = 0; y < pMe->m_mazeData.height; ++y)
			{

				for (int x = 0; x < pMe->m_mazeData.width; ++x)
				{
					for (int i = 0; i < 4; ++i)
					{
						pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeData.wallData[y][x].pWalls[i]); SAFE_RELEASE(pMe->m_mazeData.wallData[y][x].pWalls[i]);
						pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeData.wallData[y][x].pPillars[i]); SAFE_RELEASE(pMe->m_mazeData.wallData[y][x].pPillars[i]);

					}
					pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeData.wallData[y][x].pFloor); SAFE_RELEASE(pMe->m_mazeData.wallData[y][x].pFloor);
					pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeData.wallData[y][x].pNode); SAFE_RELEASE(pMe->m_mazeData.wallData[y][x].pNode);

				}
				pMe->m_mazeData.wallData[y].clear();
			}
		}
		pMe->m_mazeData.wallData.clear();
		for (int y = 0; y < pMe->m_mazeData.height; ++y)
		{
			pMe->m_mazeData.wallData.emplace_back(std::vector<SCellData>(pMe->m_mazeData.width));
			for (int x = 0; x < pMe->m_mazeData.width; ++x)
			{

				// Mark Maze Cell as Solid.
				pMe->m_mazeData.wallData[y][x].Walls = 0xFF;
			}
		}
		pMe->CreateMazeMesh();

	}
}

void CMazeGenerator::CreateMazeMesh()
{
	ISceneNode* Walls[4];
	ISceneNode* Pillars[4];
	ISceneNode* Floor;

	// Get the walls and Pillars.
	if (m_pCellTemplate)
	{
		CScene* pCell = static_cast<CScene*>(m_pCellTemplate);
		pCell->FindSceneNode("WALL_NORTH", &Walls[0]);
		pCell->FindSceneNode("WALL_EAST", &Walls[1]);
		pCell->FindSceneNode("WALL_SOUTH", &Walls[2]);
		pCell->FindSceneNode("WALL_WEST", &Walls[3]);

		pCell->FindSceneNode("PILLAR_NE", &Pillars[0]);
		pCell->FindSceneNode("PILLAR_SE", &Pillars[1]);
		pCell->FindSceneNode("PILLAR_SW", &Pillars[2]);
		pCell->FindSceneNode("PILLAR_NW", &Pillars[3]);

		pCell->FindSceneNode("floor_tile", &Floor);
	}

	for (int y = 0; y < m_mazeData.height; ++y)
	{

		for (int x = 0; x < m_mazeData.width; ++x)
		{
			auto* pCurrent = &m_mazeData.wallData[y][x];
			pCurrent->pNode = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(m_pRootNode, false, false, "", false, false));
			pCurrent->pNode->SetPosition(VECTOR3(x * 5, 0.f, y * 5));
			pCurrent->pNode->SetParent((ISceneNode*)m_pRootNode);
			SAFE_ADDREF(pCurrent->pNode);
			pCurrent->pFloor = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(Floor, false, false, "", false, false));
			pCurrent->pFloor->SetParent(pCurrent->pNode);
			SAFE_ADDREF(pCurrent->pFloor);
			if (x == 0)
			{
				AddWall(3, pCurrent, Walls); AddPillar(3, pCurrent, Pillars);
			}

			if (x == m_mazeData.width - 1)
			{
				AddWall(1, pCurrent, Walls); AddPillar(1, pCurrent, Pillars);
			}

			if (y == 0)
			{
				AddWall(2, pCurrent, Walls); AddPillar(2, pCurrent, Pillars);
			}

			if (y == m_mazeData.height - 1)
			{
				AddWall(0, pCurrent, Walls); AddPillar(0, pCurrent, Pillars);
			}

			if (pCurrent->Walls != 0)
			{
				//AddWall(0, pCurrent, Walls);
				//AddPillar(0, pCurrent, Pillars);

			}
		}


	}
}

void CMazeGenerator::AddPillar(int i, SCellData* pCurrent, ISceneNode** Pillars)
{
	pCurrent->pPillars[i] = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(Pillars[i], false, false, "", false, false));
	pCurrent->pPillars[i]->SetParent(pCurrent->pNode);
	SAFE_ADDREF(pCurrent->pPillars[i]);
}

void CMazeGenerator::AddWall(int i, SCellData* pCurrent, ISceneNode** Walls)
{
	pCurrent->pWalls[i] = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(Walls[i], false, false, "", false, false));
	pCurrent->pWalls[i]->SetParent(pCurrent->pNode);
	SAFE_ADDREF(pCurrent->pWalls[i]);
}


