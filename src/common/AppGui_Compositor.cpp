#include "pch.h"
#include "Def_Str.h"
#include "RenderConst.h"
#include "settings.h"
#include "App.h"
#include "Cam.h"
#include "GraphicsSystem.h"

#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreVector3.h>
#include <OgreWindow.h>
#include <OgreSceneManager.h>

#include <OgreTextureGpuManager.h>
// #include <OgrePixelFormatGpuUtils.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>

// #include <OgreAtmosphere2Npr.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecularDef.h>
using namespace Ogre;


//-----------------------------------------------------------------------------------------
//  🪄 Setup Compositor
//-----------------------------------------------------------------------------------------
CompositorWorkspace* AppGui::SetupCompositor()
{
	LogO("C### setup Compositor");
	// We first create the Cubemap workspace and pass it to the final workspace
	// that does the real rendering.
	//
	// If in your application you need to create a workspace but don't have a cubemap yet,
	// you can either programatically modify the workspace definition (which is cumbersome)
	// or just pass a PF_NULL texture that works as a dud and barely consumes any memory.
	// See Tutorial_Terrain for an example of PF_NULL dud.

	auto* rndSys = mRoot->getRenderSystem();
	auto* texMgr = rndSys->getTextureGpuManager();
	auto* mgr = mRoot->getCompositorManager2();
	
	//  💥 destroy old
	LogO("D### setup Compositor rem workspaces: "+ toStr(mWorkspaces.size()));
	for (auto ws : mWorkspaces)
	{
		// LogO("D### setup Compositor rem workspace");
		mgr->removeWorkspace( ws );
		// ws = 0;
	}
	mWorkspaces.clear();


	// 🔮 create Reflections  ----
	CreateCubeReflect();


	//  Render window external channels  ----
	CompositorChannelVec ext( 2 );
	ext[0] = mWindow->getTexture();
	ext[1] = mCubeReflTex;

	
#ifndef SR_EDITOR  // game
	// DestroyCameras();
	const int views = pSet->game.local_players;
	bool vr_mode = pSet->game.vr_mode;
#else
	const int views = 1;  // ed
	bool vr_mode = pSet->vr_mode;
#endif


	//  🎛️ Gui, add MyGUI pass  ----
	CompositorNodeDef* node = mgr->getNodeDefinitionNonConst("SR3_Render");
	CompositorTargetDef* target = node->getTargetPass(0);
	auto passes = target->getCompositorPasses();
	CompositorPassDef* pass = passes.back();
	if (pass->getType() != PASS_CUSTOM)
	{
		pass = target->addPass(PASS_CUSTOM, MyGUI::OgreCompositorPassProvider::mPassId);
	}
	if (!vr_mode)  // single gui, fullscreen
	{
		pass->mExecutionMask = 0x02;
		pass->mViewportModifierMask = 0x00;
	}

	DestroyCameras();


	//  Viewports
	//-----------------------------------------------------------------------------------------
	const IdString wsName( "SR3_Workspace" );

	if (vr_mode)
	{
		//  👀 VR mode  ---- ----
		//.................................................................................
		const Real eyeSide = 0.5f, eyeFocus = 0.45f, eyeZ = -10.f;  // dist

		auto camL = CreateCamera( "EyeL", 0,
			Vector3(-eyeSide, 0.f,0.f), Vector3(eyeFocus * -eyeSide, 0.f, eyeZ) );
		auto camR = CreateCamera( "EyeR", camL->nd,
			Vector3( eyeSide, 0.f,0.f), Vector3(eyeFocus *  eyeSide, 0.f, eyeZ) );
		mCamera = camL->cam;

		Vector4 dims = Vector4( 0.0f, 0.0f, 0.5f, 1.0f );  // offset, scale
		CompositorWorkspace* ws1,*ws2;
		ws1 = mgr->addWorkspace( mSceneMgr, ext,
				camL->cam, wsName,
				true, -1, 0, 0,
				dims, 0x01, 0x01 );

		dims = Vector4( 0.5f, 0.0f, 0.5f, 1.0f );
		ws2 = mgr->addWorkspace( mSceneMgr, ext, 
				camR->cam, wsName,
				true, -1, 0, 0,
				dims, 0x02, 0x02 );

		mWorkspaces.push_back(ws1);
		mWorkspaces.push_back(ws2);

		LogO("C### Created VR workspaces: "+toStr(mWorkspaces.size()));
		return ws1;
	}

#ifndef SR_EDITOR
	//  👥 splitscreen   ---- ---- ---- ----
	//.....................................................................................
	else if (views > 1)
	{
		for (int i = 0; i < 4; ++i)
			mDims[i].Default();

		for (int i = 0; i < views; ++i)
		{
			bool f1 = i > 0;
			auto c = CreateCamera( "Player" + toStr(i), 0, Vector3::ZERO, Vector3::NEGATIVE_UNIT_Z );
			if (i==0)
				mCamera = c->cam;

			//  set dimensions for the viewports
			float dims[4];  // left,top, width,height
			#define dim_(l,t,w,h)  {  dims[0]=l;  dims[1]=t;  dims[2]=w;  dims[3]=h;  }

			if (views == 1)
			{
				dim_(0.0, 0.0, 1.0, 1.0);
			}
			else if (views == 2)
			{
				if (!pSet->split_vertically)
				{	if (i == 0)	dim_(0.0, 0.0, 1.0, 0.5)
					else		dim_(0.0, 0.5, 1.0, 0.5)
				}else{
					if (i == 0) dim_(0.0, 0.0, 0.5, 1.0)
					else		dim_(0.5, 0.0, 0.5, 1.0)	}
			}
			else if (views == 3)
			{
				if (!pSet->split_vertically)
				{
					if (i == 0)			dim_(0.0, 0.0, 0.5, 0.5)
					else if (i == 1)	dim_(0.5, 0.0, 0.5, 0.5)
					else if (i == 2)	dim_(0.0, 0.5, 1.0, 0.5)
				}else{
					if (i == 0)			dim_(0.0, 0.0, 0.5, 1.0)
					else if (i == 1)	dim_(0.5, 0.0, 0.5, 0.5)
					else if (i == 2)	dim_(0.5, 0.5, 0.5, 0.5)
				}
			}
			else if (views == 4)
			{
				if (i == 0)			dim_(0.0, 0.0, 0.5, 0.5)
				else if (i == 1)	dim_(0.5, 0.0, 0.5, 0.5)
				else if (i == 2)	dim_(0.0, 0.5, 0.5, 0.5)
				else if (i == 3)	dim_(0.5, 0.5, 0.5, 0.5)
			}else
			{
				LogO("ERROR: Unsupported number of viewports: " + toStr(views));
				// return;
			}
			#undef dim_

			// save dims (for later use by Hud)
			// for (int d=0; d<4; ++d)
			{
				mDims[i].left = dims[0]*2-1;  mDims[i].top = dims[1]*2-1;
				mDims[i].width = dims[2]*2;  mDims[i].height = dims[3]*2;
				mDims[i].right = mDims[i].left + mDims[i].width;
				mDims[i].bottom = mDims[i].top + mDims[i].height;
				mDims[i].avgsize = (mDims[i].width + mDims[i].height) * 0.25f;
			}

			CompositorWorkspace* w =
				mgr->addWorkspace( mSceneMgr, ext,
					c->cam, wsName,
					true, -1, 0, 0,
					Vector4(dims[0], dims[1], dims[2], dims[3]),
					f1 ? 0x02 : 0x01, f1 ? 0x02 : 0x01 );

			mWorkspaces.push_back(w);
		}

		LogO("C### Created Split workspaces: "+toStr(mWorkspaces.size()));
		return mWorkspaces[0];
	}
#endif
	else  // 🖥️ single view
	//.....................................................................................
	{
		auto c = CreateCamera( "Player", 0, Vector3(0,50,100), Vector3(0,0,0) );
		mCamera = c->cam;

		auto ws = mgr->addWorkspace( mSceneMgr, ext, c->cam, wsName, true );  // in .compositor
		mWorkspaces.push_back(ws);
		LogO("C### Created Single workspaces: "+toStr(mWorkspaces.size()));
		return ws;
	}
}


