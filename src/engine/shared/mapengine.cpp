/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/mapengine.h>
#include <engine/storage.h>
#include "datafile.h"

class CMapEngine : public IEngineMap
{
	CDataFileReader m_DataFile;
public:
	CMapEngine() {}

	virtual void *GetData(int Index) { return m_DataFile.GetData(Index); }
	virtual void *GetDataSwapped(int Index) { return m_DataFile.GetDataSwapped(Index); }
	virtual void UnloadData(int Index) { m_DataFile.UnloadData(Index); }
	virtual void *GetItem(int Index, int *pType, int *pID) { return m_DataFile.GetItem(Index, pType, pID); }
	virtual void GetType(int Type, int *pStart, int *pNum) { m_DataFile.GetType(Type, pStart, pNum); }
	virtual void *FindItem(int Type, int ID) { return m_DataFile.FindItem(Type, ID); }
	virtual int NumItems() { return m_DataFile.NumItems(); }

	virtual void Unload()
	{
		m_DataFile.Close();
	}

	virtual bool Load(const char *pMapName)
	{
		IStorage *pStorage = Kernel()->RequestInterface<IStorage>();
		if(!pStorage)
			return false;
		return m_DataFile.Open(pStorage, pMapName, IStorage::TYPE_ALL);
	}

	virtual bool IsLoaded()
	{
		return m_DataFile.IsOpen();
	}

	virtual unsigned Crc()
	{
		return m_DataFile.Crc();
	}
};

extern IEngineMap *CreateEngineMap() { return new CMapEngine; }

CEngineMap::CEngineMap()
{
	m_pDataFile = new CDataFileReader();
}

void *CEngineMap::GetData(int Index) { return m_pDataFile->GetData(Index); }
void *CEngineMap::GetDataSwapped(int Index) { return m_pDataFile->GetDataSwapped(Index); }
void CEngineMap::UnloadData(int Index) { m_pDataFile->UnloadData(Index); }
void *CEngineMap::GetItem(int Index, int *pType, int *pID) { return m_pDataFile->GetItem(Index, pType, pID); }
void CEngineMap::GetType(int Type, int *pStart, int *pNum) { m_pDataFile->GetType(Type, pStart, pNum); }
void *CEngineMap::FindItem(int Type, int ID) { return m_pDataFile->FindItem(Type, ID); }
int CEngineMap::NumItems() { return m_pDataFile->NumItems(); }

void CEngineMap::Unload()
{
	m_pDataFile->Close();
}

bool CEngineMap::Load(const char *pMapName, class IStorage *pStorage)
{
	return m_pDataFile->Open(pStorage, pMapName, IStorage::TYPE_ALL);
}

bool CEngineMap::IsLoaded()
{
	return m_pDataFile->IsOpen();
}

unsigned CEngineMap::Crc()
{
	return m_pDataFile->Crc();
}
