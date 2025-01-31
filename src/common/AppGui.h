#pragma once
#include "Gui_Def.h"
#include "Cam.h"
#include "ViewDim.h"
#include "par.h"
#include <string>
#include <OgreString.h>
#include <OgreHlmsCommon.h>

#include <SDL_keycode.h>
#include <MyGUI_KeyCode.h>
#include <MyGUI_MouseButton.h>

namespace MyGUI{  class Gui;  class Ogre2Platform;  }
namespace Ogre {  class Root;  class SceneManager;  class Window;  class Camera;
	class TextureGpu;  class CompositorWorkspace;  class HlmsSamplerblock;
	namespace v1 {  class Overlay;  }
	class Terra;  class HlmsPbsTerraShadows;
	class PlanarReflections;  }
class GraphicsSystem;  class SETTINGS;  class CScene;  class CGui;  class CGuiCom;
class HlmsPbsDb2;

//  cube reflections, car etc
enum IblQuality
{
	MipmapsLowest, IblLow, IblMedium, IblHigh
};


//--------------------------------------------------------------------------------
//  Base application with:
//    Ogre, MyGui, settings, Fps bar, utils
//    Shadows, Reflection cube
//--------------------------------------------------------------------------------
class AppGui : public BGui
{
public:
	//  .cfg set  ----------------
	SETTINGS* pSet =0;
	void LoadDefaultSet(SETTINGS* settings, std::string setFile);
	void LoadSettings();


	//  🟢 Ogre  ----------------
	Ogre::Root* mRoot =0;
	Ogre::SceneManager* mSceneMgr =0;
	Ogre::Window* mWindow =0;
	Ogre::Camera* mCamera =0;

	GraphicsSystem* mGraphicsSystem =0;

	//  🏞️ Scene
	CScene* scn =0;


	//  🎛️ Gui  ----------------
	MyGUI::Gui* mGui =0;
	MyGUI::Ogre2Platform* mPlatform =0;

	CGui* gui =0;
	CGuiCom* gcom =0;

	//  Gui input utils  ----
	MyGUI::MouseButton sdlButtonToMyGUI(int button);
	std::vector<unsigned long> utf8ToUnicode(const std::string& utf8);

	MyGUI::KeyCode SDL2toGUIKey(SDL_Keycode code);
	void SetupKeysForGUI();
	std::unordered_map<SDL_Keycode, MyGUI::KeyCode> mKeyMap;


	//  📈 Fps bar  ----------------
	Img bckFps =0;
	Txt txFps =0, txFpsInfo =0;
	void UpdFpsText(float dt), CreateFpsBar();
	float timFps = 0.f;
	float GetGPUmem();


	//  ⛓️ Utils  ----------------
	//  🌐 Wireframe
	bool bWireframe = 0;
	void SetWireframe();
	void SetWireframe(Ogre::HlmsTypes type, bool wire);
	//  Texture filtering, wrap
	void SetTexWrap(Ogre::HlmsTypes type, Ogre::String name, bool wrap = true);
	void SetTexWrap(Ogre::Item* it, bool wrap = true);
	void InitTexFiltUV(Ogre::HlmsSamplerblock* sb, bool wrap = true);
	void InitTexFilt(Ogre::HlmsSamplerblock* sb);
	void SetAnisotropy();
	void SetAnisotropy(Ogre::Item* it);
	//  select
	void UpdSelectGlow(Ogre::Renderable *rend, bool selected);
	void BarrierResolve(Ogre::TextureGpu* tex);  // fix rtt for vulkan

	//  unload tex, gpu mem  ----
	template <typename T, size_t MaxNumTextures>
	void unloadTexturesFromUnusedMaterials(
		Ogre::HlmsDatablock* datablock,
		std::set<Ogre::TextureGpu*> &usedTex,
		std::set<Ogre::TextureGpu*> &unusedTex );
	void unloadTexturesFromUnusedMaterials();
	void unloadUnusedTextures();

	void MinimizeMemory();  // mtr,tex, reduce mem, each track load
	// void setTightMemoryBudget(), setRelaxedMemoryBudget();
	std::map<std::string, int> mapCommonTex;  // preload and don't unload
	void InitCommonTex(), LoadCommonTex();
	void LoadTex(Ogre::String fname, bool wait=true);


	//  🆕 Init  ----------------
	std::string getSystemLanguage();
	void InitAppGui();
	void DestroyGui();
	void Quit();


	//  🌒 Shadows ----------------
	Ogre::v1::Overlay *mDebugOverlayPSSM =0;
	Ogre::v1::Overlay *mDebugOverlaySpotlights =0;

	void createPcfShadowNode();
	void createEsmShadowNodes();
	void setupESM();
	Ogre::CompositorWorkspace *setupShadowCompositor();
	void updShadowFilter();

	const char *chooseEsmShadowNode();  // fixme ESM..
	// void setupShadowNode( bool forEsm );

	// void createShadowMapDebugOverlays();
	// void destroyShadowMapDebugOverlays();


	//  🔮 Reflection Cubemap  ----------------
	Ogre::Camera* mCubeCamera = 0;
	Ogre::TextureGpu* mCubeReflTex = 0;
	Ogre::CompositorWorkspace* wsCubeRefl = 0;
	int iReflSkip =0, iReflStart =0;
	void CreateCubeReflect(), UpdCubeRefl();

	void ApplyReflect();  // to all Db2s, after CubeRefl created
	std::set<HlmsPbsDb2*> vDbRefl;

	void AddGuiShadows(bool vr_mode = false, int plr = 0, bool gui = true);
	Ogre::CompositorWorkspace* SetupCompositor();

	//  util get SR3_ name for player
	Ogre::String getRender(int plr=0), getWorkspace(int plr=0);


	//  👥 Split screen  ----------------
	std::vector<Cam> mCamsAll, mCams;  // for each player
	Cam* findCam(Ogre::String name);

	Cam* CreateCamera(Ogre::String name,
		Ogre::SceneNode* node,  // creates for 0, attaches if not 0
		Ogre::Vector3 pos, Ogre::Vector3 lookAt);
	void DestroyCameras();
	
	//  viewport dimensions, for each player
	ViewDim mDims[MAX_Players];

	//  workspace, Ogre render setup   players + 1 for Refl cube, more for fluids etc
	std::vector<Ogre::CompositorWorkspace*> mWorkspaces;


	//  ⛰️ Terrain  ----------------
	// Ogre::String mtrName;
	// Ogre::SceneNode* nodeTerrain = 0;
};
