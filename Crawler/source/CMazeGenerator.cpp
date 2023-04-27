#include "CMazeGenerator.h"
#include "IAttributeManager.h"
#include <nlohmann/json.hpp>
#include "IEngine.h"
#include <corecrt_math_defines.h>
#include "CFunctionObject.h"
#include "CScene.h"
#include "ITexture.h"
#include "CInstancedMazeMesh.h"
#include "CEvents.h"

CMazeGenerator::CMazeGenerator()
	: m_pCellTemplate(nullptr)
	, m_pRootNode(nullptr)
	, m_mapDataFile(nullptr)
	, m_pMaze2DTexture(nullptr)
{
	SETCLASS("CMazeGenerator");
	m_pPlayerData = new SPlayerData();
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

	AddAttribute(new CAtrInt("Maze Width", this, &m_mazeDataOriginal.width));
	AddAttribute(new CAtrInt("Maze Height", this, &m_mazeDataOriginal.height));

	AddAttribute(new CAtrReference("Mini Map Texture", this, (IIdentifiable**)&m_pMaze2DTexture, atrMan.GetTypeVector("ITexture")));
}

void CMazeGenerator::InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent)
{
	IAttributeObject::InitialiseAttributeObject(atrMan, pParent);
	atrMan.AddTickableObject(this);

}

void CMazeGenerator::PostLoad(void)
{
	IAttributeObject::PostLoad();


}

void CMazeGenerator::UninitialiseAttributeObject(IAttributeManager& atrMan)
{
	IAttributeObject::UninitialiseAttributeObject(atrMan);
	atrMan.RemoveTickableObject(this);
}

void CMazeGenerator::FunctionGenerateMap(CFunctionObject* p)
{
	CMazeGenerator* pMe = static_cast<CMazeGenerator*>(p->GetFunctionParent());
	if (p)
	{
		if (!pMe->m_mazeDataOriginal.wallData.empty())
		{
			// Clear Maze.
			for (int y = 0; y < pMe->m_mazeDataOriginal.height; ++y)
			{

				for (int x = 0; x < pMe->m_mazeDataOriginal.width; ++x)
				{
					for (int i = 0; i < 4; ++i)
					{
						if (pMe->m_mazeDataOriginal.wallData[y][x].pWalls[i])
							pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeDataOriginal.wallData[y][x].pWalls[i]); SAFE_RELEASE(pMe->m_mazeDataOriginal.wallData[y][x].pWalls[i]);

						if (pMe->m_mazeDataOriginal.wallData[y][x].pPillars[i])
							pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeDataOriginal.wallData[y][x].pPillars[i]); SAFE_RELEASE(pMe->m_mazeDataOriginal.wallData[y][x].pPillars[i]);

					}
					pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeDataOriginal.wallData[y][x].pFloor); SAFE_RELEASE(pMe->m_mazeDataOriginal.wallData[y][x].pFloor);
					pMe->GetAttributeManager()->RemoveAttributeObject(pMe->m_mazeDataOriginal.wallData[y][x].pNode); SAFE_RELEASE(pMe->m_mazeDataOriginal.wallData[y][x].pNode);

				}
				pMe->m_mazeDataOriginal.wallData[y].clear();
			}
		}
		pMe->m_mazeDataOriginal.wallData.clear();


		pMe->InitMaze();

		pMe->CreateMazeMesh();

	}
}

void CMazeGenerator::CreateZones(int count, int cx, int cy, int walltype, int sizemin, int sizemax)
{
	for (int i = 0; i < count; i++)
	{
		int tx = GetRandomMinMax(0, m_mazeDataOriginal.width);
		int ty = GetRandomMinMax(0, m_mazeDataOriginal.height);
		int width = GetRandomMinMax(sizemin, sizemax);
		int height = GetRandomMinMax(sizemin, sizemax);
		while (tx + width >= m_mazeDataOriginal.width)
			tx--;

		while (ty + height >= m_mazeDataOriginal.height)
			ty--;

		if (!FindPoint(tx, ty, tx + width, ty + height, cx, cy))
		{
			for (int y = ty; y < ty + height; ++y)
			{
				for (int x = tx; x < tx + width; ++x)
				{
					m_mazeDataOriginal.wallData[y][x].wallType = walltype;
				}
			}
		}
	}
}