//  💥🎥 destroy camera
void AppGui::DestroyCameras()
{
	LogO("D### destroy Cameras");
	// for (auto cam : mCameras)
		// mSceneMgr->destroyCamera(cam);
	// mSceneMgr->destroyAllCameras();
	mCams.clear();  // for game, not all
}

Cam* AppGui::findCam(String name)
{
	for (Cam& c : mCamsAll)
		if (c.name == name)
			return &c;
	return 0;
}

//  🆕🎥 create camera
Cam* AppGui::CreateCamera(String name,
	SceneNode* nd, Vector3 pos, Vector3 lookAt)
{
	Cam* fc = findCam(name);
	if (fc)
	{
		LogO("---: got cam: " + name);
		mCams.push_back(*fc);
		return fc;
	}
// #define USEnodes  //- crashes in ogre upd frustum later
#ifdef USEnodes
	if (!nd)
	{	nd = mSceneMgr->getRootSceneNode(SCENE_DYNAMIC)->createChildSceneNode(SCENE_DYNAMIC);
		nd->setName( name + "CamNd" );
		LogO("c--: new cam node: " + name + "CamNd" );
		nd->setPosition( 0, 50, 100 );  // whatever, upd later
	}
#endif
	Camera* cam = mSceneMgr->createCamera( name );
	LogO("c--: new cam: " + name);

	cam->setPosition( pos );
	cam->lookAt( lookAt );
	cam->setNearClipDistance( 0.1f );
	cam->setFarClipDistance( pSet->view_distance );
	cam->setAutoAspectRatio( true );
	cam->setLodBias( pSet->lod_bias );

#ifdef USEnodes
	cam->detachFromParent();
	nd->attachObject( cam );
#endif

	Cam c;
	c.cam = cam;  c.nd = nd;
	c.name = name;
	
	mCamsAll.push_back(c);
	mCams.push_back(c);
	return &mCams.back();
}


