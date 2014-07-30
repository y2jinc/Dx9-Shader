//-------------------------------------------------------------
// File: main.cpp
//
// Desc: X����
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
// ��������
//-------------------------------------------------------------
CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;


//-------------------------------------------------------------
// Name: WinMain()
// Desc: �����Լ�
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
// Desc: ���ø����̼� ������
//-------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_pMesh                     = new CD3DMesh();// ���߰�

    m_dwCreationWidth           = 300;
    m_dwCreationHeight          = 300;
    m_strWindowTitle            = TEXT( "main" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
    m_bStartFullscreen          = false;
    m_bShowCursorWhenFullscreen = false;

    m_pFont                     = new CD3DFont( _T("Arial"), 12, D3DFONT_BOLD );
    m_bLoadingApp               = TRUE;
}




//-------------------------------------------------------------
// Name: ~CMyD3DApplication()
// Desc: �Ҹ���
//-------------------------------------------------------------
CMyD3DApplication::~CMyD3DApplication()
{

}




//-------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: �� �ѹ��� �ʱ�ȭ
//        ������ �ʱ�ȭ�� IDirect3D9�ʱ�ȭ�� ������
//        �׷��� LPDIRECT3DDEVICE9�ʱ�ȭ�� ������ ���� ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // �ε� �޽��� ���
    SendMessage( m_hWnd, WM_PAINT, 0, 0 );

    m_bLoadingApp = FALSE;

    return S_OK;
}




//-------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: �ʱ�ȭ�� ȣ���. �ʿ��� �ɷ�(caps)üũ
//-------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps,
                     DWORD dwBehavior,    D3DFORMAT Format )
{
    UNREFERENCED_PARAMETER( Format );
    UNREFERENCED_PARAMETER( dwBehavior );
    UNREFERENCED_PARAMETER( pCaps );
    
    BOOL bCapsAcceptable;

    // �׷��Ⱥ��尡 ���α׷��� ������ �ɷ��� �ִ°� Ȯ��
    bCapsAcceptable = TRUE;

    if( bCapsAcceptable )         
        return S_OK;
    else
        return E_FAIL;
}




//-------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: ����̽��� ���������� �ʱ�ȭ
//        �����ӹ��� ���˰� ����̽� ������ ���ѵڿ� ȣ��
//        ���⼭ Ȯ���� �޸𸮴� DeleteDeviceObjects()���� ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
	HRESULT hr;

	// �ڡڡ���ī:X���� �б�
	if(FAILED(hr=m_pMesh->Create( m_pd3dDevice, _T("map.x"))))
        return DXTRACE_ERR( "Load", hr );

	// ��Ʈ
    m_pFont->InitDeviceObjects( m_pd3dDevice );

    return S_OK;
}

//-------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: ȭ��ũ�Ⱑ �������� ȣ���
//        Ȯ���� �޸𸮴� InvalidateDeviceObjects()���� ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
	m_pMesh->RestoreDeviceObjects( m_pd3dDevice );// �ڡڡ��߰�

    // ���� ��ũ��
    #define RS   m_pd3dDevice->SetRenderState

    // ������ ���� ����
    RS( D3DRS_DITHERENABLE,   FALSE );
    RS( D3DRS_SPECULARENABLE, FALSE );
    RS( D3DRS_ZENABLE,        TRUE );
    RS( D3DRS_LIGHTING,        FALSE );

    // ��Ʈ
    m_pFont->RestoreDeviceObjects();

    return S_OK;
}




//-------------------------------------------------------------
// Name: FrameMove()
// Desc: �� �����Ӹ��� ȣ���. �ִϸ��̼� ó���� ���
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    return S_OK;
}




