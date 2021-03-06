//-------------------------------------------------------------
// File: main.cpp
//
// Desc: 볼륨 그림자
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

#define RS   m_pd3dDevice->SetRenderState
#define TSS  m_pd3dDevice->SetTextureStageState
#define SAMP m_pd3dDevice->SetSamplerState

//-------------------------------------------------------------
// 전체화면 렌더 폴리곤
//-------------------------------------------------------------
const DWORD CBigSquare::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;

//-------------------------------------------------------------
// 생성자
//-------------------------------------------------------------
CBigSquare::CBigSquare()
{
	m_pVB=NULL;
}
//-------------------------------------------------------------
// 메모리확보
//-------------------------------------------------------------
HRESULT CBigSquare::Create( LPDIRECT3DDEVICE9 pd3dDevice )
{
	if( FAILED( pd3dDevice->CreateVertexBuffer(
				4*sizeof(SHADOWVERTEX),	// 정점버퍼크기
				D3DUSAGE_WRITEONLY,		// 사용법
				FVF,					// 정점포맷
				D3DPOOL_MANAGED,		// 유효한 메모리 클래스
				&m_pVB,					// 생성된 정점버퍼
				NULL ) ) )				// NULL고정
		return E_FAIL;

	return S_OK;
}
//-------------------------------------------------------------
// 데이터 생성
//-------------------------------------------------------------
void CBigSquare::RestoreDeviceObjects( FLOAT sx, FLOAT sy )
{
	SHADOWVERTEX* v;
	m_pVB->Lock( 0, 0, (void**)&v, 0 );
	v[0].p = D3DXVECTOR4(  0, sy, 0.0f, 1.0f );
	v[1].p = D3DXVECTOR4(  0,  0, 0.0f, 1.0f );
	v[2].p = D3DXVECTOR4( sx, sy, 0.0f, 1.0f );
	v[3].p = D3DXVECTOR4( sx,  0, 0.0f, 1.0f );
	v[0].color = D3DCOLOR_RGBA(0,0,0,0x7f);
	v[1].color = D3DCOLOR_RGBA(0,0,0,0x7f);
	v[2].color = D3DCOLOR_RGBA(0,0,0,0x7f);
	v[3].color = D3DCOLOR_RGBA(0,0,0,0x7f);
	m_pVB->Unlock();
}
//-------------------------------------------------------------
// 메모리 해제
//-------------------------------------------------------------
void CBigSquare::Destroy()
{
	SAFE_RELEASE( m_pVB );
}
//-------------------------------------------------------------
// 화면렌더
//-------------------------------------------------------------
void CBigSquare::Render( LPDIRECT3DDEVICE9 pd3dDevice )
{
	pd3dDevice->SetFVF( FVF );
	pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(SHADOWVERTEX));
	pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
}




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
    m_pBigSquare				= new CBigSquare();
	m_pMeshBG					= new CD3DMesh();				
	m_pMeshBox					= new CD3DMesh();
	m_pShadowBox				= new CShadowVolume();

	m_pEffect					= NULL;
	m_hmWVP						= NULL;
	m_hvPos						= NULL;

	// 선택된 장면 설정
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_d3dEnumeration.AppMinDepthBits = 15;
    m_d3dEnumeration.AppMinStencilBits = 1;

	m_dwCreationWidth           = 500;
    m_dwCreationHeight          = 375;
    m_strWindowTitle            = TEXT( "main" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
	m_bStartFullscreen			= false;
	m_bShowCursorWhenFullscreen	= false;

    // 폰트
    m_pFont                     = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_bLoadingApp               = TRUE;

    ZeroMemory( &m_UserInput, sizeof(m_UserInput) );
    m_fWorldRotX                = 0.0f;
    m_fWorldRotY                = 0.0f;
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
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
	// 셰이더 체크
	if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
		if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
			return E_FAIL;
	
	// 양면스텐실 기능 확인
	if( !( pCaps->StencilCaps & D3DSTENCILCAPS_TWOSIDED ) ) return E_FAIL;
	
	// 스텐실기능을 지원하는지 체크
	if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal
			, pCaps->DeviceType
			, adapterFormat
			, D3DUSAGE_RENDERTARGET
			// 픽셀셰이더후 블렌딩 지원 기능이 있는가?
			| D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
			, D3DRTYPE_SURFACE
			, backBufferFormat ) ) )
		return E_FAIL;

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
	DWORD i;
	
	// 이펙트
	if( FAILED( D3DXCreateEffectFromFile( m_pd3dDevice
								, "hlsl.fx", NULL, NULL 
								, 0, NULL, &m_pEffect, NULL )))
		return E_FAIL;
	m_hmWVP = m_pEffect->GetParameterByName( NULL, "mWVP" );
	m_hvPos = m_pEffect->GetParameterByName( NULL, "vLightPos" );

	// 전체화면렌더 폴리곤 초기화
	if( FAILED( m_pBigSquare->Create( m_pd3dDevice ) )) return E_FAIL;

	// 배경메시 읽기
	if( FAILED( hr = m_pMeshBG->Create( m_pd3dDevice, "CornellNoBox.x" ) ) )
		return DXTRACE_ERR( "Load Mesh", hr );
	for(i=0;i<m_pMeshBG->m_dwNumMaterials;i++){
		m_pMeshBG->m_pMaterials[i].Ambient.r = m_pMeshBG->m_pMaterials[i].Diffuse.r*=2;
		m_pMeshBG->m_pMaterials[i].Ambient.g = m_pMeshBG->m_pMaterials[i].Diffuse.g*=2;
		m_pMeshBG->m_pMaterials[i].Ambient.b = m_pMeshBG->m_pMaterials[i].Diffuse.b*=2;
	}
	// 상자메시 읽기
	if( FAILED( hr = m_pMeshBox->Create( m_pd3dDevice, "box.x" ) ) )
		return DXTRACE_ERR( "Load Mesh", hr );
	for(i=0;i<m_pMeshBox->m_dwNumMaterials;i++){
		m_pMeshBox->m_pMaterials[i].Ambient.r = m_pMeshBox->m_pMaterials[i].Diffuse.r*=1.13f;
		m_pMeshBox->m_pMaterials[i].Ambient.g = m_pMeshBox->m_pMaterials[i].Diffuse.g*=0.93f;
		m_pMeshBox->m_pMaterials[i].Ambient.b = m_pMeshBox->m_pMaterials[i].Diffuse.b*=0.53f;
	}
	
	// 그림자볼륨 생성
	m_pShadowBox->Create( m_pMeshBox->GetSysMemMesh() );

    // 폰트 초기화
    m_pFont->InitDeviceObjects( m_pd3dDevice );

	return S_OK;
}




