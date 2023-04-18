#pragma once

#include "IResource.h"
#include <thread>
#include <nlohmann/json_fwd.hpp>

class ISceneNode;

class CMazeGenerator : public IAttributeObject
{
	struct SCellData
	{
		ISceneNode* pWalls[4];
		ISceneNode* pPillars[4];
		ISceneNode* pFloor{};
		ISceneNode* pNode{};
		BYTE Walls;
	};
	struct SMazeData
	{
		int width{};
		int height{};
		std::vector<std::vector<SCellData>> wallData;

	};

public:
	CMazeGenerator();
	virtual ~CMazeGenerator();

protected:
	void OnLostDevice(void) override;
	void OnResetDevice(IGraphicsDevice& device) override;

protected:
	void DeclareAttributes(IAttributeManager& atrMan) override;
	void InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent) override;
	void PostLoad(void) override;

	void UninitialiseAttributeObject(IAttributeManager& atrMan) override;

	static void FunctionGenerateMap(CFunctionObject* p);

	void CreateMazeMesh();

	void AddPillar(int i, SCellData* pCurrent, ISceneNode** Pillars);

	void AddWall(int i, SCellData* pCurrent, ISceneNode** Walls);

protected:
private:

	nlohmann::json* m_mapDataFile;
	std::string m_sMazeDataFile;

	SMazeData m_mazeData;

	IAttributeObject* m_pCellTemplate;
	IAttributeObject* m_pRootNode;
};
