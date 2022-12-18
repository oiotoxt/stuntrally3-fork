#pragma once
#include "Gui_Def.h"
#include "GameState.h"
// #include <OgreFrameListener.h>
// #include <OgreWindowEventUtilities.h>
// #include <OgreMaterialManager.h>
#include <vector>

namespace ICS {  class InputControlSystem;  class DetectingBindingListener;  }
namespace MyGUI{  class Ogre2Platform;  }
namespace Ogre {  class SceneNode;  class Root;  class SceneManager;  class Window;  }
// class MasterClient;  class P2PGameClient;


//  main, race menus
enum Menu_Btns {  Menu_Race, Menu_Replays, Menu_Help, Menu_Options,  ciMainBtns };
enum Race_Btns {  Race_Single, Race_Tutorial, Race_Champ, Race_Challenge,
				  Race_Difficulty, Race_Simulation, Race_HowToPlay, Race_Back,  ciRaceBtns };
//  gui
enum TAB_Game    {  TAB_Back=0, TAB_Track,TAB_Car, TAB_Setup, TAB_Split,TAB_Multi, TAB_Champs,TAB_Stages,TAB_Stage  };
enum TAB_Options {  TABo_Back=0, TABo_Screen, TABo_Input, TABo_View, TABo_Graphics, TABo_Sound, TABo_Settings, TABo_Tweak  };
enum LobbyState  {  DISCONNECTED, HOSTING, JOINED  };



class BaseApp : public BGui, public GameState
	// public Ogre::FrameListener, public SFO::WindowListener
{
	friend class CarModel;
	friend class CGame;
	friend class CHud;
	friend class CGui;

public:
	BaseApp();
	virtual ~BaseApp();
	// virtual void Run(bool showDialog);
	
	bool bLoading =0, bLoadingEnd =0, bSimulating =0;  int iLoad1stFrames =0;
	
	//  🚗 Cars / vehicles
	//  is in BaseApp for camera mouse move
	typedef std::vector<class CarModel*> CarModels;
	CarModels carModels;
	
	void showMouse(), hideMouse(), updMouse();
	
	///  effects
	// void recreateCompositor();
	// bool AnyEffectEnabled();

	class SETTINGS* pSet =0;

	//  wnd, hud, upl
	bool bWindowResized =1, bSizeHUD =1, bRecreateHUD =0;
	// float roadUpdTm =0.f;
	// Ogre::SceneNode* ndSky =0;  //-
	

	// bool mShowDialog =0, mShutDown =0;
	// bool setup(), configure();  void updateStats();

	///  create
	// virtual void createScene() = 0;
	// virtual void destroyScene() = 0;

	// void createViewports(), refreshCompositor(bool disableAll=false);
	// void setupResources(), createResourceListener(), loadResources();
	void LoadingOn(), LoadingOff();

	///  frame events
	// bool frameRenderingQueued(const Ogre::FrameEvent& evt);
	// bool frameEnded(const Ogre::FrameEvent& evt);
	// virtual bool frameStart(Ogre::Real time) = 0;
	// virtual bool frameEnd(Ogre::Real time) = 0;
	// void onCursorChange(const std::string& name);


	///  Ogre
	Ogre::Root* mRoot =0;  Ogre::SceneManager* mSceneMgr =0;
	Ogre::Window* mWindow =0;
	// SDL_Window* mSDLWindow =0;

	// virtual void windowResized (int x, int y);
	// virtual void windowClosed();


	///  🕹️ Input
	//------------------------------------------------------------
	ICS::InputControlSystem* mInputCtrl =0;
	ICS::InputControlSystem* mInputCtrlPlayer[4] ={0,};
	// std::vector<SDL_Joystick*> mJoysticks;
	
	// this is set to true when the user is asked to assign a new key
	bool bAssignKey =0;
	ICS::DetectingBindingListener* mInputBindListner =0;

	///  input  vars
	bool alt =0, ctrl =0, shift =0;  // key modifiers  for FollowCamera move
	bool mbLeft =0, mbRight =0, mbMiddle =0;  // mouse buttons
	// int mMouseX =0, mMouseY =0;
	bool mbWireFrame =0;
	int iCurCam =0;
	void CreateInputs();

	///  input events  ----
	void mouseMoved( const SDL_Event &arg ) override;
	void mousePressed( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;
	void mouseReleased( const SDL_MouseButtonEvent &arg, Ogre::uint8 id ) override;

	// void textEditing( const SDL_TextEditingEvent& arg ) override;
	void textInput( const SDL_TextInputEvent& arg ) override;

	// virtual void keyPressed( const SDL_KeyboardEvent &arg ) override;  // in App
	// virtual void keyReleased(const SDL_KeyboardEvent &arg ) override;

	void joyButtonPressed( const SDL_JoyButtonEvent &evt, int button ) override;
	void joyButtonReleased( const SDL_JoyButtonEvent &evt, int button ) override;
	void joyAxisMoved( const SDL_JoyAxisEvent &arg, int axis ) override;
	// void joyPovMoved( const SDL_JoyHatEvent &arg, int index ) override;


	///  🎛️ Gui  ..........................
	bool isFocGui = 0, isFocRpl = 0;  // gui shown
	bool IsFocGuiInput()  {  return isFocGui || isFocRpl;  }
	bool IsFocGui();
	bool isTweak();
	Ogre::Window* getWindow()  {  return mWindow;  }
	
	MyGUI::Gui* mGui =0;
	MyGUI::Ogre2Platform* mPlatform =0;
	void baseInitGui(class GraphicsSystem *mGraphicsSystem), baseSizeGui();
	void DestroyGui();

	Img bckFps =0, imgBack =0;
	Txt txFps =0;

	//  ⏳ Loading  backgr, bar
	Img bckLoad =0, bckLoadBar =0, barLoad =0, imgLoad =0;
	Txt txLoadBig =0, txLoad =0;
	int barSizeX =0, barSizeY =0;
	void SetLoadingBar(float pecent);


	///  main menu  // pSet->inMenu
	WP mWndMainPanels[ciMainBtns] ={0,}, mWndRacePanels[ciRaceBtns] ={0,};
	Btn mWndMainBtns[ciMainBtns]  ={0,}, mWndRaceBtns[ciRaceBtns]   ={0,};

	Wnd mWndMain =0, mWndRace =0,
		mWndGame =0,mWndReplays =0,  mWndHelp =0, mWndOpts =0,  // menu, windows
		mWndWelcome =0, mWndHowTo =0, mWndRpl =0, mWndRplTxt =0,
		mWndNetEnd =0, mWndTweak =0, mWndTrkFilt =0,  // rpl controls, netw, tools
		mWndChampStage =0, mWndChampEnd =0,
		mWndChallStage =0, mWndChallEnd =0;
	Tab mWndTabsGame =0, mWndTabsRpl =0, mWndTabsHelp =0, mWndTabsOpts =0;  // main tabs on windows
	
	//MyGUI::VectorWidgetPtr
	std::vector<WP> vwGui;  // all widgets to destroy


	///  👥 multiplayer  networking
	// boost::scoped_ptr<MasterClient> mMasterClient;
	// boost::scoped_ptr<P2PGameClient> mClient;
	// LobbyState mLobbyState =DISCONNECTED;
};
