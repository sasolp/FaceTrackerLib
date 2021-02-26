#pragma once
#include "ueye.h"
#include <vector>

enum _disp_mode
{
	e_disp_mode_bitmap = 0,
	e_disp_mode_directdraw = 1
};

// memory needed for live display while using DIB
struct Memory
{
	Memory(void)
		: pcImageMemory(NULL)
		, lMemoryId(0)
		, lSequenceId(0)
	{
	}

	char* pcImageMemory;
	int lMemoryId;
	int lSequenceId;
};

/////////////////////////////////////////////////////////////////////////////
// CIdsSimpleLiveDlg dialog
class IDSCamera
{

public:
	IDSCamera();
	BOOL Init(int dev_id);
	void Start(int dev_id);
	void Stop();
	void Exit();
	int LoadParameter(int dev_id);
	int set_prop(int prop, double value);
	int load_config_file(wstring path);
	int GetNextImage(char**buf, int*id);
	// uEye varibles
	HIDS	m_hCam;				// handle to camera
	int		m_nColorMode;		// Y8/RGB16/RGB24/REG32
	int		m_nBitsPerPixel;	// number of bits needed store one pixel
	int		m_nSizeX;			// width of image
	int		m_nSizeY;			// height of image
	int		m_nPosX;			// left offset of image
	int		m_nPosY;			// right offset of image

	typedef std::vector<Memory> MemoryVector;
	MemoryVector m_vecMemory;
	const int m_cnNumberOfSeqMemory;
	SENSORINFO m_sInfo;			// sensor information struct
	int     m_nRenderMode;		// render  mode
	int     m_nFlipHor;			// horizontal flip flag
	int     m_nFlipVert;		// vertical flip flag

	int InitCamera(HIDS *hCam, HWND hWnd);
	bool OpenCamera(int dev_id);
	void ExitCamera();
	int  InitDisplayMode();
	void LoadParameters();
	void GetMaxImageSize(int *pnSizeX, int *pnSizeY);

	int AllocImageMems(int nSizeX, int nSizeY, int nBitsPerPixel);
	int FreeImageMems(void);
	int InitSequence(void);
	int ClearSequence(void);
	int GetLastMem(char** ppLastMem, int& lMemoryId, int& lSequenceId);
};