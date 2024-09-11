#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>
#include <vector>
#include "Entity.h"
#include <condition_variable>
#include <mutex>

struct struct_4 {
    int field0_0x0;
    int field1_0x4;
    float x;
    // 34 is height of one line
    float y;
    float field4_0x10;
    float field5_0x14;
    int field6_0x18;
    int field7_0x1c;
    int field8_0x20;
    unsigned int field9_0x24;  // flags. 0x2 - right-align.
	/* The string.
	^mNull; - empty icon
	^mW; - W keyboard button icon
	^mAtk6;^mAtkH;^mFish;
	^mChar%s;
	(probably KYK, MAY, MLL, ZAT, POT, CHP, FAU, AXL, VEN, SLY, INO, BED, RAM, SIN, ELP, LEO, JHN, JKO, KUM, RVN, DZY, BKN, ANS, APRL, ARLS, GBRL, PHLX, PRDM, RBKY, VRNN, ZPPA, DARL)
	^g%04d; // guild icon
	*/
    char * field10_0x28;
    int field11_0x2c;
    int field12_0x30;
    int field13_0x34;
    float field14_0x38;
    float field15_0x3c;
    char field16_0x40;
    char field17_0x41;
    char field18_0x42;
    char field19_0x43;
    char field20_0x44;
    char field21_0x45;
    char field22_0x46;
    char field23_0x47;
    char field24_0x48;
    char field25_0x49;
    char field26_0x4a;
    char field27_0x4b;
    char field28_0x4c;
    char field29_0x4d;
    char field30_0x4e;
    char field31_0x4f;
    char field32_0x50;
    char field33_0x51;
    char field34_0x52;
    char field35_0x53;
    char field36_0x54;
    char field37_0x55;
    char field38_0x56;
    char field39_0x57;
    char field40_0x58;
    char field41_0x59;
    char field42_0x5a;
    char field43_0x5b;
    char field44_0x5c;
    char field45_0x5d;
    char field46_0x5e;
    char field47_0x5f;
    char field48_0x60;
    char field49_0x61;
    char field50_0x62;
    char field51_0x63;
    char field52_0x64;
    char field53_0x65;
    char field54_0x66;
    char field55_0x67;
    char field56_0x68;
    char field57_0x69;
    char field58_0x6a;
    char field59_0x6b;
    char field60_0x6c;
    char field61_0x6d;
    char field62_0x6e;
    char field63_0x6f;
    char field64_0x70;
    char field65_0x71;
    char field66_0x72;
    char field67_0x73;
    char field68_0x74;
    char field69_0x75;
    char field70_0x76;
    char field71_0x77;
    char field72_0x78;
    char field73_0x79;
    char field74_0x7a;
    char field75_0x7b;
    char field76_0x7c;
    char field77_0x7d;
    char field78_0x7e;
    char field79_0x7f;
    char field80_0x80;
    char field81_0x81;
    char field82_0x82;
    char field83_0x83;
    char field84_0x84;
    char field85_0x85;
    char field86_0x86;
    char field87_0x87;
    char field88_0x88;
    char field89_0x89;
    char field90_0x8a;
    char field91_0x8b;
    char field92_0x8c;
    char field93_0x8d;
    char field94_0x8e;
    char field95_0x8f;
    char field96_0x90;
    char field97_0x91;
    char field98_0x92;
    char field99_0x93;
    char field100_0x94;
    char field101_0x95;
    char field102_0x96;
    char field103_0x97;
    char field104_0x98;
    char field105_0x99;
    char field106_0x9a;
    char field107_0x9b;
    char field108_0x9c;
    char field109_0x9d;
    char field110_0x9e;
    char field111_0x9f;
    char field112_0xa0;
    char field113_0xa1;
    char field114_0xa2;
    char field115_0xa3;
    char field116_0xa4;
    char field117_0xa5;
    char field118_0xa6;
    char field119_0xa7;
    char field120_0xa8;
    char field121_0xa9;
    char field122_0xaa;
    char field123_0xab;
    char field124_0xac;
    char field125_0xad;
    char field126_0xae;
    char field127_0xaf;
    char field128_0xb0;
    char field129_0xb1;
    char field130_0xb2;
    char field131_0xb3;
    char field132_0xb4;
    char field133_0xb5;
    char field134_0xb6;
    char field135_0xb7;
    char field136_0xb8;
    char field137_0xb9;
    char field138_0xba;
    char field139_0xbb;
    char field140_0xbc;
    char field141_0xbd;
    char field142_0xbe;
    char field143_0xbf;
    int field144_0xc0;
    char field145_0xc4;
    char field146_0xc5;
    char field147_0xc6;
    char field148_0xc7;
    int field149_0xc8;
    int field150_0xcc;
    unsigned long long field151_0xd0;
    unsigned long long field152_0xd8;
    unsigned long long field153_0xe0;
    unsigned long long field154_0xe8;
    int field155_0xf0;
    // flag 0x20 means single-byte string
    int field156_0xf4;
    int field157_0xf8;
    int field158_0xfc;
    float field159_0x100;
    float field160_0x104;
    int field161_0x108;
    int field162_0x10c;
    int field163_0x110;
    int field164_0x114;
    int field165_0x118;
    int field166_0x11c;
    unsigned int field167_0x120;
    unsigned int flags2;
    char field169_0x128;
    char field170_0x129;
    char field171_0x12a;
    char field172_0x12b;
    char field173_0x12c;
    char field174_0x12d;
    char field175_0x12e;
    char field176_0x12f;
    char field177_0x130;
    char field178_0x131;
    char field179_0x132;
    char field180_0x133;
    char field181_0x134;
    char field182_0x135;
    char field183_0x136;
    char field184_0x137;
    char field185_0x138;
    char field186_0x139;
    char field187_0x13a;
    char field188_0x13b;
    char field189_0x13c;
    char field190_0x13d;
    char field191_0x13e;
    char field192_0x13f;
    char field193_0x140;
    char field194_0x141;
    char field195_0x142;
    char field196_0x143;
    char field197_0x144;
    char field198_0x145;
    char field199_0x146;
    char field200_0x147;
    char field201_0x148;
    char field202_0x149;
    char field203_0x14a;
    char field204_0x14b;
    char field205_0x14c;
    char field206_0x14d;
    char field207_0x14e;
    char field208_0x14f;
    char field209_0x150;
    char field210_0x151;
    char field211_0x152;
    char field212_0x153;
    char field212_0x154;
    char field212_0x155;
    char field212_0x156;
    char field212_0x157;
    char field212_0x158;
    char field212_0x159;
    char field212_0x15a;
    char field212_0x15b;
    char field212_0x15c;
    char field212_0x15d;
    char field212_0x15e;
    char field212_0x15f;
    struct_4() {
		this->x = 640.0;
		this->y = 360.0;
		this->field4_0x10 = 200.0;
		this->field5_0x14 = 300.0;
		this->field6_0x18 = 0;
		this->field14_0x38 = 1.0;
		this->field15_0x3c = 1.0;
		this->field155_0xf0 = 0;
		this->field0_0x0 = 0;
		this->field8_0x20 = 0;
		this->field10_0x28 = (char *)0x0;
		this->field12_0x30 = 0;
		this->field13_0x34 = 0;
		this->field144_0xc0 = 0;
		this->field149_0xc8 = 0;
		this->field150_0xcc = 0;
		this->field1_0x4 = 1;
		this->field7_0x1c = -1;
		this->field9_0x24 = 1;
		this->field11_0x2c = 0xf7;
		this->field151_0xd0 = 0;
		this->field152_0xd8 = 0;
		this->field153_0xe0 = 0;
		this->field154_0xe8 = 0;
		memset(&this->field156_0xf4,0,0x60);
    }
};

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
using SendUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using ReadUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using drawTextWithIcons_t = void(*)(struct_4* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device);
HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	HRESULT presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	void endSceneHook(IDirect3DDevice9* device);
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void setPresentFlag();
	bool consumePresentFlag();
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void logic();
	void assignNextId(bool acquireLock = false);
	void imguiHandleResetBefore();
	void imguiHandleResetAfter();
	EndScene_t orig_EndScene = nullptr;
	std::mutex orig_EndSceneMutex;
	Present_t orig_Present = nullptr;
	std::mutex orig_PresentMutex;
	SendUnrealPawnData_t orig_SendUnrealPawnData = nullptr;
	std::mutex orig_SendUnrealPawnDataMutex;
	bool orig_SendUnrealPawnDataMutexLocked = false;
	DWORD orig_SendUnrealPawnDataMutexThreadId = NULL;
	ReadUnrealPawnData_t orig_ReadUnrealPawnData = nullptr;
	std::mutex orig_ReadUnrealPawnDataMutex;
	WNDPROC orig_WndProc = nullptr;
	std::mutex orig_WndProcMutex;
	bool orig_WndProcMutexLockedByWndProc = false;
	DWORD wndProcThread = 0;
	bool butDontPrepareBoxData = false;
