//-------------------------------------------------------------
// File: main.cpp
//
// Desc: 볼륨포그
//-------------------------------------------------------------
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <basetsd.h>
#include <math.h>
#include <stdio.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <tchar.h>
#include "DXUtil.h"
#include "D3DEnumeration.h"
#include "D3DSettings.h"
#include "D3DApp.h"
#include "D3DFont.h"
#include "D3DFile.h"
#include "D3DUtil.h"
#include "resource.h"
#include "main.h"


//-------------------------------------------------------------
// 전역변수
//-------------------------------------------------------------
CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;


//-------------------------------------------------------------
// Name: WinMain()
// Desc: 메인함수
//-------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    g_pApp  = &d3dApp;
    g_hInst = hInst;

    InitCommonControls();
    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}




//-------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: 어플리케이션 생성자
//-------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
	m_pMesh						= new CD3DMesh();
	m_pMeshBg					= new CD3DMesh();
	m_pDecl						= NULL;
	m_pEffect					= NULL;
	m_hmWVP						= NULL;
	m_hmWVPT					= NULL;
	m_hvCol						= NULL;
	m_hvDir						= NULL;

	m_pMapZ						= NULL;
	m_pColorMap					= NULL;
	m_pColorMapSurf				= NULL;
	m_pDepthMap					= NULL;
	m_pDepthMapSurf				= NULL;
	m_pFogMap					= NULL;
	m_pFogMapSurf				= NULL;

	m_zoom						= 5.0f;
    m_fWorldRotX                = -0.8f;
	m_fWorldRotY                = 0;

	m_dwCreationWidth           = 500;
    m_dwCreationHeight          = 375;
    m_strWindowTitle            = TEXT( "main" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
	m_bStartFullscreen			= false;
	m_bShowCursorWhenFullscreen	= false;

    m_pFont                     = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_bLoadingApp               = TRUE;
    
	ZeroMemory( &m_UserInput, sizeof(m_UserInput) );
}




//-------------------------------------------------------------
// Name: ~CMyD3DApplication()
// Desc: 소멸자
//-------------------------------------------------------------
CMyD3DApplication::~CMyD3DApplication()
{
}




//-------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: 단 한번만 초기화
//       윈도우 초기화와 IDirect3D9초기화는 끝난뒤
//       그러나 LPDIRECT3DDEVICE9초기화는 끝나지 않은 상태
//-------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // 로딩 메시지 출력
    SendMessage( m_hWnd, WM_PAINT, 0, 0 );

    m_bLoadingApp = FALSE;

    return S_OK;
}




//-------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: 초기화시 호출됨. 필요한 능력(caps)체크
//-------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps
						, DWORD dwBehavior, D3DFORMAT Format )
{
    UNREFERENCED_PARAMETER( Format );
    UNREFERENCED_PARAMETER( dwBehavior );
    UNREFERENCED_PARAMETER( pCaps );
    
	// 셰이더체크
	if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) &&
	  !(dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) )
		return E_FAIL;	// 정점셰이더
	
	if( pCaps->PixelShaderVersion < D3DPS_VERSION(2,0))
		return E_FAIL;	// 픽셀셰이더

	// MRT(Multiple Render Target)를 2장 사용한다
	if(pCaps->NumSimultaneousRTs < 2) return E_FAIL;

	return S_OK;
}