int CMazeGenerator::GetRandomMinMax(int min, int max)
{
	return min + (rand() % (int)(max - min + 1));
}

void CMazeGenerator::DoMapGeneration()
{
	// Choose a position at the top of the map.
	int cx = GetRandomMinMax(1, 31);
	int cy = 1;
	m_pPlayerData->x = cx;
	m_pPlayerData->y = cy;
	int nx = cx;
	int ny = cy;

	m_mazeDataOriginal.wallData[cy][cx].wallType = 0;
	m_mazeDataOriginal.wallData[cy][cx].index = 1;

	int32_t currentId = 1;
	// Make some Un-Editable Areas.
	int walltype = 0xFE;
	int count = 30;
	CreateZones(count, cx, cy, walltype, 3, 9);
	CreateZones(60, cx, cy, eWallTypes_None, 3, 5);
	for (int i = 0; i < m_mazeDataOriginal.height; i++)
	{
		for (int j = 0; j < m_mazeDataOriginal.width; j++)
		{
			if (i % 32 == 0 || j % 32 == 0)
				m_mazeDataOriginal.wallData[i][j].wallType = eWallTypes_Uneditable;
		}
	}
	// Make the walker
	int MAX_STEPS = 32767;
	int MAX_JUMPS = 900;
	int currentLevel = 0;
	int moveDirTimes = 0;
	int dir = GetRandomMinMax(0, 3);
	for (int step = 0; step < MAX_STEPS; ++step)
	{
		/*
		DrawMaze2D(screenSurface);
		SDL_Rect rect;
		rect.x = 1 + (cx * 5);
		rect.y = 1 + (cy * 5);
		rect.w = 4;
		rect.h = 4;

		//SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 255, 0, 0));

		rect.x = 650;
		rect.y = 200;
		rect.w = 100.f * (float(step) / float(MAX_STEPS));
		rect.h = 16;
		//SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 255, 0, 0));
		//Update the surface
		SDL_UpdateWindowSurface(window);
		SDL_Delay(1);
		//ns::debug::write_line("%d", step);
		*/
		int tries = 0;

		bool stepNotDone = true;
		if (moveDirTimes > GetRandomMinMax(3, 7))
		{
			dir = GetRandomMinMax(0, 3);
			moveDirTimes = 0;
		}
		moveDirTimes++;
		// choose a direction to move in.

		while (stepNotDone)
		{

			nx = cx; ny = cy;

			switch (dir)
			{
			case 0: // North
			{
				ny--;
				break;
			}
			case 1: // EAST
			{
				nx--;
				break;
			}
			case 2: // SOUTH
			{
				ny++;
				break;
			}
			case 3: // WEST
			{
				nx++;
				break;
			}
			default:
				break;
			}

			if (IsAValidLocation(nx, ny))
			{
				cx = nx; cy = ny;
				stepNotDone = false;
				m_mazeDataOriginal.wallData[cy][cx].wallType = 0;
				currentId++;
				m_mazeDataOriginal.wallData[cy][cx].index = currentId;

				SMappedCell cell;

				cell.x = cx; cell.y = cy;

				m_path.emplace_back(cell);
				continue;
			}
			else
			{
				// We have hit a wall.
				// Move back to the last location.
				nx = cx;
				ny = cy;
			}
			tries++;
			//dir += GetRandomMinMax(1, 2);
			dir = GetRandomMinMax(0, 3);
			dir %= 4;

			if (tries > 8)
			{
				if (GetRandomMinMax(0, 100) < 10)
				{
					// Jump to a random location.
					if (MAX_JUMPS > 0)
					{
						m_mazeDataOriginal.wallData[cy][cx].wallType = eWallTypes_Solid;
						if (IsAValidLocation(cx, cy))
						{
							// choose a different grid location.
							int levelX = cx / 32;
							int levelY = cy / 32;

							int newX = levelX;

							int newY = levelY;
							while (newX == levelX && newY == levelY && newY == levelX && newX == levelY)
							{
								newX = GetRandomMinMax(0, 3);
								newY = GetRandomMinMax(0, 3);
							}
							nx = (newX * 32) + GetRandomMinMax(0, 31);
							ny = (newY * 32) + GetRandomMinMax(0, 31);

							if (IsAValidLocation(nx, ny))
							{
								int nextLevel = newY * 4 + newX;
								printf("%d %d %d %d\n", nx / 32, ny / 32, currentLevel, nextLevel);
								SStairLocation newStairs;
								if (nextLevel < currentLevel)
								{
									m_mazeDataOriginal.wallData[cy][cx].wallType = eWallTypes_StairsUp;
									newStairs.dx = cx;
									newStairs.dy = cy;

								}
								else
								{
									m_mazeDataOriginal.wallData[cy][cx].wallType = eWallTypes_StairsDown;
									newStairs.ux = cx;
									newStairs.uy = cy;

								}

								if (nextLevel > currentLevel)
								{
									m_mazeDataOriginal.wallData[ny][nx].wallType = eWallTypes_StairsUp;
									newStairs.dx = nx;
									newStairs.dy = ny;

								}
								else
								{
									m_mazeDataOriginal.wallData[ny][nx].wallType = eWallTypes_StairsDown;
									newStairs.ux = nx;
									newStairs.uy = ny;

								}
								currentId++;
								m_mazeDataOriginal.wallData[ny][nx].index = currentId;
								cx = nx; cy = ny;
								m_stairLocations.emplace_back(newStairs);
								MAX_JUMPS--;
								currentLevel = nextLevel;
								break;
							}
						}
					}
					m_mazeDataOriginal.wallData[cy][cx].wallType = eWallTypes_None;

				}
				else
				{
					// we have tried all 4 directions.
					// so choose a different starting point.
					if (m_path.empty())
					{
						cy = 0;
						cx = GetRandomMinMax(0, m_mazeDataOriginal.width - 1);
						m_pPlayerData->x = cx;
						m_pPlayerData->y = cy;
					}
					else
					{
						int newpos = GetRandomMinMax(0, m_path.size() - 1);

						if (m_mazeDataOriginal.wallData[m_path[newpos].y][m_path[newpos].x].wallType == eWallTypes_None)
						{
							cx = m_path[newpos].x;
							cy = m_path[newpos].y;
						}
						else
						{

						}
					}
					tries = 0;
				}
				break;
			}

		}
	}
}