//-------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: 화면크기가 변했을때 호출됨
//       확보한 메모리는 InvalidateDeviceObjects()에서 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	m_LighPos = D3DXVECTOR3(0.0f, 5.488f, 2.770f);

    // 재질설정
    D3DMATERIAL9 mtrl;
    D3DUtil_InitMaterial( mtrl, 1.0f, 0.0f, 0.0f );
    m_pd3dDevice->SetMaterial( &mtrl );

    // 텍스처 설정
    TSS( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    TSS( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    TSS( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    TSS( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    TSS( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    TSS( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    SAMP( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    SAMP( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    // 렌더링 상태설정
    RS( D3DRS_DITHERENABLE,   FALSE );
    RS( D3DRS_SPECULARENABLE, FALSE );
    RS( D3DRS_ZENABLE,        TRUE );
    RS( D3DRS_AMBIENT,        0x000F0F0F );

    // 월드행렬
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );
    m_pd3dDevice->SetTransform( D3DTS_WORLD,  &matIdentity );

	// 뷰행렬
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -5.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_mView, &vFromPt, &vLookatPt, &vUpVec );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &m_mView );

    // 투영행렬
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    D3DXMatrixPerspectiveFovLH( &m_mProj, 0.21f*D3DX_PI, fAspect, 1.0f, 100.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &m_mProj );

	// 광원설정
    D3DLIGHT9 light;
    D3DUtil_InitLight( light, D3DLIGHT_DIRECTIONAL, -0.0f, -1.0f, 0.2f );
    light.Diffuse.r   = 0.5f;
    light.Diffuse.g   = 0.5f;
    light.Diffuse.b   = 0.5f;
    light.Ambient.r   = 0.5f;
    light.Ambient.g   = 0.5f;
    light.Ambient.b   = 0.5f;
    m_pd3dDevice->SetLight( 0, &light );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

	// 사각형
	m_pBigSquare->RestoreDeviceObjects( (FLOAT)m_d3dsdBackBuffer.Width,
										(FLOAT)m_d3dsdBackBuffer.Height );
	// 메시
	m_pMeshBG->RestoreDeviceObjects(m_pd3dDevice);
	m_pMeshBox->RestoreDeviceObjects(m_pd3dDevice);

	// 이펙트
	if( m_pEffect != NULL ) m_pEffect->OnResetDevice();

    m_pFont->RestoreDeviceObjects();	// 폰트
	
    return S_OK;
}




//-------------------------------------------------------------
// Name: FrameMove()
// Desc: 매 프레임마다 호출됨. 애니메이션 처리등 담당
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    UpdateInput( &m_UserInput );// 볺쀍긢�[�^궻뛛륷

	//---------------------------------------------------------
	// 입력에 따라 좌표계를 갱신한다
	//---------------------------------------------------------
	// 회전
    D3DXMATRIX m;
    D3DXMATRIX matRotY;
    D3DXMATRIX matRotX;

    if( m_UserInput.bRotateLeft && !m_UserInput.bRotateRight )
        m_fWorldRotY += m_fElapsedTime;
    else if( m_UserInput.bRotateRight && !m_UserInput.bRotateLeft )
        m_fWorldRotY -= m_fElapsedTime;

    if( m_UserInput.bRotateUp && !m_UserInput.bRotateDown )
        m_fWorldRotX += m_fElapsedTime;
    else if( m_UserInput.bRotateDown && !m_UserInput.bRotateUp )
        m_fWorldRotX -= m_fElapsedTime;

	//---------------------------------------------------------
	// 행렬 갱신
	//---------------------------------------------------------
	// 월드 회전
    D3DXMatrixRotationX( &matRotX, m_fWorldRotX );
    D3DXMatrixRotationY( &matRotY, m_fWorldRotY );
    D3DXMatrixMultiply( &m, &matRotX, &matRotY );

    // 뷰행렬
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 2.73f, -8.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 2.73f, 0.0f );
    D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &m_mView, &vFromPt, &vLookatPt, &vUpVec );
	m_mView = m * m_mView;
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &m_mView );

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
}