//-------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: 디바이스가 생성된후의 초기화
//       프레임버퍼 포맷과 디바이스 종류가 변한뒤에 호출
//       여기서 확보한 메모리는 DeleteDeviceObjects()에서 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    HRESULT hr;

    // 메시읽기
	if( FAILED( hr=m_pMesh->Create( m_pd3dDevice, "ufo.x" )))
        return DXTRACE_ERR( "Load Mesh", hr );
	m_pMesh->UseMeshMaterials(FALSE);// 텍스처를 직접설정
	m_pMesh->SetFVF( m_pd3dDevice, D3DFVF_XYZ | D3DFVF_NORMAL );

    // 메시읽기
	if( FAILED( hr=m_pMeshBg->Create( m_pd3dDevice, "map.x" )))
        return DXTRACE_ERR( "Load Mesh", hr );
	m_pMeshBg->UseMeshMaterials(FALSE);// 텍스처를 직접설정
	m_pMeshBg->SetFVF( m_pd3dDevice, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 );

	// 정점선언 오브젝트 생성
	D3DVERTEXELEMENT9 decl[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,	0},
		{0, 24, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};
	if( FAILED( hr = m_pd3dDevice->CreateVertexDeclaration( decl, &m_pDecl )))
		return DXTRACE_ERR ("CreateVertexDeclaration", hr);

	
	// 셰이더읽기
    if( FAILED( hr = D3DXCreateEffectFromFile(
						m_pd3dDevice, "hlsl.fx", NULL, NULL, 
						0, NULL, &m_pEffect, NULL ) ) )
		return DXTRACE_ERR( "CreateEffectFromFile", hr );
	m_hmWVP  = m_pEffect->GetParameterByName( NULL, "mWVP" );
	m_hmWVPT = m_pEffect->GetParameterByName( NULL, "mWVPT" );
	m_hvCol  = m_pEffect->GetParameterByName( NULL, "vCol" );
	m_hvDir  = m_pEffect->GetParameterByName( NULL, "vLightDir" );

    m_pFont->InitDeviceObjects( m_pd3dDevice );// 폰트
    
	return S_OK;
}

//-------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: 화면크기가 변했을때 호출됨
//       확보한 메모리는 InvalidateDeviceObjects()에서 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	D3DVIEWPORT9 viewport;
	
	// 새롭게 설정한 뷰포트로부터 텍스처를 재구성
	m_pd3dDevice->GetViewport(&viewport);
	m_Width  = viewport.Width;
	m_Height = viewport.Height;
	// 텍스처크기는 뷰포트크기 이상의 2^n
	for( m_MapW=1 ; m_MapW<m_Width  ; m_MapW*=2 );
	for( m_MapH=1 ; m_MapH<m_Height ; m_MapH*=2 );

	// 렌더링타겟의 깊이버퍼 생성
	if (FAILED(m_pd3dDevice->CreateDepthStencilSurface(m_MapW, m_MapH, 
		D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pMapZ, NULL)))
		return E_FAIL;
	// 컬러버퍼
	if (FAILED(m_pd3dDevice->CreateTexture( m_MapW, m_MapH, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pColorMap, NULL)))
		return E_FAIL;
	if (FAILED(m_pColorMap->GetSurfaceLevel(0, &m_pColorMapSurf)))
		return E_FAIL;
	// 깊이버퍼
	if (FAILED(m_pd3dDevice->CreateTexture(m_MapW, m_MapH, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_pDepthMap, NULL)))
		return E_FAIL;
	if (FAILED(m_pDepthMap->GetSurfaceLevel(0, &m_pDepthMapSurf)))
		return E_FAIL;
	// 포그버퍼
	if (FAILED(m_pd3dDevice->CreateTexture(m_MapW, m_MapH, 1, 
		D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &m_pFogMap, NULL)))
		return E_FAIL;
	if (FAILED(m_pFogMap->GetSurfaceLevel(0, &m_pFogMapSurf)))
		return E_FAIL;


	// 단축매크로
	#define RS   m_pd3dDevice->SetRenderState
	#define SAMP m_pd3dDevice->SetSamplerState

    // 렌더링 상태설정
    RS  ( D3DRS_ZENABLE,        TRUE );
    RS  ( D3DRS_LIGHTING,       FALSE );

    // 텍스처 상태설정
	SAMP( 0, D3DSAMP_ADDRESSU,  D3DTADDRESS_CLAMP );
	SAMP( 0, D3DSAMP_ADDRESSV,  D3DTADDRESS_CLAMP );
    SAMP( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    SAMP( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    // 월드행렬
    D3DXMatrixIdentity( &m_mWorld );

	// 뷰행렬
    D3DXVECTOR3 vFrom   = D3DXVECTOR3( 0.0f, 0.0f, -m_zoom );
    D3DXVECTOR3 vLookat = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUp     = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_mView, &vFrom, &vLookat, &vUp );

    // 투영행렬
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width )
				  / ((FLOAT)m_d3dsdBackBuffer.Height);
    D3DXMatrixPerspectiveFovLH( &m_mProj, D3DX_PI/4, fAspect
								, 1.0f, 100.0f );

	// 광원방향 설정
	m_LightDir = D3DXVECTOR4(-0.6f, 0.6f, -0.6f, 0.3f);
	
	m_pMesh  ->RestoreDeviceObjects(m_pd3dDevice);
	m_pMeshBg->RestoreDeviceObjects(m_pd3dDevice);
	if( m_pEffect!=NULL ) m_pEffect->OnResetDevice();// 셰이더

    m_pFont->RestoreDeviceObjects();	// 폰트

    return S_OK;
}