#if 0
	// Use one node to control both cameras
	mCamerasNode = mSceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )
						->createChildSceneNode( Ogre::SCENE_DYNAMIC );
	mCamerasNode->setName( "Cameras Node" );

	mCamerasNode->setPosition( 0, 5, 15 );

	mEyeCameras[0] = mSceneManager->createCamera( "Left Eye" );
	mEyeCameras[1] = mSceneManager->createCamera( "Right Eye" );

	const Ogre::Real eyeDistance = 0.5f;
	const Ogre::Real eyeFocusDistance = 0.45f;

	for( int i = 0; i < 2; ++i )
	{
		const Ogre::Vector3 camPos( eyeDistance * ( Ogre::Real( i ) * 2 - 1 ), 0, 0 );
		mEyeCameras[i]->setPosition( camPos );

		Ogre::Vector3 lookAt( eyeFocusDistance * ( Ogre::Real( i ) * 2 - 1 ), -5, -15 );
		// Ogre::Vector3 lookAt( 0, 0, 0 );

		// Look back along -Z
		mEyeCameras[i]->lookAt( lookAt );
		mEyeCameras[i]->setNearClipDistance( 0.2f );
		mEyeCameras[i]->setFarClipDistance( 1000.0f );
		mEyeCameras[i]->setAutoAspectRatio( true );

		// By default cameras are attached to the Root Scene Node.
		mEyeCameras[i]->detachFromParent();
		mCamerasNode->attachObject( mEyeCameras[i] );
	}

	mCamera = mEyeCameras[0];
#endif



