#include "pch.h"
#include "Def_Str.h"
#include "BaseApp.h"
#include "pathmanager.h"
#include "settings.h"

// #include "Localization.h"
// #include "SplitScreen.h"
// #include "Compositor.h"

#include "CarModel.h"
#include "FollowCamera.h"
#include "GraphicsSystem.h"

#include <MyGUI_Prerequest.h>
#include <MyGUI.h>
#include <MyGUI_Ogre2Platform.h>

#include <OgreLogManager.h>
#include <OgreTimer.h>
#include <OgreOverlayManager.h>
#include <OgreWindow.h>

// #include "PointerFix.h"
#include "ICSInputControlSystem.h"
using namespace Ogre;


//  Create
//-------------------------------------------------------------------------------------
void BaseApp::CreateInputs()
{
	SetupKeysForGUI();
	// mCursorManager = new SFO::SDLCursorManager();
	// onCursorChange(MyGUI::PointerManager::getInstance().getDefaultPointer());
	// mCursorManager->setEnabled(true);

	std::string file = PATHMANAGER::UserConfigDir() + "/input.xml";
	mInputCtrl = new ICS::InputControlSystem(file, true, mInputBindListner, NULL, 100);

	for (int j=0; j<SDL_NumJoysticks(); ++j)
		mInputCtrl->addJoystick(j);
	for (int i=0; i<4; ++i)
	{
		file = PATHMANAGER::UserConfigDir() + "/input_p" + toStr(i) + ".xml";
		mInputCtrlPlayer[i] = new ICS::InputControlSystem(file, true, mInputBindListner, NULL, 100);
		for (int j=0; j<SDL_NumJoysticks(); ++j)
			mInputCtrlPlayer[i]->addJoystick(j);
	}

	// bSizeHUD = true;
	// bWindowResized = true;
	// mSplitMgr->Align();

	// mRoot->addFrameListener(this);
}

//  Run
//-------------------------------------------------------------------------------------
/*
void BaseApp::Run(bool showDialog)
{
	mShowDialog = showDialog;
	if (!setup())
		return;
	
	if (!pSet->limit_fps)
		mRoot->startRendering();  // default
	else
	{	Ogre::Timer tim;
		while (1)
		{
			WindowEventUtilities::messagePump();
			long min_fps = 1000000.0 / std::max(10.f, pSet->limit_fps_val);
			if (tim.getMicroseconds() > min_fps)
			{
				tim.reset();
				if (!mRoot->renderOneFrame())
					break;
			}else
			if (pSet->limit_sleep >= 0)
				boost::this_thread::sleep(boost::posix_time::milliseconds(pSet->limit_sleep));
	}	}

	destroyScene();
}
*/

//  🌟 ctor
//-------------------------------------------------------------------------------------
BaseApp::BaseApp()
	// :mMasterClient(), mClient()
{
}

//  💥 dtor
//-------------------------------------------------------------------------------------
BaseApp::~BaseApp()
{
	//if (mSplitMgr)
		//refreshCompositor(false);

	// CompositorManager::getSingleton().removeAll();
	// delete mSplitMgr;
	
	DestroyGui();

	//  save inputs
	mInputCtrl->save(PATHMANAGER::UserConfigDir() + "/input.xml");
	delete mInputCtrl;
	for (int i=0; i<4; ++i)
	{
		mInputCtrlPlayer[i]->save(PATHMANAGER::UserConfigDir() + "/input_p" + toStr(i) + ".xml");
		delete mInputCtrlPlayer[i];
	}

/*	delete mCursorManager;

	#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		mRoot->unloadPlugin("RenderSystem_Direct3D9");
		//mRoot->unloadPlugin("RenderSystem_Direct3D11");
	#endif
	mRoot->unloadPlugin("RenderSystem_GL");
*/
	// OGRE_DELETE mRoot;

	// SDL_SetWindowFullscreen(mSDLWindow, 0);
	// SDL_DestroyWindow(mSDLWindow);
}

#if 0