//-------------------------------------------------------------
// Name: FrameMove()
// Desc: 매 프레임마다 호출됨. 애니메이션 처리등 담당
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
	UpdateInput( &m_UserInput ); // 입력데이터 갱신

	//---------------------------------------------------------
	// 입력에 따라 좌표계를 갱신한다
	//---------------------------------------------------------
	// 회전
    if( m_UserInput.bRotateLeft && !m_UserInput.bRotateRight )
        m_fWorldRotY += m_fElapsedTime;
    else if( m_UserInput.bRotateRight && !m_UserInput.bRotateLeft )
        m_fWorldRotY -= m_fElapsedTime;

    if( m_UserInput.bRotateUp && !m_UserInput.bRotateDown )
        m_fWorldRotX += m_fElapsedTime;
    else if( m_UserInput.bRotateDown && !m_UserInput.bRotateUp )
        m_fWorldRotX -= m_fElapsedTime;
	// 줌
	if(m_UserInput.bZ && !m_UserInput.bX)
		m_zoom += 0.01f;
	else if(m_UserInput.bX && !m_UserInput.bZ)
		m_zoom -= 0.01f;


	//---------------------------------------------------------
	// 행렬갱신
	//---------------------------------------------------------
 	D3DXMATRIX matRotX, matRotY;
    D3DXMatrixRotationX( &matRotX, m_fWorldRotX );
    D3DXMatrixRotationY( &matRotY, m_fWorldRotY );
    D3DXMatrixMultiply( &m_mWorld, &matRotY, &matRotX );

    D3DXVECTOR3 vFrom   = D3DXVECTOR3( 0.0f, 0.0f, -m_zoom );
    D3DXVECTOR3 vLookat = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUp     = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_mView, &vFrom, &vLookat, &vUp );

	return S_OK;
}




//-------------------------------------------------------------
// Name: UpdateInput()
// Desc: 입력데이터 갱신
//-------------------------------------------------------------
void CMyD3DApplication::UpdateInput( UserInput* pUserInput )
{
    pUserInput->bRotateUp    = ( m_bActive && (GetAsyncKeyState( VK_UP )    & 0x8000) == 0x8000 );
    pUserInput->bRotateDown  = ( m_bActive && (GetAsyncKeyState( VK_DOWN )  & 0x8000) == 0x8000 );
    pUserInput->bRotateLeft  = ( m_bActive && (GetAsyncKeyState( VK_LEFT )  & 0x8000) == 0x8000 );
    pUserInput->bRotateRight = ( m_bActive && (GetAsyncKeyState( VK_RIGHT ) & 0x8000) == 0x8000 );
	pUserInput->bZ = ( m_bActive && (GetAsyncKeyState( 'Z' ) & 0x8000) == 0x8000 );
	pUserInput->bX = ( m_bActive && (GetAsyncKeyState( 'X' ) & 0x8000) == 0x8000 );
}