void CMazeGenerator::InitEmptyMaze()
{
	auto seed = std::chrono::system_clock::now().time_since_epoch().count();
	m_sMapSeed = std::to_string(seed);
	srand(seed);

	//m_mazeData.width = 129;
	//m_mazeData.height = 129;
	m_mazeDataOriginal.wallData.resize(m_mazeDataOriginal.height);

	for (int i = 0; i < m_mazeDataOriginal.height; i++)
	{
		m_mazeDataOriginal.wallData[i].resize(m_mazeDataOriginal.width);
	}

	for (int i = 0; i < m_mazeDataOriginal.height; i++)
	{
		for (int j = 0; j < m_mazeDataOriginal.width; j++)
		{
			if (i % 32 == 0 || j % 32 == 0)
				m_mazeDataOriginal.wallData[i][j].wallType = eWallTypes_Uneditable;
			else
				m_mazeDataOriginal.wallData[i][j].wallType = eWallTypes_Solid;
		}
	}
}

void CMazeGenerator::InitMaze()
{
	InitEmptyMaze();
	DoMapGeneration();
	//SaveMap(m_sMapSeed + ".map");

	//LoadMap("Test.map");



	if (m_pMaze2DTexture)
	{
		IDeviceTexture* pDeviceTexture = m_pMaze2DTexture->GetDeviceTexture();
		if (!pDeviceTexture)
		{

			pDeviceTexture = GetAttributeManager()->GetEngine()->GetGraphicsDevice()->CreateDeviceTexture2D(m_mazeDataOriginal.width, m_mazeDataOriginal.height, ETF_R8G8B8A8);
			m_pMaze2DTexture->SetDeviceTexture(pDeviceTexture);
		}
		BYTE* pPixelData;
		unsigned int pitch;

		if (pDeviceTexture->Lock(&pPixelData, ELKT_WRITE_DISCARD, pitch))
		{
			for (int y = 0; y < m_mazeDataOriginal.height; y++)
			{
				for (int x = 0; x < m_mazeDataOriginal.width; x++)
				{
					int index = y * pitch + x * 4;
					if (m_mazeDataOriginal.wallData[y][x].wallType == eWallTypes_Solid)
					{
						pPixelData[index + 0] = 255;
						pPixelData[index + 1] = 255;
						pPixelData[index + 2] = 255;
						pPixelData[index + 3] = 255;
					}
					else if (m_mazeDataOriginal.wallData[y][x].wallType == eWallTypes_Uneditable)
					{
						pPixelData[index + 0] = 0;
						pPixelData[index + 1] = 0;
						pPixelData[index + 2] = 255;
						pPixelData[index + 3] = 255;
					}
					else if (m_mazeDataOriginal.wallData[y][x].wallType == eWallTypes_StairsUp)
					{
						pPixelData[index + 0] = 0;
						pPixelData[index + 1] = 255;
						pPixelData[index + 2] = 0;
						pPixelData[index + 3] = 255;
					}
					else if (m_mazeDataOriginal.wallData[y][x].wallType == eWallTypes_StairsDown)
					{
						pPixelData[index + 0] = 0;
						pPixelData[index + 1] = 0;
						pPixelData[index + 2] = 255;
						pPixelData[index + 3] = 255;
					}
					else
					{
						pPixelData[index + 0] = 0;
						pPixelData[index + 1] = 0;
						pPixelData[index + 2] = 0;
						pPixelData[index + 3] = 255;
					}
				}
			}
			pDeviceTexture->Unlock();
		}
	}
}