//-------------------------------------------------------------
// Name: Render()
// Desc: ȭ�� ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
    D3DXMATRIXA16 mWorld, mView, mProj;

    // ȭ�� Ŭ����
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         D3DCOLOR_RGBA(0x00,0x00,0x00,0x00), 1.0f, 0L );

    //---------------------------------------------------------
    // ����
    //---------------------------------------------------------
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // ---------------------------------------------------------
        // ������� ����
        // ---------------------------------------------------------
		D3DXMATRIX mWorld;
		D3DXMatrixIdentity( &mWorld );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, &mWorld );

        // ---------------------------------------------------------
        // ����� ����
        // ---------------------------------------------------------
        D3DXVECTOR3 vEyePt   ( 0.0f, 5.0f,-10.0f ); // �ָ���
        D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f ); // ī�޶���ġ
        D3DXVECTOR3 vUp      ( 0.0f, 1.0f, 0.0f ); // ��溤��
        D3DXMatrixLookAtLH( &mView, &vEyePt, &vLookatPt, &vUp );
        m_pd3dDevice->SetTransform( D3DTS_VIEW, &mView );

        // ---------------------------------------------------------
        // ������� ����
        // ---------------------------------------------------------
        D3DXMatrixPerspectiveFovLH( &mProj
                            , D3DX_PI/4      // �þ߰�
                            , 1.0f           // ȭ�� ��Ⱦ��
                            , 1.0f, 100.0f );// ����,�Ĺ�Ŭ����
        m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &mProj );

        // ---------------------------------------------------------
        // �ڡڡ��߰�:�� ����
        // ---------------------------------------------------------
	    #define TSS  m_pd3dDevice->SetTextureStageState
	    #define SAMP m_pd3dDevice->SetSamplerState
		TSS( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		TSS( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		SAMP( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		SAMP( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

		m_pMesh->Render( m_pd3dDevice );

        RenderText();    // ȭ����¿� ���� ���

        // ��������
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-------------------------------------------------------------
// Name: RenderText()
// Desc: ���¿� ������ ȭ�鿡 ���
//-------------------------------------------------------------
HRESULT CMyD3DApplication::RenderText()
{
    D3DCOLOR fontColor        = D3DCOLOR_ARGB(255,255,255,0);
    TCHAR szMsg[MAX_PATH] = TEXT("");

    // ȭ�����
    FLOAT fNextLine = 40.0f; 

    lstrcpy( szMsg, m_strDeviceStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    lstrcpy( szMsg, m_strFrameStats );
    fNextLine -= 20.0f;
    m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );

    // ���۹� & �Ķ���� ���
    fNextLine = (FLOAT) m_d3dsdBackBuffer.Height; 
    lstrcpy( szMsg, TEXT("Press 'F2' to configure display") );
    fNextLine -= 20.0f; m_pFont->DrawText( 2, fNextLine, fontColor, szMsg );
    return S_OK;
}




//-------------------------------------------------------------
// Name: MsgProc()
// Desc: WndProc �������̵�
//-------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_PAINT:
        {
            if( m_bLoadingApp )
            {
                // �ε���
                HDC hDC = GetDC( hWnd );
                TCHAR strMsg[MAX_PATH];
                wsprintf(strMsg, TEXT("Loading... Please wait"));
                RECT rct;
                GetClientRect( hWnd, &rct );
                DrawText( hDC, strMsg, -1, &rct
                        , DT_CENTER|DT_VCENTER|DT_SINGLELINE );
                ReleaseDC( hWnd, hDC );
            }
            break;
        }

    }

    return CD3DApplication::MsgProc( hWnd, msg, wParam, lParam );
}




//-------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: RestoreDeviceObjects() ���� ������ ������Ʈ ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
	m_pMesh->InvalidateDeviceObjects();	// �ڡڡ��߰�

    m_pFont->InvalidateDeviceObjects();    // ��Ʈ

    return S_OK;
}




//-------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: InitDeviceObjects() ���� ������ ������Ʈ ����
//-------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	m_pMesh->Destroy();	// �ڡڡ��߰�

	// ��Ʈ
    m_pFont->DeleteDeviceObjects();

    return S_OK;
}




//-------------------------------------------------------------
// Name: FinalCleanup()
// Desc: ���������� ȣ���
//-------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
	SAFE_DELETE( m_pMesh );// �ڡڡ��߰�

	SAFE_DELETE( m_pFont );    // ��Ʈ

    return S_OK;
}