//  config
//-------------------------------------------------------------------------------------
bool BaseApp::configure()
{
	RenderSystem* rs;
	if (rs = mRoot->getRenderSystemByName(pSet->rendersystem))
	{
		mRoot->setRenderSystem(rs);
	}else
	{	LogO("RenderSystem '" + pSet->rendersystem + "' is not available. Exiting.");
		return false;
	}
	if (pSet->rendersystem == "OpenGL Rendering Subsystem")
		mRoot->getRenderSystem()->setConfigOption("RTT Preferred Mode", pSet->buffer);

	mRoot->initialise(false);

	Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE;
	if (SDL_WasInit(flags) == 0)
	{
		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
		if (SDL_Init(flags) != 0)
			throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
	}

	//  Enable joystick events  // todo:
	SDL_JoystickEventState(SDL_ENABLE);
	//  Open all available joysticks.  TODO: open them when they are required
	for (int i=0; i<SDL_NumJoysticks(); ++i)
	{
		SDL_Joystick* js = SDL_JoystickOpen(i);
		if (js)
		{
			mJoysticks.push_back(js);
			const char* s = SDL_JoystickName(js);
			int axes = SDL_JoystickNumAxes(js);
			int btns = SDL_JoystickNumButtons(js);
			//SDL_JoystickNumBalls SDL_JoystickNumHats
			LogO(String("<Joystick> name: ")+s+"  axes: "+toStr(axes)+"  buttons: "+toStr(btns));
		}
	}
	SDL_StartTextInput();

	NameValuePairList params;
	params.insert(std::make_pair("title", "Stunt Rally"));
	params.insert(std::make_pair("FSAA", toStr(pSet->fsaa)));
	params.insert(std::make_pair("vsync", pSet->vsync ? "true" : "false"));

	int pos_x = SDL_WINDOWPOS_UNDEFINED,
		pos_y = SDL_WINDOWPOS_UNDEFINED;

#if 0  /// _tool_ rearrange window pos for local netw testing
	SDL_Rect screen;
	if (SDL_GetDisplayBounds(/*pSet.screen_id*/0, &screen) != 0)
		LogO("SDL_GetDisplayBounds errror");
		
	if (pSet->net_local_plr <= 0)
	{	pos_x = 0;  pos_y = 0;
	}else
	{	pos_x = screen.w - pSet->windowx;
		pos_y = screen.h - pSet->windowy;
	}
#endif
	/// \todo For multiple monitors, WINDOWPOS_UNDEFINED is not the best idea. Needs a setting which screen to launch on,
	/// then place the window on that screen (derive x&y pos from SDL_GetDisplayBounds)


	//  Create an application window with the following settings:
	mSDLWindow = SDL_CreateWindow(
		"Stunt Rally", pos_x, pos_y, pSet->windowx, pSet->windowy,
		SDL_WINDOW_SHOWN | (pSet->fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_RESIZABLE);

	SFO::SDLWindowHelper helper(mSDLWindow, pSet->windowx, pSet->windowy, "Stunt Rally", pSet->fullscreen, params);
	helper.setWindowIcon("stuntrally.png");
	mWindow = helper.getWindow();

	return true;
}


//  Setup
//-------------------------------------------------------------------------------------
bool BaseApp::setup()
{
	Ogre::Timer ti,ti2;
	LogO("*** start setup ***");
	
	if (pSet->rendersystem == "Default")
	{
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		pSet->rendersystem = "Direct3D9 Rendering Subsystem";
		#else
		pSet->rendersystem = "OpenGL Rendering Subsystem";
		#endif
	}

	#ifdef _DEBUG
	#define D_SUFFIX ""  // "_d"
	#else
	#define D_SUFFIX ""
	#endif

	//  when show ogre dialog is on, load both rendersystems so user can select
	if (pSet->ogre_dialog)
	{
		mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/RenderSystem_GL" + D_SUFFIX);
		#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/RenderSystem_Direct3D9" + D_SUFFIX);
		#endif
	}else{
		if (pSet->rendersystem == "OpenGL Rendering Subsystem")
			mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/RenderSystem_GL" + D_SUFFIX);
		else if (pSet->rendersystem == "Direct3D9 Rendering Subsystem")
			mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/RenderSystem_Direct3D9" + D_SUFFIX);
	}

	mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/Plugin_ParticleFX" + D_SUFFIX);
#if defined(OGRE_VERSION) && OGRE_VERSION >= 0x10B00
    //mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/Codec_STBI" + D_SUFFIX);  // only png
    mRoot->loadPlugin(PATHMANAGER::OgrePluginDir() + "/Codec_FreeImage" + D_SUFFIX);  // for jpg screenshots
#endif

	#ifdef _DEBUG
	LogManager::getSingleton().setMinLogLevel(LML_TRIVIAL);  // all
	#endif

	setupResources();

	if (!configure())
		return false;


	mSceneMgr = mRoot->createSceneManager("DefaultSceneManager");

	#if OGRE_VERSION >= MYGUI_DEFINE_VERSION(1, 9, 0) 
	OverlaySystem* pOverlaySystem = new OverlaySystem();
	mSceneMgr->addRenderQueueListener(pOverlaySystem);
	#endif

	mSplitMgr = new SplitScr(mSceneMgr, mWindow, pSet);

	createViewports();  // calls mSplitMgr->Align();

	TextureManager::getSingleton().setDefaultNumMipmaps(5);

		LogO(String(":::* Time setup vp: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();


	//  Gui
	baseInitGui();

		LogO(String(":::* Time setup gui: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();

	createResourceListener();


	LogO("*** createFrameListener ***");
	createFrameListener();

		LogO(String(":::* Time createFrameListener: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();

	LogO("*** createScene ***");
	createScene();

		LogO(String(":::* Time createScene: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();

	LogO("*** recreateCompositor, does nothing ***");
	recreateCompositor();

	LogO("*** end setup ***");


	///  material factory setup
	sh::OgrePlatform* platform = new sh::OgrePlatform("General", PATHMANAGER::Data() + "/materials");
	platform->setCacheFolder(PATHMANAGER::ShaderDir());

	mFactory = new sh::Factory(platform);

	postInit();

		LogO(String(":::* Time post, mat factory: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();

	loadResources();

		LogO(String(":::* Time resources: ") + fToStr(ti.getMilliseconds(),0,3) + " ms");  ti.reset();  

	LogO(String(":::* Time setup total: ") + fToStr(ti2.getMilliseconds(),0,3) + " ms");
	
	return true;
}

void BaseApp::destroyScene()
{
}

//  Resources
//-------------------------------------------------------------------------------------
void BaseApp::setupResources()
{
	// Load resource paths from config file
	ConfigFile cf;
	std::string s = PATHMANAGER::GameConfigDir() +
		(pSet->tex_size > 0 ? "/resources.cfg" : "/resources_s.cfg");
	cf.load(s);

	// Go through all sections & settings in the file
	ConfigFile::SectionIterator seci = cf.getSectionIterator();

	String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		ConfigFile::SettingsMultiMap *settings = seci.getNext();
		ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			ResourceGroupManager::getSingleton().addResourceLocation(
				PATHMANAGER::Data() + "/" + archName, typeName, secName);
	}	}
}

void BaseApp::createResourceListener()
{
}
void BaseApp::loadResources()
{
	LoadingOn();
	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	LoadingOff();
}
#endif


///  show / hide  Loading bar
//-------------------------------------------------------------------------------------
void BaseApp::LoadingOn()
{
	if (!imgLoad)  return;
	imgLoad->setVisible(true);
	imgBack->setVisible(true);
	bckLoad->setVisible(true);
#if 0
	mSplitMgr->SetBackground(ColourValue(0.15,0.165,0.18));
	mSplitMgr->mGuiViewport->setBackgroundColour(ColourValue(0.15,0.165,0.18,1.0));
	mSplitMgr->mGuiViewport->setClearEveryFrame(true);

	// Turn off  rendering except overlays
	mSceneMgr->clearSpecialCaseRenderQueues();
	mSceneMgr->addSpecialCaseRenderQueue(RENDER_QUEUE_OVERLAY);
	mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_INCLUDE);
#endif
}
void BaseApp::LoadingOff()
{
	if (!imgLoad)  return;
	imgLoad->setVisible(false);
	imgBack->setVisible(false);
	bckLoad->setVisible(false);
#if 0
	// Turn On  full rendering
	mSplitMgr->SetBackground(ColourValue(0.2,0.3,0.4));
	mSplitMgr->mGuiViewport->setBackgroundColour(ColourValue(0.2,0.3,0.4));
	mSceneMgr->clearSpecialCaseRenderQueues();
	mSceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_EXCLUDE);
#endif
}


///  base Init Gui
//--------------------------------------------------------------------------------------------------------------
void BaseApp::baseInitGui()
{
	mWindow = mGraphicsSystem->getRenderWindow();

	if (mPlatform)
		return;
	LogO("C::# Init MyGui");

	using namespace MyGUI;
	//  Gui
	mPlatform = new Ogre2Platform();
	mPlatform->initialise(
		mWindow, mGraphicsSystem->getSceneManager(),
		"Essential",
		PATHMANAGER::UserConfigDir() + "/MyGUI.log");

	// mGraphicsSystem->mWorkspace = setupCompositor();


	mGui = new Gui();
	mGui->initialise("core.xml");

	// MyGUI::LanguageManager::getInstance().setCurrentLanguage("en");

	// FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
	// MyGUI::ResourceManager::getInstance().load("core.xml");

	// PointerManager::getInstance().eventChangeMousePointer +=	MyGUI::newDelegate(this, &BaseApp::onCursorChange);
	// PointerManager::getInstance().setVisible(false);

		
	//------------------------ lang
	/*if (pSet->language == "")  // autodetect
	{	pSet->language = getSystemLanguage();
		setlocale(LC_NUMERIC, "C");  }*/
	
	if (!PATHMANAGER::FileExists(PATHMANAGER::Data() + "/gui/core_language_" + pSet->language + "_tag.xml"))
		pSet->language = "en";  // use en if not found
		
	MyGUI::LanguageManager::getInstance().setCurrentLanguage(pSet->language);
	//------------------------

		
	// mPlatform->getRenderManagerPtr()->setSceneManager(mSplitMgr->mGuiSceneMgr);
	// mPlatform->getRenderManagerPtr()->setActiveViewport(mSplitMgr->mNumViewports);


	///  create widgets
	//------------------------------------------------
	//  Fps
	bckFps = mGui->createWidget<ImageBox>("ImageBox",
		0,0, 234,78, Align::Default, "Main");
	bckFps->setImageTexture("back_fps.png");
	bckFps->setAlpha(0.9f);  bckFps->setVisible(false);

	txFps = mGui->createWidget<TextBox>("TextBox",
		6,3, 226,78, Align::Default, "ToolTip");
	txFps->setFontName("hud.fps");  //txFps->setAlpha(1.f);
	txFps->setTextShadow(true);  txFps->setTextShadowColour(Colour::Black);


	//  loading
	bckLoad = mGui->createWidget<ImageBox>("ImageBox",
		100,100, 500,110, Align::Default, "Pointer", "LoadBck");
	bckLoad->setImageTexture("loading_back.jpg");

	barSizeX = 480;
	bckLoadBar = bckLoad->createWidget<ImageBox>("ImageBox",
		10,43, 480,26, Align::Default, "LoadBckBar");
	bckLoadBar->setImageTexture("loading_bar2.jpg");
	bckLoadBar->setColour(Colour(0.5,0.5,0.5,1));

	barSizeY = 22;
	barLoad = bckLoadBar->createWidget<ImageBox>("ImageBox",
		0,2, 30,22, Align::Default, "LoadBar");
	barLoad->setImageTexture("loading_bar1.jpg");


	txLoadBig = bckLoad->createWidget<TextBox>("TextBox",
		10,8, 400,30, Align::Default, "LoadTbig");
	txLoadBig->setFontName("hud.text");
	txLoadBig->setTextColour(Colour(0.7,0.83,1));
	txLoadBig->setCaption(TR("#{LoadingDesc}"));

	txLoad = bckLoad->createWidget<TextBox>("TextBox",
		10,77, 400,24, Align::Default, "LoadT");
	txLoad->setFontName("hud.text");
	txLoad->setTextColour(Colour(0.65,0.78,1));
	txLoad->setCaption(TR("#{Initializing}..."));


	///  menu background image
	//  dont show for autoload and no loadingbackground
	if (!(!pSet->loadingbackground && pSet->autostart))
	{
		imgBack = mGui->createWidget<ImageBox>("ImageBox",
			0,0, 800,600, Align::Default, "Back","ImgBack");
		imgBack->setImageTexture("background2.jpg");
	}

	///  loading background img
	imgLoad = mGui->createWidget<ImageBox>("ImageBox",
		0,0, 800,600, Align::Default, "Back", "ImgLoad");
	imgLoad->setImageTexture("background2.png");
	imgLoad->setVisible(true);

	LogO("---- baseSizeGui");
	baseSizeGui();
}

void BaseApp::SetLoadingBar(float pecent)
{
	float p = pecent * 0.01f;
	int s = p * barSizeX; // w = p * 512.f;
	//barLoad->setImageCoord( IntCoord(512-w,0, w,64) );
	barLoad->setSize( s, barSizeY );
}


///  size gui (on resolution change)
//-------------------------------------------------------------------
void BaseApp::baseSizeGui()
{
	int sx = mWindow->getWidth(), sy = mWindow->getHeight() +30; //?
	bckLoad->setPosition(sx/2 - 250/*200*/, sy - 140);

	//imgBack->setCoord(0,0, sx, sy);
	//return;

	//  fit image to window, preserve aspect
	int ix = 1920, iy = 1200;  // get org img size ...
	int six, siy;  // sized to window
	int oix=0, oiy=0;  // offset pos
	float sa = float(sx)/sy, si = float(ix)/iy;  // aspects
	
	if (si >= sa)  // wider than screen
	{
		siy = sy;  six = si * siy;  // six/siy = si
		oix = (six - sx) / 2;
	}else
	{	six = sx;  siy = six / si;
		oiy = (siy - sy) / 2;
	}
	imgLoad->setCoord(-oix, -oiy, six, siy);
	if (imgBack)
	imgBack->setCoord(-oix, -oiy, six, siy);
}


void BaseApp::DestroyGui()
{
	LogO("D::# Destroy MyGui");
	if (mGui)
	{	mGui->shutdown();  delete mGui;  mGui = 0;  }
	if (mPlatform)
	{	mPlatform->shutdown();  delete mPlatform;  mPlatform = 0;  }
}