bool CMazeGenerator::IsAValidLocation(int cx, int cy)
{
	if (cx < 0 || cx >= m_mazeDataOriginal.width)
		return false;
	if (cy < 0 || cy >= m_mazeDataOriginal.height)
		return false;

	// check if the 3 out of the 4 adjacent walls are solid, by using the cx and cy values.
	// if they are solid, then we are in a dead end.
	int count = 0;
	if (cx > 0)
	{
		if (m_mazeDataOriginal.wallData[cy][cx - 1].wallType == eWallTypes_Solid)
			count++;
	}
	else {
		count++;
	}
	if (cx < m_mazeDataOriginal.width - 1)
	{
		if (m_mazeDataOriginal.wallData[cy][cx + 1].wallType == eWallTypes_Solid)
			count++;
	}
	else {
		count++;
	}
	if (cy > 0)
	{
		if (m_mazeDataOriginal.wallData[cy - 1][cx].wallType == eWallTypes_Solid)
			count++;
	}
	else {
		count++;
	}
	if (cy < m_mazeDataOriginal.height - 1)
	{
		if (m_mazeDataOriginal.wallData[cy + 1][cx].wallType == eWallTypes_Solid)
			count++;
	}
	else {
		count++;
	}
	int cntNo = 3;
	//ns::debug::write_line("%d %d - %d", cx, cy, count);
	if (GetRandomMinMax(1, 1000) < 10)
	{
		cntNo = 2;
	}
	if (count < cntNo)
		return false;


	// check if the current cell is a solid wall
	if (m_mazeDataOriginal.wallData[cy][cx].wallType != eWallTypes_Solid)
		return false;

	return true;
}

void CMazeGenerator::GeneratePathTree()
{
	std::reverse_iterator <std::vector<SMappedCell>::iterator> it = m_path.rbegin();
}

