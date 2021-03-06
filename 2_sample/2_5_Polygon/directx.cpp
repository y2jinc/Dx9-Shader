#include <d3d9.h>							// DirectX を使えるようにする
#include "stdafx.h"
#include "draw.h"							 // ﾆ�ｸｮｰ� ｷｻｴ�ﾇﾔｼ� ﾈ｣ﾃ�

// グローバル変数 :
LPDIRECT3D9				g_pD3D    = NULL;	// Direct 3Dｿ｡ ﾁ｢ｱﾙ
LPDIRECT3DDEVICE9		g_pD3DDev = NULL;	// ｺ�ｵ�ｿﾀﾄｫｵ蠢｡ ﾁ｢ｱﾙ

//-----------------------------------------------------------------------------
// Direct X Graphics ﾃﾊｱ篳ｭ
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Direct 3D ｻ鄙�
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) ) return E_FAIL;

    // ｵ�ｽｺﾇﾃｷｹﾀﾌ ｸ�ｵ� ﾁｶｻ�
    D3DDISPLAYMODE d3ddm;
    if( FAILED( g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) )
		return E_FAIL;

    // Direct 3D ｵ�ｹﾙﾀﾌｽｺ ｻ�ｼｺ
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );        // ｸ�ｶ･ 0ﾀｸｷﾎ
    d3dpp.Windowed = TRUE;                      // ﾀｩｵｵｿ�ｸ�ｵ�
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;   // ﾈｭｸ鯊�ﾈｯ ｹ貉�
    d3dpp.BackBufferFormat = d3ddm.Format;      // ﾇ�ﾀ� ﾈｭｸ� ﾆ�ｸﾋ ｻ鄙�

    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &g_pD3DDev ) ) ) return E_FAIL;
	
	if(FAILED(Initialize(g_pD3DDev))) return E_FAIL; // アプリケーションの初期化

	return S_OK;
}
//-----------------------------------------------------------------------------
// ｷｻｴ�ﾇﾔｼ�
//-----------------------------------------------------------------------------
VOID Render()
{
    // ｹ隹貘ｻ ｰﾋｰﾔﾄ･ﾇﾑｴﾙ
    g_pD3DDev->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
    
    g_pD3DDev->BeginScene(); // ｷｻｴ� ｽﾃﾀﾛ
    
	Update(g_pD3DDev);			// ｷｻｴ�
    
    g_pD3DDev->EndScene(); // ｷｻｴ� ﾁｾｷ�
    
    // ﾃ箙ﾂﾇﾑ ｳｻｿ�ﾀｻ ｽﾇﾁｦ ﾀｩｵｵｿ�ｿ｡ ｳｪﾅｸｳｪｰﾔ ﾇﾑｴﾙ
    g_pD3DDev->Present( NULL, NULL, NULL, NULL );
}
//-----------------------------------------------------------------------------
// 後片付け
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    Close(g_pD3DDev);			// ｾ�ﾇﾃｸｮﾄﾉﾀﾌｼﾇ ﾁｾｷ�

	RELEASE(g_pD3DDev);
    RELEASE(g_pD3D);
}