//-------------------------------------------------------------
// Name: Render()
// Desc: 화면 렌더
//-------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
 	LPDIRECT3DSURFACE9 pOldBackBuffer, pOldZBuffer;
	D3DVIEWPORT9 oldViewport;
	D3DXHANDLE hTechnique;
	D3DXMATRIX m, mL, mWT;
	D3DXVECTOR4 v;
	DWORD i;
	FLOAT ds = 0.5f/(FLOAT)m_MapW;// 텍셀중심 이동용
	FLOAT dt = 0.5f/(FLOAT)m_MapH;
	FLOAT s = (FLOAT)m_Width /(FLOAT)m_MapW + ds;
	FLOAT t = (FLOAT)m_Height/(FLOAT)m_MapH + dt;
	// 투영공간에서 텍스처좌표로 이동하는 변환행렬
	D3DXMATRIX	mT = D3DXMATRIX(
						0.5f*s, 0.0f  ,0.0f,0.0f,
						0.0f  ,-0.5f*t,0.0f,0.0f,
						0.0f  , 0.0f  ,1.0f,0.0f,
						0.5f*s, 0.5f*t,0.0f,1.0f);
	// 변환용뷰포트          x y  width   height   minz maxz
	D3DVIEWPORT9 viewport = {0,0, m_Width,m_Height,0.0f,1.0f};

    if( SUCCEEDED( m_pd3dDevice->BeginScene()))	// 렌더 시작
    {
		if(m_pEffect != NULL)
		{
			//-------------------------------------------------
			// 렌더링타겟 보존
			//-------------------------------------------------
			m_pd3dDevice->GetRenderTarget(0, &pOldBackBuffer);
			m_pd3dDevice->GetDepthStencilSurface(&pOldZBuffer);
			m_pd3dDevice->GetViewport(&oldViewport);

			//-------------------------------------------------
			// 렌더링타겟 변경
			//-------------------------------------------------
			m_pd3dDevice->SetRenderTarget(0, m_pColorMapSurf);
			m_pd3dDevice->SetRenderTarget(1, m_pDepthMapSurf);
			m_pd3dDevice->SetDepthStencilSurface(m_pMapZ);
			m_pd3dDevice->SetViewport(&viewport);

			// 렌더링타겟 클리어
			m_pd3dDevice->Clear(0L, NULL,
							D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
							0xffffff, 1.0f, 0L);

			//-------------------------------------------------
			// 셰이더 설정
			//-------------------------------------------------
			hTechnique = m_pEffect->GetTechniqueByName( "TShader" );
			m_pEffect->SetTechnique( hTechnique );
			m_pEffect->Begin( NULL, 0 );
			m_pEffect->Pass( 0 );
			
			//-------------------------------------------------
			// 셰이더 상수 설정
			//-------------------------------------------------
			// 좌표변환
			m = m_mWorld * m_mView * m_mProj;
			m_pEffect->SetMatrix( m_hmWVP, &m );

			// 광원
			D3DXMatrixInverse( &m, NULL, &m_mWorld);
			D3DXVec4Transform( &v, &m_LightDir, &m );
			D3DXVec4Normalize( &v, &v );v.w = 0.3f;
			m_pEffect->SetVector( m_hvDir, &v );

			//-------------------------------------------------
			// 렌더
			//-------------------------------------------------
			D3DMATERIAL9 *pMtrl = m_pMeshBg->m_pMaterials;
			for( i=0; i<m_pMeshBg->m_dwNumMaterials; i++ ) {
				// 메시색
				v.x = pMtrl->Diffuse.r;
				v.y = pMtrl->Diffuse.g;
				v.z = pMtrl->Diffuse.b;
				v.w = pMtrl->Diffuse.a;
				m_pEffect->SetVector( m_hvCol, &v );
				// 텍스처
				m_pEffect->SetTexture("DecaleMap"
								, m_pMeshBg->m_pTextures[i]);

				m_pMeshBg->m_pLocalMesh->DrawSubset( i ); // �`됪
				pMtrl++;
			}

			m_pEffect->End();

			//-------------------------------------------------
			//-------------------------------------------------
			// 패스2,3:포그맵 생성
			//-------------------------------------------------
			//-------------------------------------------------

			//-------------------------------------------------
			// 렌더링타겟 변경
			//-------------------------------------------------
			m_pd3dDevice->SetRenderTarget(0, m_pFogMapSurf);
			m_pd3dDevice->SetRenderTarget(1, NULL);
			m_pd3dDevice->SetViewport(&viewport);
			// 렌더링타겟 클리어
			m_pd3dDevice->Clear(0L, NULL,
							D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
							0x0, 1.0f, 0L);

			//-------------------------------------------------
			// 셰이더 설정
			//-------------------------------------------------
			hTechnique = m_pEffect->GetTechniqueByName( "TVolume" );
			m_pEffect->SetTechnique( hTechnique );
			m_pEffect->SetTexture("DepthMap", m_pDepthMap);
			m_pEffect->SetTexture("FrameBuffer", m_pFogMap);

			D3DXMatrixTranslation ( &mL, 0,1.0f,0 );
			m = mL*m_mWorld * m_mView * m_mProj;
			m_pEffect->SetMatrix( m_hmWVP, &m );

			mWT = m * mT;
			m_pEffect->SetMatrix( m_hmWVPT, &mWT );
			
			for( i=0 ; i<2 ; i++ ){
				for( DWORD j=0; j<m_pMesh->m_dwNumMaterials; j++ ){
					// 덇뱗�`됪귩뢎귦귞궧궲띋긚�^�[긣
					m_pd3dDevice->EndScene();
					m_pd3dDevice->BeginScene();
					m_pEffect->Begin( NULL, 0 );
					m_pEffect->Pass( i );

					m_pMesh->m_pLocalMesh->DrawSubset( j );
					m_pEffect->End();
				}
			}

			//-------------------------------------------------
			//-------------------------------------------------
			// 4패스:장면렌더
			//-------------------------------------------------
			//-------------------------------------------------

			//-------------------------------------------------
			// 렌더링타겟 복구
			//-------------------------------------------------
			m_pd3dDevice->SetRenderTarget(0, pOldBackBuffer);
			m_pd3dDevice->SetDepthStencilSurface(pOldZBuffer);
			m_pd3dDevice->SetViewport(&oldViewport);
			pOldBackBuffer->Release();
			pOldZBuffer->Release();

			//-------------------------------------------------
			// 셰이더 설정
			//-------------------------------------------------
			hTechnique = m_pEffect->GetTechniqueByName( "TFinal" );
			m_pEffect->SetTechnique( hTechnique );
			m_pEffect->Begin( NULL, 0 );
			m_pEffect->Pass( 0 );
			m_pEffect->SetTexture( "FogMap",   m_pFogMap );
			m_pEffect->SetTexture( "ColorMap", m_pColorMap );

			// 버퍼클리어
			m_pd3dDevice->Clear( 0L, NULL,
							D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
							0x00404080, 1.0f, 0L );

			m_pd3dDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX1 );
			typedef struct {FLOAT p[4]; FLOAT tu, tv;} TVERTEX;
			TVERTEX Vertex[4] = {
				//         x               y     z rhw tu  tv
				{             0,              0, 0, 1, ds, dt},
				{(FLOAT)m_Width,              0, 0, 1,  s, dt},
				{(FLOAT)m_Width,(FLOAT)m_Height, 0, 1,  s,  t},
				{             0,(FLOAT)m_Height, 0, 1, ds,  t},
			};
			m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLEFAN
							, 2, Vertex, sizeof( TVERTEX ) );

			m_pEffect->End();
		}