void CMazeGenerator::CreateMazeMesh()
{
	ISceneNode* Walls[4];
	ISceneNode* Pillars[4];
	ISceneNode* Floor;
	/*
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
	*/
	for (int y = 0; y < m_mazeDataOriginal.height; ++y)
	{

		for (int x = 0; x < m_mazeDataOriginal.width; ++x)
		{
			auto* pCurrent = &m_mazeDataOriginal.wallData[y][x];
			CInstancedMazeMesh* pMaze = dynamic_cast<CInstancedMazeMesh*>(m_pRootNode);
			if (pMaze)
			{
				if (pCurrent->wallType != eWallTypes_StairsDown && pCurrent->wallType != eWallTypes_StairsUp)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 0, eMazeMeshType_Floor);
					pMaze->AddNewMeshInstance((1 + x) * 5, 5, (1 + y) * 5, 0, eMazeMeshType_Floor);
				}
				/*pCurrent->pNode = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(m_pRootNode, false, false, "", false, false));
				pCurrent->pNode->SetPosition(VECTOR3(x * 5, 0.f, y * 5));
				pCurrent->pNode->SetParent((ISceneNode*)m_pRootNode);
				SAFE_ADDREF(pCurrent->pNode);
				pCurrent->pFloor = static_cast<ISceneNode*>(GetAttributeManager()->CloneAttributeObject(Floor, false, false, "", false, false));
				pCurrent->pFloor->SetParent(pCurrent->pNode);
				SAFE_ADDREF(pCurrent->pFloor);*/

				if (x == 0)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 3, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 3, eMazeMeshType_Pillar);

					//AddWall(3, pCurrent, Walls); AddPillar(3, pCurrent, Pillars);

				}

				if (x == m_mazeDataOriginal.width - 1)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 1, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 1, eMazeMeshType_Pillar);
					//AddWall(1, pCurrent, Walls); AddPillar(1, pCurrent, Pillars);
				}

				if (y == 0)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 2, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 2, eMazeMeshType_Pillar);
					//AddWall(2, pCurrent, Walls); AddPillar(2, pCurrent, Pillars);
				}

				if (y == m_mazeDataOriginal.height - 1)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 0, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 0, eMazeMeshType_Pillar);
					//AddWall(0, pCurrent, Walls); AddPillar(0, pCurrent, Pillars);
				}

				if (pCurrent->wallType == eMazeMeshType_Wall
					|| pCurrent->wallType == eWallTypes_Uneditable
					|| pCurrent->wallType == eWallTypes_Solid)
				{
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 0, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 1, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 2, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 3, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 0, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 1, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 2, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, 0, (1 + y) * 5, 3, eMazeMeshType_Pillar);

				}
				if (pCurrent->wallType == eWallTypes_StairsDown || pCurrent->wallType == eWallTypes_StairsUp)
				{
					// find out which direction the stairs are facing
					// check which square adjacent to the stairs is empty
					int dir = -1;

					if (m_mazeDataOriginal.wallData[y][x - 1].wallType == eWallTypes_None)
					{
						dir = 3;
					}
					else if (m_mazeDataOriginal.wallData[y][x + 1].wallType == eWallTypes_None)
					{
						dir = 1;
					}
					else if (m_mazeDataOriginal.wallData[y - 1][x].wallType == eWallTypes_None)
					{
						dir = 2;
					}
					else if (m_mazeDataOriginal.wallData[y + 1][x].wallType == eWallTypes_None)
					{
						dir = 0;
					}

					if (pCurrent->wallType == eWallTypes_StairsUp)
					{
						dir = (dir + 2) % 4;
					}
					pMaze->AddNewMeshInstance((1 + x) * 5, pCurrent->wallType == eWallTypes_StairsDown ? -4 : -0, (1 + y) * 5, dir, pCurrent->wallType);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 0, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 1, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 2, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 3, eMazeMeshType_Wall);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 0, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 1, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 2, eMazeMeshType_Pillar);
					pMaze->AddNewMeshInstance((1 + x) * 5, -4.5, (1 + y) * 5, 3, eMazeMeshType_Pillar);
				}
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

void CMazeGenerator::Tick(float fDeltaTime)
{
	IAttributeObject::Tick(fDeltaTime);
	((CInstancedMazeMesh*)m_pRootNode)->UpdateMeshInstanceVisibility(m_pPlayerData->x, m_pPlayerData->y, 10);
}

bool CMazeGenerator::FindPoint(int x1, int y1, int x2, int y2, int x, int y)
{
	if (x > x1 && x < x2 && y > y1 && y < y2)
		return true;

	return false;
}

bool CMazeGenerator::SaveMap(const std::string& fileName)
{
	bool res = false;
	nlohmann::json jsonMapData;
	jsonMapData = nlohmann::json();

	jsonMapData["width"] = m_mazeDataOriginal.width;
	jsonMapData["height"] = m_mazeDataOriginal.height;
	jsonMapData["startX"] = m_pPlayerData->x;
	jsonMapData["startY"] = m_pPlayerData->y;
	jsonMapData["seed"] = m_sMapSeed;
	jsonMapData["wallData"] = nlohmann::json::array();
	for (int y = 0; y < m_mazeDataOriginal.height; y++)
	{
		nlohmann::json jsonWallData;
		for (int x = 0; x < m_mazeDataOriginal.width; x++)
		{

			jsonWallData.push_back(m_mazeDataOriginal.wallData[y][x].wallType);


		}
		jsonMapData["wallData"].push_back(jsonWallData);
	}

	jsonMapData["pathData"] = nlohmann::json::array();
	for (auto path : m_path)
	{
		nlohmann::json jsonPathData;
		jsonPathData.push_back(path.x);
		jsonPathData.push_back(path.y);
		jsonMapData["pathData"].push_back(jsonPathData);
	}


	std::ofstream file(fileName);
	file << jsonMapData.dump(4);
	file.close();

	return res;
}