//  🔮 create cube Reflections tex,workspace
void AppGui::CreateCubeReflect()
{
	auto* rndSys = mRoot->getRenderSystem();
	auto* texMgr = rndSys->getTextureGpuManager();
	auto* mgr = mRoot->getCompositorManager2();

	// mIblQuality = IblLow;  //par..
	// mIblQuality = IblMedium;

	// A RenderTarget created with AllowAutomipmaps means the compositor still needs to
	// explicitly generate the mipmaps by calling generate_mipmaps.

	uint32 iblSpecularFlag = 0;
	if( rndSys->getCapabilities()->hasCapability( RSC_COMPUTE_PROGRAM ) &&
		mIblQuality != MipmapsLowest )
	{
		iblSpecularFlag = TextureFlags::Uav | TextureFlags::Reinterpretable;
	}

	/*TextureGpu* reflTex = texMgr->findTextureNoThrow("DynamicCubemap");
	if (reflTex)
		texMgr->destroyTexture(reflTex);*/

	mCubeReflTex = texMgr->createOrRetrieveTexture( "DynamicCubemap",
		GpuPageOutStrategy::Discard,
		TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps | iblSpecularFlag,
		TextureTypes::TypeCube );
	mCubeReflTex->scheduleTransitionTo( GpuResidency::OnStorage );

	uint32 resolution = 512u;  //?
	if( mIblQuality == MipmapsLowest || mIblQuality == IblMedium)
		resolution = 1024u;
	else if( mIblQuality == IblLow )
		resolution = 256u;
	else
		resolution = 512u;
	
	mCubeReflTex->setResolution( resolution, resolution );

	int mips = PixelFormatGpuUtils::getMaxMipmapCount( resolution );
	auto curmips = mCubeReflTex->getNumMipmaps();
	
	// if (curmips != mips)
	// if( mIblQuality != MipmapsLowest )
		// mCubeReflTex->setNumMipmaps( mips );  // no change-
		mCubeReflTex->setNumMipmaps( mips - 4u );  //par Limit max mipmap to 16x16
	
	mCubeReflTex->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
	mCubeReflTex->scheduleTransitionTo( GpuResidency::Resident );


	HlmsManager *hlmsMgr = mRoot->getHlmsManager();
	assert( dynamic_cast<HlmsPbs *>( hlmsMgr->getHlms( HLMS_PBS ) ) );
	HlmsPbs *hlmsPbs = static_cast<HlmsPbs *>( hlmsMgr->getHlms( HLMS_PBS ) );
	// hlmsPbs->resetIblSpecMipmap( 0u );  // auto, no-
	hlmsPbs->resetIblSpecMipmap( mips - 4u );  //+

	//  Create camera used to render to cubemap reflections
	if (mCubeCamera)
	{	mSceneMgr->destroyCamera(mCubeCamera);  mCubeCamera = 0;  }
	if( !mCubeCamera )
	{
		mCubeCamera = mSceneMgr->createCamera( "CubemapCam", true, true );
		mCubeCamera->setFOVy( Degree(90) );  mCubeCamera->setAspectRatio( 1 );
		mCubeCamera->setFixedYawAxis( false );
		mCubeCamera->setPosition( 0, 1.0, 0 );  // upd in car
		mCubeCamera->setNearClipDistance( 0.1 );
		// mCubeCamera->setUseRenderingDistance(true);
		// mCubeCamera->_setRenderedRqs(0, 200);
		mCubeCamera->setVisibilityFlags( RV_MaskReflect );
		// mCubeCamera->setDefaultVisibilityFlags( RV_Sky);  //** set in cubemap_target
		//? mCubeCamera->setVrData(VrData
		// mCubeCamera->setFarClipDistance( 100 );  // todo: !
		mCubeCamera->setFarClipDistance( pSet->view_distance );
		// mCubeCamera->setShadowRenderingDistance( 300 );  // par?-
		// mCubeCamera->setCastShadows(true);
	}

	//  No need to tie RenderWindow's use of MSAA with cubemap's MSAA
	const IdString idCubeNode =
		//mWindow->getSampleDescription().isMultisample() ? "CubemapNodeMsaa" :
		"CubemapNode";  // never use MSAA for cubemap.
	{
		CompositorNodeDef *nodeDef = mgr->getNodeDefinitionNonConst( idCubeNode );
		const CompositorPassDefVec &passes =
			nodeDef->getTargetPass( nodeDef->getNumTargetPasses() - 1u )->getCompositorPasses();

		OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassIblSpecularDef *>( passes.back() ) );
		CompositorPassIblSpecularDef *iblSpecPassDef =
			static_cast<CompositorPassIblSpecularDef *>( passes.back() );
		iblSpecPassDef->mForceMipmapFallback = mIblQuality == MipmapsLowest;
		iblSpecPassDef->mSamplesPerIteration = mIblQuality == IblLow ? 32.0f : mIblQuality == IblMedium ? 16.f : 128.0f;
		iblSpecPassDef->mSamplesSingleIterationFallback = iblSpecPassDef->mSamplesPerIteration;
	}


	//  🔮 Cubemap's compositor channels  ----
	CompositorChannelVec cubeExt( 1 );
	cubeExt[0] = mCubeReflTex;

	const String name( "SR3_ReflCubemap" );  // created from code
	if( !mgr->hasWorkspaceDefinition( name ) )
	{
		LogO("REFL ws add");
		auto* w = mgr->addWorkspaceDefinition( name );
		w->connectExternal( 0, idCubeNode, 0 );
	}

	auto* mWorkspace = mgr->addWorkspace(
		mSceneMgr, cubeExt, mCubeCamera, name, true );
	mWorkspaces.push_back(mWorkspace);  //+
}
