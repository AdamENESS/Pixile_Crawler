#pragma once

#include "IResource.h"
#include <thread>
#include <nlohmann/json_fwd.hpp>
class ITexture;
class ISceneNode;
class CKeyboardEvent;


struct SPlayerData {
	int x{};
	int y{};
	int facing{};
};
enum eWallTypes
{
	eWallTypes_None = 0,
	eWallTypes_Wall,
	eWallTypes_Pillar,
	eWallTypes_WallWithFrame,
	eWallTypes_DoorShut,
	eWallTypes_DoorOpen,
	eWallTypes_DoorLocked,
	eWallTypes_DoorBroken,
	eWallTypes_DoorWithFrame,
	eWallTypes_Uneditable,
	eWallTypes_StairsUp,
	eWallTypes_StairsDown,
	eWallTypes_Room,
	eWallTypes_Solid
};

struct SMappedCell
{
	int x{};
	int y{};
};

struct SStairLocation
{
	int ux{};
	int uy{};
	int dx{};
	int dy{};
};


struct SCellData
{
	ISceneNode* pWalls[4];
	ISceneNode* pPillars[4];
	ISceneNode* pFloor{};
	ISceneNode* pNode{};
	uint8_t wallType{};
	uint32_t index{ 0 };
};

struct SMazeData
{
	int width{};
	int height{};
	std::vector<std::vector<SCellData>> wallData;
	std::vector<std::vector<int>> mazePath;
};

class CMazeGenerator : public IAttributeObject
{


public:
	CMazeGenerator();
	virtual ~CMazeGenerator();

	bool SaveMap(const std::string& fileName);
	bool LoadMap(const std::string& fileName);

	SMazeData* GetMazeData() { return &m_mazeDataOriginal; }
	SPlayerData* PlayerData() const { return m_pPlayerData; }
	void PlayerData(SPlayerData* val) { m_pPlayerData = val; }
	//SMazeData& GetMazeData() const { return m_mazeData; }
protected:
	void OnLostDevice(void) override;
	void OnResetDevice(IGraphicsDevice& device) override;

protected:
	void DeclareAttributes(IAttributeManager& atrMan) override;
	void InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent) override;
	void PostLoad(void) override;

	void UninitialiseAttributeObject(IAttributeManager& atrMan) override;

	static void FunctionGenerateMap(CFunctionObject* p);
	void InitEmptyMaze();


	bool IsAValidLocation(int cx, int cy);
	void GeneratePathTree();

	void CreateZones(int count, int cx, int cy, int walltype, int sizemin, int sizemax);
	int GetRandomMinMax(int min, int max);

	void DoMapGeneration();
	void CreateMazeMesh();
	void InitMaze();

	void AddPillar(int i, SCellData* pCurrent, ISceneNode** Pillars);

	void AddWall(int i, SCellData* pCurrent, ISceneNode** Walls);

	void Tick(float fDeltaTime) override;

protected:

	bool FindPoint(int x1, int y1, int x2,
		int y2, int x, int y);

private:

	nlohmann::json* m_mapDataFile;
	std::string m_sMazeDataFile;

	SMazeData m_mazeDataOriginal;
	SMazeData m_mazeDataModified;

	IAttributeObject* m_pCellTemplate;
	IAttributeObject* m_pRootNode;
	SPlayerData* m_pPlayerData;

	ITexture* m_pMaze2DTexture;

	std::string m_sMapSeed;
	std::vector<SStairLocation> m_stairLocations;
	std::vector<SMappedCell> m_path;

};



class CDungeonCrawlerGame : public IAttributeObject
{
public:
	CDungeonCrawlerGame();
	virtual ~CDungeonCrawlerGame();

protected:
	void OnLostDevice(void) override;
	void OnResetDevice(IGraphicsDevice& device) override;
	VECTOR2 getDestPos(int direction);
	bool canMove(VECTOR2 pos);

	void moveBackward(void);
	void moveForward(void);

	void strafeLeft(void);

	void strafeRight(void);

	int invertDirection(int direction);

	void turnLeft(void);
	void turnRight(void);

	void DeclareAttributes(IAttributeManager& atrMan) override;
	void InitialiseAttributeObject(IAttributeManager& atrMan, IIdentifiable* pParent) override;
	void PostLoad(void) override;

	void UninitialiseAttributeObject(IAttributeManager& atrMan) override;

	void Tick(float fDeltaTime) override;
	static void OnKeyboardEvent(IReferenceCounted* pThis, IEvent* p);

	void HandleKeyboard(CKeyboardEvent* pEvent);


private:
	CMazeGenerator* m_pMazeGenerator{ nullptr };
	SPlayerData* m_pPlayerData;
	ISceneNode* m_pPlayerAvatar;


};