bool CMazeGenerator::LoadMap(const std::string& fileName)
{
	bool res = false;
	std::ifstream file(fileName);
	nlohmann::json jsonMapData;
	file >> jsonMapData;
	file.close();

	m_mazeDataOriginal.width = jsonMapData["width"];
	m_mazeDataOriginal.height = jsonMapData["height"];
	m_mazeDataOriginal.wallData.resize(m_mazeDataOriginal.height);

	for (int i = 0; i < m_mazeDataOriginal.height; i++)
	{
		m_mazeDataOriginal.wallData[i].resize(m_mazeDataOriginal.width);
	}
	//m_mazeData.wallData = new SCellData*[m_mazeData.height];
	for (int y = 0; y < m_mazeDataOriginal.height; y++)
	{
		//m_mazeData.wallData[y] = new SCellData[m_mazeData.width];
		for (int x = 0; x < m_mazeDataOriginal.width; x++)
		{
			m_mazeDataOriginal.wallData[y][x].wallType = jsonMapData["wallData"][y][x];
		}
	}

	return res;
}

CDungeonCrawlerGame::CDungeonCrawlerGame()
	:IAttributeObject()
	, m_pPlayerAvatar(nullptr)
	, m_pMazeGenerator(nullptr)
{
	SETCLASS("CDungeonCrawlerGame");
}

CDungeonCrawlerGame::~CDungeonCrawlerGame()
{

}

void CDungeonCrawlerGame::OnLostDevice(void)
{
	IAttributeObject::OnLostDevice();
}

void CDungeonCrawlerGame::OnResetDevice(IGraphicsDevice& device)
{
	IAttributeObject::OnResetDevice(device);
}
float deg_to_rad(float deg)
{
	return deg * (3.14159265358979323846 / 180);
}

VECTOR2 CDungeonCrawlerGame::getDestPos(int direction)
{
	//return VECTOR2(m_pPlayerData->x + sin(direction * 90), m_pPlayerData->y		+ cos(direction * 90));
	VECTOR2 vec = VECTOR2(
		sin(deg_to_rad(direction * 90)),
		cos(deg_to_rad(direction * 90)));

	vec.x = int(vec.x) + m_pPlayerData->x;
	vec.y = int(vec.y) + m_pPlayerData->y;
	
	if (vec.x < 0)
		vec.x = 0;
	if (vec.y < 0)
		vec.y = 0;

	if (vec.x >= m_pMazeGenerator->GetMazeData()->width)
		vec.x = m_pMazeGenerator->GetMazeData()->width - 1;

	if (vec.y >= m_pMazeGenerator->GetMazeData()->height)
		vec.y = m_pMazeGenerator->GetMazeData()->height - 1;

	return vec;
}


bool CDungeonCrawlerGame::canMove(VECTOR2 pos)
{
	if (m_pMazeGenerator->GetMazeData()->wallData[pos.y][pos.x].wallType == eWallTypes_None)
		return true;

	if (m_pMazeGenerator->GetMazeData()->wallData[pos.y][pos.x].wallType == eWallTypes_Room)
		return true;

	return false;
}

void CDungeonCrawlerGame::moveBackward(void)
{
	VECTOR2 destPos = getDestPos(invertDirection(m_pPlayerData->facing));
	if (canMove(destPos))
	{
		m_pPlayerData->x = destPos.x;
		m_pPlayerData->y = destPos.y;
		//m_bFlipped = !m_bFlipped;
	}
}

void CDungeonCrawlerGame::moveForward(void)
{
	VECTOR2 destPos = getDestPos(m_pPlayerData->facing);
	if (canMove(destPos))
	{
		m_pPlayerData->x = destPos.x;
		m_pPlayerData->y = destPos.y;
		//m_bFlipped = !m_bFlipped;
	}
}

