/*
-----------------------------------------------------------------------------
This source file is part of OGRE
	(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "pch.h"
#include "GraphicsSystem.h"
#include "Game.h"

#include "OgreRoot.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreConfigFile.h"
#include "OgreWindow.h"

#include "Compositor/OgreCompositorWorkspace.h"
#include "OgreArchiveManager.h"
#include "OgreHlmsManager.h"
#include "Terra/Hlms/OgreHlmsTerra.h"
#include "Terra/TerraWorkspaceListener.h"

//Declares WinMain / main
#include "MainEntryPointHelper.h"
#include "System/Android/AndroidSystems.h"
#include "System/MainEntryPoints.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
	#include "OSX/macUtils.h"
	#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
		#include "System/iOS/iOSUtils.h"
	#else
		#include "System/OSX/OSXUtils.h"
	#endif
#endif


class Tutorial_TerrainGraphicsSystem final : public GraphicsSystem
{
	Ogre::TerraWorkspaceListener *mTerraWorkspaceListener;

	void stopCompositor() override
	{
		if( mWorkspace )
			mWorkspace->removeListener( mTerraWorkspaceListener );
		delete mTerraWorkspaceListener;
		mTerraWorkspaceListener = 0;
	}

	Ogre::CompositorWorkspace *setupCompositor() override
	{
		/*using namespace Ogre;
		CompositorManager2 *compositorManager = mRoot->getCompositorManager2();

		//+ pbs
		CompositorWorkspace *workspace = compositorManager->addWorkspace(
			mSceneManager, mRenderWindow->getTexture(), mCamera,
			// "PbsMaterialsWorkspace", true );
			"Tutorial_TerrainWorkspace", true );

		if( !mTerraWorkspaceListener )
		{
			HlmsManager *hlmsManager = mRoot->getHlmsManager();
			Hlms *hlms = hlmsManager->getHlms( HLMS_USER3 );
			OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( hlms ) );
			mTerraWorkspaceListener = new TerraWorkspaceListener( static_cast<HlmsTerra *>( hlms ) );
		}
		workspace->addListener( mTerraWorkspaceListener );

		return workspace;*/
		return 0;
	}

	void setupResources() override
	{
		GraphicsSystem::setupResources();

		Ogre::ConfigFile cf;
		cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

		Ogre::String originalDataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );

		if( originalDataFolder.empty() )
			originalDataFolder = AndroidSystems::isAndroid() ? "/" : "./";
		else if( *(originalDataFolder.end() - 1) != '/' )
			originalDataFolder += "/";

		const int count = 5;
		const char *c_locations[count] =
		{
			"materials/Terrain",
			"materials/Terrain/GLSL",
			"materials/Terrain/HLSL",
			"materials/Terrain/Metal",
			// "materials/Postprocessing/SceneAssets",
			"materials/Pbs"
		};

		for( size_t i=0; i < count; ++i )
		{
			Ogre::String dataFolder = originalDataFolder + c_locations[i];
			addResourceLocation( dataFolder, getMediaReadArchiveType(), "General" );
		}
	}

	void registerHlms() override
	{
		GraphicsSystem::registerHlms();

		Ogre::ConfigFile cf;
		cf.load( AndroidSystems::openFile( mResourcePath + "resources2.cfg" ) );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
		Ogre::String rootHlmsFolder = Ogre::macBundlePath() + '/' +
			cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
		Ogre::String rootHlmsFolder = mResourcePath +
			cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif
		if( rootHlmsFolder.empty() )
			rootHlmsFolder = AndroidSystems::isAndroid() ? "/" : "./";
		else if( *(rootHlmsFolder.end() - 1) != '/' )
			rootHlmsFolder += "/";

		Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();

		Ogre::String shaderSyntax = "GLSL";
		if (renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem")
			shaderSyntax = "GLSLES";
		if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
			shaderSyntax = "HLSL";
		else if( renderSystem->getName() == "Metal Rendering Subsystem" )
			shaderSyntax = "Metal";

		Ogre::String mainFolderPath;
		Ogre::StringVector libraryFoldersPaths;
		Ogre::StringVector::const_iterator libraryFolderPathIt;
		Ogre::StringVector::const_iterator libraryFolderPathEn;

		Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

		Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();

		{
			//Create & Register HlmsTerra
			//Get the path to all the subdirectories used by HlmsTerra
			Ogre::HlmsTerra::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
			Ogre::Archive *archiveTerra = archiveManager.load( rootHlmsFolder + mainFolderPath,
																getMediaReadArchiveType(), true );
			Ogre::ArchiveVec archiveTerraLibraryFolders;
			libraryFolderPathIt = libraryFoldersPaths.begin();
			libraryFolderPathEn = libraryFoldersPaths.end();
			while( libraryFolderPathIt != libraryFolderPathEn )
			{
				Ogre::Archive *archiveLibrary = archiveManager.load(
					rootHlmsFolder + *libraryFolderPathIt, getMediaReadArchiveType(), true );
				archiveTerraLibraryFolders.push_back( archiveLibrary );
				++libraryFolderPathIt;
			}

			//Create and register the terra Hlms
			hlmsTerra = OGRE_NEW Ogre::HlmsTerra( archiveTerra, &archiveTerraLibraryFolders );
			hlmsManager->registerHlms( hlmsTerra );
		}

		//Add Terra's piece files that customize the PBS implementation.
		//These pieces are coded so that they will be activated when
		//we set the HlmsPbsTerraShadows listener and there's an active Terra
		//(see OgreGame::createScene01)
		Ogre::Hlms *hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS );
		Ogre::Archive *archivePbs = hlmsPbs->getDataFolder();
		Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
		libraryPbs.push_back( Ogre::ArchiveManager::getSingletonPtr()->load(
									rootHlmsFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows",
									getMediaReadArchiveType(), true ) );
		hlmsPbs->reloadFrom( archivePbs, &libraryPbs );
	}

public:
	Tutorial_TerrainGraphicsSystem( GameState *gameState ) :
		GraphicsSystem( gameState ),
		mTerraWorkspaceListener( 0 )
	{
		mResourcePath = "./";
	}
};

void MainEntryPoints::createSystems( GameState **outGraphicsGameState,
										GraphicsSystem **outGraphicsSystem,
										GameState **outLogicGameState,
										LogicSystem **outLogicSystem )
{
	OgreGame *gfxGameState = new OgreGame();

	Tutorial_TerrainGraphicsSystem *graphicsSystem =
		new Tutorial_TerrainGraphicsSystem( gfxGameState );

	gfxGameState->_notifyGraphicsSystem( graphicsSystem );

	*outGraphicsGameState = gfxGameState;
	*outGraphicsSystem = graphicsSystem;
}

void MainEntryPoints::destroySystems( GameState *graphicsGameState,
										GraphicsSystem *graphicsSystem,
										GameState *logicGameState,
										LogicSystem *logicSystem )
{
	delete graphicsSystem;
	delete graphicsGameState;
}

const char* MainEntryPoints::getWindowTitle()
{
	return "Stunt Rally 3";
}


#if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMainApp( HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR strCmdLine, INT nCmdShow )
#else
int mainApp( int argc, const char *argv[] )
#endif
{
	return MainEntryPoints::mainAppSingleThreaded( DEMO_MAIN_ENTRY_PARAMS );
	// return MainEntryPoints::mainAppMultiThreaded( DEMO_MAIN_ENTRY_PARAMS );  // todo
}
#endif