#if 0 // 디버그용 텍스처 출력
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLOROP,    D3DTOP_SELECTARG1);
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLORARG1,  D3DTA_TEXTURE);
	m_pd3dDevice->SetTextureStageState(1,D3DTSS_COLOROP,    D3DTOP_DISABLE);
	m_pd3dDevice->SetFVF( D3DFVF_XYZRHW | D3DFVF_TEX1 );
	m_pd3dDevice->SetVertexShader(NULL);
	m_pd3dDevice->SetPixelShader(0);
	for(DWORD loop = 0; loop < 3; loop++){
		const float scale = 128.0f;
		typedef struct {FLOAT p[4]; FLOAT tu, tv;} TVERTEX;

		TVERTEX Vertex[4] = {
			//       x        y   z rhw tu tv
			{(loop  )*scale,    0,0, 1, ds, dt,},
			{(loop+1)*scale,    0,0, 1,  s, dt,},
			{(loop+1)*scale,scale,0, 1,  s,  t,},
			{(loop  )*scale,scale,0, 1, ds,  t,},
		};
		switch(loop){
		case 0: m_pd3dDevice->SetTexture( 0, m_pColorMap ); break;
		case 1: m_pd3dDevice->SetTexture( 0, m_pDepthMap ); break;
		case 2: m_pd3dDevice->SetTexture( 0, m_pFogMap   ); break;
		}
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLEFAN, 2, Vertex, sizeof( TVERTEX ) );
	}