void CDungeonCrawlerGame::strafeLeft(void)
{
	auto direction = m_pPlayerData->facing - 1;
	if (direction < 0)
		direction = 3;

	VECTOR2 destPos = getDestPos(direction);
	if (canMove(destPos))
	{
		m_pPlayerData->x = destPos.x;
		m_pPlayerData->y = destPos.y;
		//m_bFlipped = !m_bFlipped;
	}
}

void CDungeonCrawlerGame::strafeRight(void)
{
	auto direction = m_pPlayerData->facing + 1;
	if (direction > 3)
		direction = 0;

	VECTOR2 destPos = getDestPos(direction);
	if (canMove(destPos))
	{
		m_pPlayerData->x = destPos.x;
		m_pPlayerData->y = destPos.y;

		//m_bFlipped = !m_bFlipped;
	}
}

int CDungeonCrawlerGame::invertDirection(int direction)
{
	if (direction == 0) return 2;
	if (direction == 1) return 3;
	if (direction == 2) return 0;
	if (direction == 3) return 1;
	return 0;
}

void CDungeonCrawlerGame::turnLeft(void)
{
	m_pPlayerData->facing = m_pPlayerData->facing - 1;
	if (m_pPlayerData->facing < 0)
		m_pPlayerData->facing = 3;
}

void CDungeonCrawlerGame::turnRight(void)
{
	m_pPlayerData->facing = m_pPlayerData->facing + 1;
	if (m_pPlayerData->facing > 3)
		m_pPlayerData->facing = 0;
}

void CDungeonCrawlerGame::DeclareAttributes(IAttributeManager& atrMan)
{
	IAttributeObject::DeclareAttributes(atrMan);
	AddAttribute(new CAtrReference("Map Generator", this, (IIdentifiable**)&m_pMazeGenerator, atrMan.GetTypeVector("CMazeGenerator")));
	AddAttribute(new CAtrReference("Player Avatar", this, (IIdentifiable**)&m_pPlayerAvatar, atrMan.GetTypeVector("ISceneNode")));
}

void CDungeonCrawlerGame::InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent)
{
	IAttributeObject::InitialiseAttributeObject(atrMan, pParent);
	atrMan.AddTickableObject(this);
	atrMan.GetEngine()->GetEventManager()->AddEventHandler("CKeyboardEvent", this, OnKeyboardEvent);
}

void CDungeonCrawlerGame::PostLoad(void)
{
	IAttributeObject::PostLoad();

	if (m_pMazeGenerator)
		m_pPlayerData = m_pMazeGenerator->PlayerData();

}

void CDungeonCrawlerGame::UninitialiseAttributeObject(IAttributeManager& atrMan)
{
	IAttributeObject::UninitialiseAttributeObject(atrMan);
	atrMan.RemoveTickableObject(this);
	atrMan.GetEngine()->GetEventManager()->RemoveEventHandler("CKeyboardEvent", this);


}

void CDungeonCrawlerGame::Tick(float fDeltaTime)
{
	IAttributeObject::Tick(fDeltaTime);
	if (m_pPlayerAvatar)
	{
		m_pPlayerAvatar->SetPosition(m_pPlayerData->x * 5, 0, m_pPlayerData->y * 5);
		m_pPlayerAvatar->SetRotation(0.0, 90 * m_pPlayerData->facing, 0.f);
	}
}

void CDungeonCrawlerGame::OnKeyboardEvent(IReferenceCounted* pThis, IEvent* p)
{
	CDungeonCrawlerGame* pMe = (CDungeonCrawlerGame*)pThis;
	CKeyboardEvent* pEvent = (CKeyboardEvent*)p;

	pMe->HandleKeyboard(pEvent);

}

void CDungeonCrawlerGame::HandleKeyboard(CKeyboardEvent* pEvent)
{
	int iKey = pEvent->GetKeyCode();

	if (iKey != 0)
	{
		if (m_pPlayerData)
		{

			if (pEvent->GetKeyState())
			{
				// Key down	


				switch (iKey)
				{
				case 'w':
				case 'W':
					moveForward();
					break;
				case 's':
				case 'S':
					moveBackward();
					break;
				case 'a':
				case 'A':
					strafeLeft();
					break;
				case 'd':
				case 'D':
					strafeRight();
					break;
				case 'q':
				case 'Q':
					turnLeft();
					break;
				case 'e':
				case 'E':
					turnRight();
					break;
				default:
					break;
				}

			}
			else
			{
				// Key up

			}
		}
	}
}