private:
	class HookHelp {
		friend class EndScene;
		void sendUnrealPawnDataHook();
		void readUnrealPawnDataHook();
	};
	void sendUnrealPawnDataHook(char* thisArg);
	void readUnrealPawnDataHook(char* thisArg);
	void prepareDrawData(bool* needClearHitDetection);
	struct HiddenEntity {
		Entity ent{ nullptr };
		int scaleX = 0;
		int scaleY = 0;
		int scaleZ = 0;
		int scaleDefault = 0;
		bool wasFoundOnThisFrame = false;
	};
	bool isEntityAlreadyDrawn(const Entity& ent) const;
	void noGravGifMode();
	std::vector<HiddenEntity>::iterator findHiddenEntity(const Entity& ent);
	void initializeImgui(IDirect3DDevice9* device);
	void destroyImgui();
	void imguiEndScene(IDirect3DDevice9* device);
	bool needToTakeScreenshot = false;

	// The EndScene function is actually being called twice: once by GuiltyGear and one more time by the Steam overlay.
	// However, Present is only called once each frame. So we use the Present function to determine if the next EndScene
	// call should draw the boxes.
	bool presentCalled = true;

	std::vector<Entity> drawnEntities;

	std::vector<HiddenEntity> hiddenEntities;
	unsigned int allowNextFrameBeenHeldFor = 0;
	unsigned int allowNextFrameCounter = 0;
	bool freezeGame = false;
	bool continuousScreenshotMode = false;
	bool needContinuouslyTakeScreens = false;
	unsigned int p1PreviousTimeOfTakingScreen = ~0;
	unsigned int p2PreviousTimeOfTakingScreen = ~0;
	
	drawTextWithIcons_t drawTextWithIcons = nullptr;
	
	bool imguiInitialized = false;
	bool checkbox1 = false;
	bool checkbox2 = false;
};

extern EndScene endScene;