//-------------------------------------------------------------
// Name: Render()
// Desc: 화면 렌더
//-------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
	D3DXMATRIX m, mW, mS, mR, mT;
	D3DXVECTOR4 v;

	
	// 화면클리어
    m_pd3dDevice->Clear( 0L, NULL
			, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL
			//   색     깊이  스텐실
			, 0xffffff, 1.0f, 0L );

    // 렌더 시작
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) ) {

		// ----------------------------------------------------
		// 준비:그림자 없는부분 렌더
		// ----------------------------------------------------
		D3DXMatrixIdentity( &m );
		m_pd3dDevice->SetTransform( D3DTS_WORLD,  &m );
		m_pMeshBG->Render( m_pd3dDevice );

		// 작은 상자
		D3DXMatrixScaling( &mS, 1.82f,1.65f, 1.82f );
		D3DXMatrixRotationY( &mR, 0.59f*D3DX_PI );
		D3DXMatrixTranslation( &mT, 2.73f-1.85f, 0.f , 1.69f );
		m = mS * mR * mT;
		m_pd3dDevice->SetTransform( D3DTS_WORLD,  &m );
		m_pMeshBox->Render( m_pd3dDevice );

		// 큰 상자
		D3DXMatrixScaling( &mS, 1.69f, 3.30f, 1.69f );
		D3DXMatrixRotationY( &mR, 0.91f*D3DX_PI );
		D3DXMatrixTranslation( &mT, 2.73f-3.685f, 0, 3.51f );
		m = mS * mR * mT;
		m_pd3dDevice->SetTransform( D3DTS_WORLD,  &m );
		m_pMeshBox->Render( m_pd3dDevice );

		// ----------------------------------------------------
		// 패스2:그림자 볼륨 렌더
		// ----------------------------------------------------
		// 깊이버퍼에 쓰기금지
		RS( D3DRS_ZWRITEENABLE,  FALSE );
		// 렌더링타겟에 쓰기금지
		RS( D3DRS_COLORWRITEENABLE,  FALSE );
		// 플랫셰이딩
		RS( D3DRS_SHADEMODE,	 D3DSHADE_FLAT );
		// 양면렌더
		RS( D3DRS_CULLMODE,  D3DCULL_NONE );

		// 양면스텐실 사용
		RS( D3DRS_STENCILENABLE, TRUE );
		RS( D3DRS_TWOSIDEDSTENCILMODE, TRUE );

		// 스텐실테스트는 기본적으로 합격(테스트 하지 않음)
		RS( D3DRS_STENCILFUNC,  D3DCMP_ALWAYS );
		RS( D3DRS_CCW_STENCILFUNC,  D3DCMP_ALWAYS );
		// 스텐실버퍼의 증감값을 1로 설정
		RS( D3DRS_STENCILREF,	   0x1 );
		RS( D3DRS_STENCILMASK,	  0xffffffff );
		RS( D3DRS_STENCILWRITEMASK, 0xffffffff );
		// 앞면이 깊이테스트에 합격하면 스텐실버퍼의 내용을 +1
		RS( D3DRS_STENCILPASS,  D3DSTENCILOP_INCR );
		RS( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
		RS( D3DRS_STENCILFAIL,  D3DSTENCILOP_KEEP );
		// 뒷면이 깊이테스트에 합격하면 스텐실버퍼의 내용을 -1
		RS( D3DRS_CCW_STENCILPASS, D3DSTENCILOP_DECR );
		RS( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP );
		RS( D3DRS_CCW_STENCILFAIL,  D3DSTENCILOP_KEEP );

		// 렌더한다
		if( m_pEffect != NULL ){
			D3DXHANDLE hTechnique = m_pEffect->GetTechniqueByName( "TShader" );
			m_pEffect->SetTechnique( hTechnique );
			m_pEffect->Begin( NULL, 0 );
			m_pEffect->Pass( 0 );

			// 작은 상자
			D3DXMatrixScaling( &mS, 1.82f,1.65f, 1.82f );
			D3DXMatrixRotationY( &mR, 0.59f*D3DX_PI );
			D3DXMatrixTranslation( &mT, 2.73f-1.85f, 0.f , 1.69f );
			mW = mS * mR * mT;
			m = mW * m_mView * m_mProj;
			if( m_hmWVP != NULL ) m_pEffect->SetMatrix( m_hmWVP, &m );
			D3DXMatrixInverse( &m, NULL, &mW);
			D3DXVec3Transform( &v, &m_LighPos, &m );
			if( m_hvPos != NULL ) m_pEffect->SetVector( m_hvPos, &v );
			m_pShadowBox->Render( m_pd3dDevice );

			// 큰 상자
			D3DXMatrixScaling( &mS, 1.69f, 3.30f, 1.69f );
			D3DXMatrixRotationY( &mR, 0.91f*D3DX_PI );
			D3DXMatrixTranslation( &mT, 2.73f-3.685f, 0, 3.51f );
			mW = mS * mR * mT;
			m = mW * m_mView * m_mProj;
			if( m_hmWVP != NULL ) m_pEffect->SetMatrix( m_hmWVP, &m );
			D3DXMatrixInverse( &m, NULL, &mW);
			D3DXVec3Transform( &v, &m_LighPos, &m );
			if( m_hvPos != NULL ) m_pEffect->SetVector( m_hvPos, &v );
			m_pShadowBox->Render( m_pd3dDevice );

			m_pEffect->End();
		}

		// 렌더상태 원상복구
		RS( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
		RS( D3DRS_CULLMODE,  D3DCULL_CCW );
		RS( D3DRS_ZWRITEENABLE,	 TRUE );
		RS( D3DRS_COLORWRITEENABLE,  0xf );
		RS( D3DRS_STENCILENABLE,	FALSE );
		RS( D3DRS_ALPHABLENDENABLE, FALSE );
		RS( D3DRS_TWOSIDEDSTENCILMODE, FALSE );

		// ----------------------------------------------------
		// 패스3:그림자 렌더
		// ----------------------------------------------------
		// 깊이테스트 금지
		RS( D3DRS_ZENABLE,		  FALSE );
		// 스텐실테스트 사용
		RS( D3DRS_STENCILENABLE,	TRUE );
		// 알파블렌딩은 선형합성
		RS( D3DRS_ALPHABLENDENABLE, TRUE );
		RS( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
		RS( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		// 폴리곤을 렌더링할때 텍스처와 정점색을 모두 사용
		TSS( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		TSS( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		TSS( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
		TSS( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );



		// 스텐실버퍼의 값이 1이상인 경우에만 그린다
		RS( D3DRS_STENCILREF,  0x1 );
		RS( D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL );
		RS( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
		
		m_pBigSquare->Render( m_pd3dDevice );

		// 렌더상태 원상복구
		RS( D3DRS_ZENABLE,		  TRUE );
		RS( D3DRS_STENCILENABLE,	FALSE );
		RS( D3DRS_ALPHABLENDENABLE, FALSE );

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

    // 디스플레이의 상태출력
    FLOAT fNextLine = 40.0f; 

    lstrcpy( szMsg, m_strDeviceStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, m_strFrameStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    // 조작방법
    fNextLine = (FLOAT) m_d3dsdBackBuffer.Height; 
    wsprintf( szMsg, TEXT("Arrow keys: Up=%d Down=%d Left=%d Right=%d"), 
              m_UserInput.bRotateUp, m_UserInput.bRotateDown, m_UserInput.bRotateLeft, m_UserInput.bRotateRight );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Use arrow keys to rotate object") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    lstrcpy( szMsg, TEXT("Press 'F2' to configure display") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    return S_OK;
}




//-------------------------------------------------------------
// Name: MsgProc()
// Desc: WndProc 오버라이딩
//-------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            if( m_bLoadingApp )
            {
                // 로드중
                HDC hDC = GetDC( hWnd );
                TCHAR strMsg[MAX_PATH];
                wsprintf( strMsg, TEXT("Loading... Please wait") );
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
	// 이펙트
	if( m_pEffect != NULL ) m_pEffect->OnLostDevice();
	// 메시
	m_pMeshBG->InvalidateDeviceObjects();
	m_pMeshBox->InvalidateDeviceObjects();
	// 폰트
    m_pFont->InvalidateDeviceObjects();

	return S_OK;
}




//-------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: InitDeviceObjects() 에서 생성한 오브젝트 해제
//-------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	// 이펙트
	SAFE_RELEASE( m_pEffect );
	// 메시
	m_pMeshBG->Destroy();
	m_pMeshBox->Destroy();
	m_pShadowBox->Destroy();

	m_pBigSquare->Destroy();
	// 폰트
    m_pFont->DeleteDeviceObjects();

    return S_OK;
}




//-------------------------------------------------------------
// Name: FinalCleanup()
// Desc: 종료직전에 호출됨
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
	SAFE_DELETE( m_pShadowBox );
	SAFE_DELETE( m_pMeshBox );
	SAFE_DELETE( m_pMeshBG );
	SAFE_DELETE( m_pBigSquare );

    SAFE_DELETE( m_pFont );

    return S_OK;
}