#endif      
		RenderText();				// 도움말 출력

        m_pd3dDevice->EndScene();	// 렌더종료
    }

    return S_OK;
}




//-------------------------------------------------------------
// Name: RenderText()
// Desc: 상태와 도움말을 화면에 출력
//-------------------------------------------------------------
HRESULT CMyD3DApplication::RenderText()
{
    D3DCOLOR fontColor        = D3DCOLOR_ARGB(255,255,255,0);
    TCHAR szMsg[MAX_PATH] = TEXT("");

    FLOAT fNextLine = 40.0f; 

    // 조작법 & 인수 출력
    fNextLine = (FLOAT) m_d3dsdBackBuffer.Height; 
	sprintf( szMsg, "Zoom: %f", m_zoom );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Use arrow keys to rotate object") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Press 'z' or 'x' to change zoom") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    // 디스플레이 상태 출력
    lstrcpy( szMsg, m_strFrameStats );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, m_strDeviceStats );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    
	return S_OK;
}




//-------------------------------------------------------------
// Name: MsgProc()
// Desc: WndProc 오버라이딩
//-------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd,
					UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            if( m_bLoadingApp )
            {
                // 로딩중
                HDC hDC = GetDC( hWnd );
                TCHAR strMsg[MAX_PATH];
                wsprintf(strMsg, TEXT("Loading... Please wait"));
                RECT rct;
                GetClientRect( hWnd, &rct );
                DrawText( hDC, strMsg, -1, &rct,
						DT_CENTER|DT_VCENTER|DT_SINGLELINE );
                ReleaseDC( hWnd, hDC );
            }
            break;
        }

    }

    return CD3DApplication::MsgProc( hWnd, msg, wParam, lParam );
}




//-------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: RestoreDeviceObjects() 에서 생성한 오브젝트 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
	// 렌더링타겟
	SAFE_RELEASE(m_pFogMapSurf);
	SAFE_RELEASE(m_pFogMap);
	SAFE_RELEASE(m_pDepthMapSurf);
	SAFE_RELEASE(m_pDepthMap);
	SAFE_RELEASE(m_pColorMapSurf);
	SAFE_RELEASE(m_pColorMap);
	SAFE_RELEASE(m_pMapZ);

	m_pMesh  ->InvalidateDeviceObjects();				// 메시
	m_pMeshBg->InvalidateDeviceObjects();				// 메시
    if(m_pEffect!=NULL) m_pEffect->OnLostDevice();	// 셰이더

    m_pFont->InvalidateDeviceObjects();	// 폰트

    return S_OK;
}




//-------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: InitDeviceObjects() 에서 생성한 오브젝트 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	m_pMesh  ->Destroy();				// 메시
	m_pMeshBg->Destroy();				// 메시
	SAFE_RELEASE( m_pEffect );		// 셰이더
	SAFE_RELEASE( m_pDecl );		// 정점선언

    m_pFont->DeleteDeviceObjects();	// 폰트

	return S_OK;
}




//-------------------------------------------------------------
// Name: FinalCleanup()
// Desc: 종료직전에 호출됨
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    SAFE_DELETE( m_pMesh   );	// 메시
    SAFE_DELETE( m_pMeshBg );	// 메시

    SAFE_DELETE( m_pFont );	// 폰트

    return S_OK;
}




