/*
Modified by CryHam  under GPLv3
-----------------------------------------------------------------------------
This source file WAS part of OGRE-Next
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

#include "Terra.h"
#include "TerraShadowMapper.h"
#include "OgreHlmsTerra.h"

#include <OgreTextureGpu.h>
#include <OgreVector4.h>
#include <OgreImage2.h>

#include <Compositor/OgreCompositorChannel.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <OgreCamera.h>
#include <OgreDepthBuffer.h>
#include <OgreMaterialManager.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreSceneManager.h>
#include <OgreStagingTexture.h>
#include <OgreTechnique.h>
#include <OgreTextureGpuManager.h>

#include "Def_Str.h"
#include "CScene.h"
#include "SceneXml.h"


namespace Ogre
{
	inline Vector3 ZupToYup( Vector3 value )
	{
		std::swap( value.y, value.z );
		value.z = -value.z;
		return value;
	}

	inline Ogre::Vector3 YupToZup( Ogre::Vector3 value )
	{
		std::swap( value.y, value.z );
		value.y = -value.y;
		return value;
	}

	//  🌟 ctor
	Terra::Terra(
			App* app1, Scene* sc1, int n,
			ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager,
			uint8 renderQueueId,
			CompositorManager2 *compositorManager, Camera *camera,
			bool zUp ) :
		MovableObject( IdType(n), objectMemoryManager, sceneManager, renderQueueId ),
		app(app1), sc(sc1),
		cnt(n),
		m_iWidth( 0u ),
		m_iHeight( 0u ),
		m_depthWidthRatio( 1.0f ),
		m_skirtSize( 10.0f ),
		m_invWidth( 1.0f ),
		m_invDepth( 1.0f ),
		m_zUp( zUp ),
		m_xzDimensions( Vector2::UNIT_SCALE ),
		m_xzInvDimensions( Vector2::UNIT_SCALE ),
		m_xzRelativeSize( Vector2::UNIT_SCALE ),
		m_fHeightMul( 1.0f ),
		m_heightUnormScaled( 1.0f ),
		m_terrainOrigin( Vector3::ZERO ),
		m_basePixelDimension( 256u ),
		m_currentCell( 0u ),

		m_prevLightDir( Vector3::ZERO ),
		m_shadowMapper( 0 ),
		m_compositorManager( compositorManager ),
		m_camera( camera ),
		mHlmsTerraIndex( std::numeric_limits<uint32>::max() ),

		m_heightMapTex( 0 )

		,normalmap( this )
		,blendmap( this )

		,bGenerateShadowMap(0)  //** ter par  //^^ todo: 1 in ed
		,bNormalized( 0 )
		,fHMin(-100.f), fHMax(100.f)
		,iLodMax( 4 )  // 5 lq, max lods .. 0 hq, all lod0
	{
		fHRange = fHMax - fHMin;
	}

	//  💥 dtor
	//-----------------------------------------------------------------------------------
	Terra::~Terra()
	{
		if( !m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock() )
		{
			HlmsDatablock *datablock = m_terrainCells[0].back().getDatablock();
			OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( datablock->getCreator() ) );
			HlmsTerra *hlms = static_cast<HlmsTerra *>( datablock->getCreator() );
			hlms->_unlinkTerra( this );
		}

		if( m_shadowMapper )
		{
			m_shadowMapper->destroyShadowMap();
			delete m_shadowMapper;
			m_shadowMapper = 0;
		}

		blendmap.Destroy();  //*
		normalmap.Destroy();
		destroyHeightmapTex();
		
		m_terrainCells[0].clear();
		m_terrainCells[1].clear();
	}

	//-----------------------------------------------------------------------------------
	Vector3 Terra::fromYUp( Vector3 value ) const
	{
		if( m_zUp )
			return YupToZup( value );
		return value;
	}
	//-----------------------------------------------------------------------------------
	Vector3 Terra::fromYUpSignPreserving( Vector3 value ) const
	{
		if( m_zUp )
			std::swap( value.y, value.z );
		return value;
	}
	//-----------------------------------------------------------------------------------
	Vector3 Terra::toYUp( Vector3 value ) const
	{
		if( m_zUp )
			return ZupToYup( value );
		return value;
	}
	//-----------------------------------------------------------------------------------
	Vector3 Terra::toYUpSignPreserving( Vector3 value ) const
	{
		if( m_zUp )
			std::swap( value.y, value.z );
		return value;
	}


	//  💥 Destroy Heightmap
	//-----------------------------------------------------------------------------------
	void Terra::destroyHeightmapTex()
	{
		if( m_heightMapTex )
		{
			TextureGpuManager *mgr =
					mManager->getDestinationRenderSystem()->getTextureGpuManager();
			mgr->destroyTexture( m_heightMapTex );
			m_heightMapTex = 0;
		}
	}

	//  🆕 Create Heightmap
	//-----------------------------------------------------------------------------------
	void Terra::createHeightmap(
			int width, int height, std::vector<float> hfHeight, int row,
			bool bMinimizeMemory, bool bLowResShadow )
	{
		m_iWidth = width;
		m_iHeight = height;
		m_depthWidthRatio = m_iHeight / (float)(m_iWidth);
		m_invWidth = 1.0f / m_iWidth;
		m_invDepth = 1.0f / m_iHeight;

		m_heightMap.resize( m_iWidth * m_iHeight );

		createHmapTex( hfHeight, row );

		
		m_xzRelativeSize = m_xzDimensions / Vector2( static_cast<Real>(m_iWidth),
													 static_cast<Real>(m_iHeight) );

		normalmap.Create();  //*
		blendmap.Create();

		m_prevLightDir = Vector3::ZERO;

		delete m_shadowMapper;
		m_shadowMapper = new ShadowMapper( mManager, m_compositorManager, bGenerateShadowMap );
		m_shadowMapper->setMinimizeMemoryConsumption( bMinimizeMemory );
		m_shadowMapper->createShadowMap( getId(), m_heightMapTex, bLowResShadow );

		calculateOptimumSkirtSize();
	}


	//  🆕 Create Normalmap
	//-----------------------------------------------------------------------------------
	void Terra::Normalmap::Create()
	{
		Destroy();

		auto sceneMgr = pTerra->mManager;
		TextureGpuManager *texMgr = sceneMgr->getDestinationRenderSystem()->getTextureGpuManager();
		texture = texMgr->createTexture(
			"NormalMapTex_" + toStr(pTerra->cnt), //pTerra->getId() ),
			GpuPageOutStrategy::SaveToSystemRam,
			TextureFlags::ManualTexture, TextureTypes::Type2D );
		
		texture->setResolution( pTerra->m_heightMapTex->getWidth(),
								pTerra->m_heightMapTex->getHeight() );
		texture->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount(
			texture->getWidth(), texture->getHeight() ) );
		if( texMgr->checkSupport(
			PFG_R10G10B10A2_UNORM, TextureTypes::Type2D,
			TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps ) )
		{
			texture->setPixelFormat( PFG_R10G10B10A2_UNORM );
		}else{
			texture->setPixelFormat( PFG_RGBA8_UNORM );
		}
		texture->scheduleTransitionTo( GpuResidency::Resident );

		auto si = toStr(pTerra->cnt);
		auto name = "Rtt_NormalMapTex_" + si;
		rtt = TerraSharedResources::getTempTexture(
			name.c_str(), pTerra->getId(),
			texture, TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps );

		MaterialPtr normalMapperMat = MaterialManager::getSingleton().load(
			"Terra/GpuNormalMapper" + si,
			ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ).staticCast<Material>();
		Pass *pass = normalMapperMat->getTechnique(0)->getPass(0);
		TextureUnitState *texUnit = pass->getTextureUnitState(0);

		texUnit->setTexture( pTerra->m_heightMapTex );

		// Normalize vScale for better precision in the shader math
		const Vector3 vScale =
			Vector3( pTerra->m_xzRelativeSize.x, pTerra->m_heightUnormScaled,
					 pTerra->m_xzRelativeSize.y ).normalisedCopy();

		GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
		psParams->setNamedConstant( "heightMapResolution", Vector4(
			static_cast<Real>( pTerra->m_iWidth ),
			static_cast<Real>( pTerra->m_iHeight ), 1, 1 ) );
		psParams->setNamedConstant( "vScale", vScale );

		CompositorChannelVec finalTargetChannels( 1, CompositorChannel() );
		finalTargetChannels[0] = rtt;


		camera = sceneMgr->createCamera( "CamTerraNormal" + si );

		auto mgr = pTerra->m_compositorManager;
		const IdString workspaceName = "Terra/GpuNormalMapperWorkspace" + si;

		//  add Workspace
		LogO("--++ WS add:  Ter Normals "+si+", all: "+toStr(mgr->getNumWorkspaces()));
		workspace = mgr->addWorkspace(
			sceneMgr, finalTargetChannels, camera, workspaceName, false );

		Update();

		#ifndef SR_EDITOR  // todo: game no upd ..
		// m_compositorManager->removeWorkspace( workspace );
		// mManager->destroyCamera( camera );
		// TerraSharedResources::destroyTempTexture( m_sharedResources, rtt );
		#endif
	}

	//  💫 Update Normalmap
	//-----------------------------------------------------------------------------------
	void Terra::Normalmap::Update()
	{
		if (!workspace)  return;
		workspace->_beginUpdate(true);
		workspace->_update();
		workspace->_endUpdate(true);

		for (uint8 i = 0u; i < texture->getNumMipmaps(); ++i)
		{
			rtt->copyTo( texture, texture->getEmptyBox(i), i,
				rtt->getEmptyBox(i), i );
		}
	}

	//  💥 Destroy Normalmap
	//-----------------------------------------------------------------------------------
	void Terra::Normalmap::Destroy()
	{
		TextureGpuManager *mgr =
			pTerra->mManager->getDestinationRenderSystem()->getTextureGpuManager();
		if (workspace)
		{	pTerra->m_compositorManager->removeWorkspace( workspace );
			workspace = 0;
		}
		if (camera)
		{	pTerra->mManager->destroyCamera( camera );
			camera = 0;
		}
		if( rtt )
		{	mgr->destroyTexture( rtt );
			rtt = 0;
		}
		if( texture )
		{	mgr->destroyTexture( texture );
			texture = 0;
		}
	}

	//-----------------------------------------------------------------------------------
	void Terra::calculateOptimumSkirtSize()
	{
		m_skirtSize = std::numeric_limits<float>::max();

		const uint32 basePixelDimension = m_basePixelDimension;
		const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension * m_depthWidthRatio);

		for( size_t y=vertPixelDimension-1u; y<m_iHeight-1u; y += vertPixelDimension )
		{
			const size_t ny = y + 1u;

			bool allEqualInLine = true;
			float minHeight = m_heightMap[y * m_iWidth];
			for( size_t x=0; x<m_iWidth; ++x )
			{
				const float minValue = std::min( m_heightMap[y * m_iWidth + x],
												  m_heightMap[ny * m_iWidth + x] );
				minHeight = std::min( minValue, minHeight );
				allEqualInLine &= m_heightMap[y * m_iWidth + x] == m_heightMap[ny * m_iWidth + x];
			}

			if( !allEqualInLine )
				m_skirtSize = std::min( minHeight, m_skirtSize );
		}

		for( size_t x=basePixelDimension-1u; x<m_iWidth-1u; x += basePixelDimension )
		{
			const size_t nx = x + 1u;

			bool allEqualInLine = true;
			float minHeight = m_heightMap[x];
			for( size_t y=0; y<m_iHeight; ++y )
			{
				const float minValue = std::min( m_heightMap[y * m_iWidth + x],
												  m_heightMap[y * m_iWidth + nx] );
				minHeight = std::min( minValue, minHeight );
				allEqualInLine &= m_heightMap[y * m_iWidth + x] == m_heightMap[y * m_iWidth + nx];
			}

			if( !allEqualInLine )
				m_skirtSize = std::min( minHeight, m_skirtSize );
		}

		m_skirtSize /= m_fHeightMul;
	}

	//-----------------------------------------------------------------------------------
	inline GridPoint Terra::worldToGrid( const Vector3 &vPos ) const
	{
		GridPoint retVal;
		const float fWidth = static_cast<float>( m_iWidth );
		const float fDepth = static_cast<float>( m_iHeight );

		const float fX = floorf( ((vPos.x - m_terrainOrigin.x) * m_xzInvDimensions.x) * fWidth );
		const float fZ = floorf( ((vPos.z - m_terrainOrigin.z) * m_xzInvDimensions.y) * fDepth );
		retVal.x = fX >= 0.0f ? static_cast<uint32>( fX ) : 0xffffffff;
		retVal.z = fZ >= 0.0f ? static_cast<uint32>( fZ ) : 0xffffffff;

		return retVal;
	}
	//-----------------------------------------------------------------------------------
	inline Vector2 Terra::gridToWorld( const GridPoint &gPos ) const
	{
		Vector2 retVal;
		const float fWidth = static_cast<float>( m_iWidth );
		const float fDepth = static_cast<float>( m_iHeight );

		retVal.x = (gPos.x / fWidth) * m_xzDimensions.x + m_terrainOrigin.x;
		retVal.y = (gPos.z / fDepth) * m_xzDimensions.y + m_terrainOrigin.z;

		return retVal;
	}
	//-----------------------------------------------------------------------------------
	bool Terra::isVisible( const GridPoint &gPos, const GridPoint &gSize ) const
	{

		if( gPos.x >= static_cast<int32>( m_iWidth ) ||
			gPos.z >= static_cast<int32>( m_iHeight ) ||
			gPos.x + gSize.x <= 0 ||
			gPos.z + gSize.z <= 0 )
		{
			//Outside terrain bounds.
			return false;
		}

		if (!bNormalized)
			return true;  //** ter temp fix bug unnorm vis

		const Vector2 cellPos = gridToWorld( gPos );
		const Vector2 cellSize( (gSize.x + 1u) * m_xzRelativeSize.x,
								(gSize.z + 1u) * m_xzRelativeSize.y );

		const Vector3 vHalfSizeYUp = Vector3( cellSize.x, m_fHeightMul, cellSize.y ) * 0.5f;
		const Vector3 vCenter =
			fromYUp( Vector3( cellPos.x, m_terrainOrigin.y, cellPos.y ) + vHalfSizeYUp );
		const Vector3 vHalfSize = fromYUpSignPreserving( vHalfSizeYUp );

		for( unsigned i = 0; i < 6u; ++i )
		{
			//Skip far plane if view frustum is infinite
			if( i == FRUSTUM_PLANE_FAR && m_camera->getFarClipDistance() == 0 )
				continue;

			Plane::Side side = m_camera->getFrustumPlane(i).getSide( vCenter, vHalfSize );

			//We only need one negative match to know the obj is outside the frustum
			if( side == Plane::NEGATIVE_SIDE )
				return false;
		}

		return true;
	}
	//-----------------------------------------------------------------------------------
	void Terra::addRenderable( const GridPoint &gridPos, const GridPoint &cellSize, uint32 lodLevel )
	{
		TerrainCell *cell = &m_terrainCells[0][m_currentCell++];
		cell->setOrigin( gridPos, cellSize.x, cellSize.z, lodLevel );
		m_collectedCells[0].push_back( cell );
	}

	//-----------------------------------------------------------------------------------
	void Terra::optimizeCellsAndAdd()
	{
		//Keep iterating until m_collectedCells[0] stops shrinking
		size_t numCollectedCells = std::numeric_limits<size_t>::max();
		while( numCollectedCells != m_collectedCells[0].size() )
		{
			numCollectedCells = m_collectedCells[0].size();

			if( m_collectedCells[0].size() > 1 )
			{
				m_collectedCells[1].clear();

				std::vector<TerrainCell*>::const_iterator itor = m_collectedCells[0].begin();
				std::vector<TerrainCell*>::const_iterator end  = m_collectedCells[0].end();

				while( end - itor >= 2u )
				{
					TerrainCell *currCell = *itor;
					TerrainCell *nextCell = *(itor+1);

					m_collectedCells[1].push_back( currCell );
					if( currCell->merge( nextCell ) )
						itor += 2;
					else
						++itor;
				}

				while( itor != end )
					m_collectedCells[1].push_back( *itor++ );

				m_collectedCells[1].swap( m_collectedCells[0] );
			}
		}

		std::vector<TerrainCell*>::const_iterator itor = m_collectedCells[0].begin();
		std::vector<TerrainCell*>::const_iterator end  = m_collectedCells[0].end();
		while( itor != end )
			mRenderables.push_back( *itor++ );

		m_collectedCells[0].clear();
	}


	//  💫 update
	//-----------------------------------------------------------------------------------
	void Terra::update( const Vector3 &lightDir, Camera* camera, float lightEpsilon )
	{
		const float lightCosAngleChange = Math::Clamp(
					(float)m_prevLightDir.dotProduct( lightDir.normalisedCopy() ), -1.0f, 1.0f );
		if( lightCosAngleChange <= (1.0f - lightEpsilon) )
		{
			m_shadowMapper->updateShadowMap( toYUp( lightDir ), m_xzDimensions, m_fHeightMul );
			m_prevLightDir = lightDir.normalisedCopy();
		}
		//m_shadowMapper->updateShadowMap( Vector3::UNIT_X, m_xzDimensions, m_height );
		//m_shadowMapper->updateShadowMap( Vector3(2048,0,1024), m_xzDimensions, m_height );
		//m_shadowMapper->updateShadowMap( Vector3(1,0,0.1), m_xzDimensions, m_height );
		//m_shadowMapper->updateShadowMap( Vector3::UNIT_Y, m_xzDimensions, m_height ); //Check! Does NAN

		mRenderables.clear();
		m_currentCell = 0;

		if (camera)
			m_camera = camera;
		const Vector3 camPos = toYUp( m_camera->getDerivedPosition() );

		const uint32 basePixelDimension = m_basePixelDimension;
		const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension * m_depthWidthRatio);

		GridPoint cellSize;
		cellSize.x = basePixelDimension;
		cellSize.z = vertPixelDimension;

		//Quantize the camera position to basePixelDimension steps
		GridPoint camCenter = worldToGrid( camPos );
		camCenter.x = (camCenter.x / basePixelDimension) * basePixelDimension;
		camCenter.z = (camCenter.z / vertPixelDimension) * vertPixelDimension;

		uint32 currentLod = 0;

//        camCenter.x = 64;
//        camCenter.z = 64;

		//LOD 0: Add full 4x4 grid
		for( int32 z=-2; z<2; ++z )
		{
			for( int32 x=-2; x<2; ++x )
			{
				GridPoint pos = camCenter;
				pos.x += x * cellSize.x;
				pos.z += z * cellSize.z;

				if( isVisible( pos, cellSize ) )
					addRenderable( pos, cellSize, currentLod );
			}
		}

		optimizeCellsAndAdd();

		m_currentCell = 16u; //The first 16 cells don't use skirts.

		const uint32 maxRes = std::max( m_iWidth, m_iHeight );
		//TODO: When we're too far (outside the terrain), just display a 4x4 grid or something like that.

		size_t numObjectsAdded = std::numeric_limits<size_t>::max();
		//LOD n: Add 4x4 grid, ignore 2x2 center (which
		//is the same as saying the borders of the grid)
		while( numObjectsAdded != m_currentCell ||
			   (mRenderables.empty() && (1u << currentLod) <= maxRes) )
		{
			numObjectsAdded = m_currentCell;

			cellSize.x <<= 1u;
			cellSize.z <<= 1u;
			
			#if 1  //**  0 for all at lod0, max tris
			if (iLodMax > 0)
			if (currentLod < iLodMax)  //** ter 5 par lod max
				++currentLod;
			#endif

			//Row 0
			{
				const int32 z = 1;
				for( int32 x=-2; x<2; ++x )
				{
					GridPoint pos = camCenter;
					pos.x += x * cellSize.x;
					pos.z += z * cellSize.z;

					if( isVisible( pos, cellSize ) )
						addRenderable( pos, cellSize, currentLod );
				}
			}
			//Row 3
			{
				const int32 z = -2;
				for( int32 x=-2; x<2; ++x )
				{
					GridPoint pos = camCenter;
					pos.x += x * cellSize.x;
					pos.z += z * cellSize.z;

					if( isVisible( pos, cellSize ) )
						addRenderable( pos, cellSize, currentLod );
				}
			}
			//Cells [0, 1] & [0, 2];
			{
				const int32 x = -2;
				for( int32 z=-1; z<1; ++z )
				{
					GridPoint pos = camCenter;
					pos.x += x * cellSize.x;
					pos.z += z * cellSize.z;

					if( isVisible( pos, cellSize ) )
						addRenderable( pos, cellSize, currentLod );
				}
			}
			//Cells [3, 1] & [3, 2];
			{
				const int32 x = 1;
				for( int32 z=-1; z<1; ++z )
				{
					GridPoint pos = camCenter;
					pos.x += x * cellSize.x;
					pos.z += z * cellSize.z;

					if( isVisible( pos, cellSize ) )
						addRenderable( pos, cellSize, currentLod );
				}
			}

			optimizeCellsAndAdd();
		}
	}
	
	//-----------------------------------------------------------------------------------
	void Terra::load( int width, int height, std::vector<float> hfHeight, int row,
					  Vector3 center, Vector3 dimensions, bool bMinimizeMemoryConsumption,
					  bool bLowResShadow )
	{
		// Use sign-preserving because origin in XZ plane is always from
		// bottom-left to top-right.
		// If we use toYUp, we'll start from top-right and go up and right
		m_terrainOrigin = toYUpSignPreserving( center - dimensions * 0.5f );
		center = toYUp( center );
		
		dimensions = toYUpSignPreserving( dimensions );
		m_xzDimensions = Vector2( dimensions.x, dimensions.z );
		m_xzInvDimensions = 1.0f / m_xzDimensions;
		m_fHeightMul = dimensions.y;
		m_basePixelDimension = 64u;

		createHeightmap( width, height, hfHeight, row,
			bMinimizeMemoryConsumption, bLowResShadow );

		{
			//Find out how many TerrainCells we need. I think this might be
			//solved analitically with a power series. But my math is rusty.
			const uint32 basePixelDimension = m_basePixelDimension;
			const uint32 vertPixelDimension = static_cast<uint32>( m_basePixelDimension *
																   m_depthWidthRatio );
			const uint32 maxPixelDimension = std::max( basePixelDimension, vertPixelDimension );
			const uint32 maxRes = std::max( m_iWidth, m_iHeight );

			uint32 numCells = 16u; //4x4
			uint32 accumDim = 0u;
			uint32 iteration = 1u;
			while( accumDim < maxRes )
			{
				numCells += 12u; //4x4 - 2x2
				accumDim += maxPixelDimension * (1u << iteration);
				++iteration;
			}

			numCells += 12u;
			accumDim += maxPixelDimension * (1u << iteration);
			++iteration;

			for( size_t i = 0u; i < 2u; ++i )
			{
				m_terrainCells[i].clear();
				m_terrainCells[i].resize( numCells, TerrainCell( this ) );
			}
		}

		VaoManager *vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

		for( size_t i = 0u; i < 2u; ++i )
		{
			std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
			std::vector<TerrainCell>::iterator endt = m_terrainCells[i].end();

			const std::vector<TerrainCell>::iterator begin = itor;

			while( itor != endt )
			{
				itor->initialize( vaoManager, ( itor - begin ) >= 16u );
				++itor;
			}
		}
	}


	//  get Height
	//-----------------------------------------------------------------------------------
	bool Terra::getHeightAt( Vector3 &vPosArg ) const
	{
		bool inside = false;

		Vector3 vPos = toYUp( vPosArg );

		GridPoint pos2D = worldToGrid( vPos );

		if( pos2D.x < m_iWidth-1 && pos2D.z < m_iHeight-1 )
		{
			const Vector2 vPos2D = gridToWorld( pos2D );

			const float dx = (vPos.x - vPos2D.x) * m_iWidth * m_xzInvDimensions.x;
			const float dz = (vPos.z - vPos2D.y) * m_iHeight * m_xzInvDimensions.y;

			float a, b, c;
			const float h00 = m_heightMap[ pos2D.z * m_iWidth + pos2D.x ];
			const float h11 = m_heightMap[ (pos2D.z+1) * m_iWidth + pos2D.x + 1 ];

			c = h00;
			if( dx < dz )
			{
				//Plane eq: y = ax + bz + c
				//x=0 z=0 -> c		= h00
				//x=0 z=1 -> b + c	= h01 -> b = h01 - c
				//x=1 z=1 -> a + b + c  = h11 -> a = h11 - b - c
				const float h01 = m_heightMap[ (pos2D.z+1) * m_iWidth + pos2D.x ];

				b = h01 - c;
				a = h11 - b - c;
			}
			else
			{
				//Plane eq: y = ax + bz + c
				//x=0 z=0 -> c		= h00
				//x=1 z=0 -> a + c	= h10 -> a = h10 - c
				//x=1 z=1 -> a + b + c  = h11 -> b = h11 - a - c
				const float h10 = m_heightMap[ pos2D.z * m_iWidth + pos2D.x + 1 ];

				a = h10 - c;
				b = h11 - a - c;
			}

			vPos.y = a * dx + b * dz + c + m_terrainOrigin.y;
			// if (bNormalized)
				// vPos.y = vPos.y * fHRange + fHMin;  //** ter scale
			inside = true;
		}

		vPosArg = fromYUp( vPos );

		return inside;
	}


	//-----------------------------------------------------------------------------------
	void Terra::setDatablock( HlmsDatablock *datablock )
	{
		if( !datablock && !m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock() )
		{
			// Unsetting the datablock. We have no way of unlinking later on. Do it now
			HlmsDatablock *oldDatablock = m_terrainCells[0].back().getDatablock();
			OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( oldDatablock->getCreator() ) );
			HlmsTerra *hlms = static_cast<HlmsTerra *>( oldDatablock->getCreator() );
			hlms->_unlinkTerra( this );
		}

		for( size_t i = 0u; i < 2u; ++i )
		{
			std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
			std::vector<TerrainCell>::iterator end  = m_terrainCells[i].end();

			while( itor != end )
			{
				itor->setDatablock( datablock );
				++itor;
			}
		}

		if( mHlmsTerraIndex == std::numeric_limits<uint32>::max() )
		{
			OGRE_ASSERT_HIGH( dynamic_cast<HlmsTerra *>( datablock->getCreator() ) );
			HlmsTerra *hlms = static_cast<HlmsTerra *>( datablock->getCreator() );
			hlms->_linkTerra( this );
		}
	}

	//-----------------------------------------------------------------------------------
	Ogre::TextureGpu* Terra::_getShadowMapTex() const
	{
		return m_shadowMapper->getShadowMapTex();
	}

	//-----------------------------------------------------------------------------------
	Vector3 Terra::getTerrainOrigin() const
	{
		return fromYUpSignPreserving( m_terrainOrigin );
	}
	//-----------------------------------------------------------------------------------
	Vector2 Terra::getTerrainXZCenter() const
	{
		return Vector2( m_terrainOrigin.x + m_xzDimensions.x * 0.5f,
						m_terrainOrigin.z + m_xzDimensions.y * 0.5f );
	}

	//-----------------------------------------------------------------------------------
	const String& Terra::getMovableType() const
	{
		static const String movType = "Terra";
		return movType;
	}
	//-----------------------------------------------------------------------------------
	void Terra::_swapSavedState()
	{
		m_terrainCells[0].swap( m_terrainCells[1] );
		m_savedState.m_renderables.swap( mRenderables );
		std::swap( m_savedState.m_currentCell, m_currentCell );
		std::swap( m_savedState.m_camera, m_camera );
	}


	//-----------------------------------------------------------------------------------
	TextureGpu *TerraSharedResources::getTempTexture( const char *texName, IdType id,
													  TextureGpu *baseTemplate, uint32 flags )
	{
		TextureGpuManager *mgr = baseTemplate->getTextureManager();
		TextureGpu *tmpRtt = mgr->createTexture( texName + toStr( id ),
												GpuPageOutStrategy::Discard, flags,
												TextureTypes::Type2D );
		tmpRtt->copyParametersFrom( baseTemplate );
		tmpRtt->scheduleTransitionTo( GpuResidency::Resident );
		if( flags & TextureFlags::RenderToTexture )
			tmpRtt->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN );

		return tmpRtt;
	}

	//-----------------------------------------------------------------------------------
	void TerraSharedResources::destroyTempTexture( TextureGpu *tmpRtt )
	{
		TextureGpuManager *mgr = tmpRtt->getTextureManager();
		mgr->destroyTexture( tmpRtt );
	}
}
