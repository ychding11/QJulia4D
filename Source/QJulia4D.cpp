
// Coded by Jan Vlietinck, 11 Oct 2009, V 1.4
// http://users.skynet.be/fquake/


#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#if D3D_COMPILER_VERSION < 46
#include <d3dx11.h>
#endif

#include "resource.h"
#include "stdio.h"
#include "timer.h"
#include "math.h"


#include "Algebra.h"
#include "TrackBall.h"

TrackBall trackBall;


static void CreateComputeShaderObject();
static void CreateGraphicShaderObject();

////////////////////////////////////////////////////////////////////////////////



static float Epsilon                    = 0.003f;
static float ColorT                     = 0.0f;
static float ColorA[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };
static float ColorB[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };
static float ColorC[4]                  = { 0.25f, 0.45f, 1.0f, 1.0f };

static float MuT                        = 0.0f;
static float MuA[4]                     = { -.278f, -.479f, 0.0f, 0.0f };
static float MuB[4]                     = { 0.278f, 0.479f, 0.0f, 0.0f };
static float MuC[4]                     = { -.278f, -.479f, -.231f, .235f };

////////////////////////////////////////////////////////////////////////////////




//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

bool animated = true;
bool vsync = false; // redraw synced with screen refresh

WCHAR message[256];
float timer=0;

Timer time;
int fps=0;
float zoom = 1;

typedef struct
{
  float diffuse[4]; // diffuse shading color
  float mu[4];    // quaternion julia parameter
  float epsilon;  // detail julia
  int c_width;      // view port size
  int c_height;
  int selfShadow;  // selfshadowing on or off 
  float orientation[4*4]; // rotation matri
  float zoom;
} QJulia4DConstants;

int ROWS, COLS;
#define HCOLS (COLS/2)
#define HROWS (ROWS/2)

bool  selfShadow = false;
int g_width, g_height; // size of window

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

HINSTANCE                           g_hInst = NULL;
HWND                                g_hWnd = NULL;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
ID3D11Device*                       g_pd3dDevice = NULL;
ID3D11DeviceContext*                g_pImmediateContext = NULL ;

IDXGISwapChain*             g_pSwapChain = NULL;
ID3D11RenderTargetView*     g_pRenderTargetView = NULL;
ID3D11ComputeShader*        g_pCS_QJulia4D;
ID3D11Buffer*		    	g_pcbFractal;      // constant buffer
ID3D11UnorderedAccessView*  g_pComputeOutput;  // compute output
  

bool hasComputeShaders; // in case of dx10 feature level use pixel shader instead of compute shader
bool useComputeShaders;  // can be toggled with p key, if off uses pixel shaders

ID3D11VertexShader*         g_pVS10;
ID3D11PixelShader*          g_pPS10;
ID3D11RasterizerState*      g_pRasterizerState;
ID3D11DepthStencilState*	  g_pDepthState;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, unsigned int, WPARAM, LPARAM );
void Render();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
  COLS = (LONG)::GetSystemMetrics( SM_CXSCREEN );
  ROWS = (LONG)::GetSystemMetrics( SM_CYSCREEN );
  SetCursorPos(HCOLS, HROWS); // set mouse to middle screen

  if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
      return 0;
  if( FAILED( InitDevice() ) )
  {
      CleanupDevice();
      return 0;
  }

  // Main message loop
  MSG msg = {0};
  while( WM_QUIT != msg.message )
  {
      if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
      {
          TranslateMessage( &msg );
          DispatchMessage( &msg );
      }
      else
      {
          Render();
      }
  }
  CleanupDevice();
  return ( int )msg.wParam;
}

//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_MAIN_ICON );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"QJulia4DWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_MAIN_ICON );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, COLS, ROWS };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"QJulia4DWindowClass", L"QJulia4D DX11", WS_OVERLAPPEDWINDOW, 0, 0, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL );

    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
    return S_OK;
}


//***********************************************************************************
void Resize()
//***********************************************************************************
{
  if ( g_pd3dDevice == NULL)
    return;

  HRESULT hr = S_OK;

  RECT rc;
  GetClientRect( g_hWnd, &rc );
  unsigned int width = rc.right - rc.left;
  unsigned int height = rc.bottom - rc.top;

  g_height = height;
  g_width  = width;

  // release references to back buffer before resize, else fails
  SAFE_RELEASE(g_pComputeOutput);
  SAFE_RELEASE(g_pRenderTargetView);

  
  DXGI_SWAP_CHAIN_DESC sd;
  g_pSwapChain->GetDesc(&sd);

  // put vsync on for full screen, else tearing
  vsync = !sd.Windowed;

  hr = g_pSwapChain->ResizeBuffers(sd.BufferCount, width, height, sd.BufferDesc.Format, 0);

  ID3D11Texture2D* pTexture;
  hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pTexture );


  if (hasComputeShaders)
  {
    hr = g_pd3dDevice->CreateUnorderedAccessView( pTexture, NULL, &g_pComputeOutput );
  }

  {
    hr = g_pd3dDevice->CreateRenderTargetView( pTexture, NULL, &g_pRenderTargetView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width  = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );
  }

  pTexture->Release();
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;
    
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    unsigned int width = rc.right - rc.left;
    unsigned int height = rc.bottom - rc.top;

    unsigned int createDeviceFlags = 0;
    #ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
       #ifdef WARP
             D3D_DRIVER_TYPE_REFERENCE,
       #else
             D3D_DRIVER_TYPE_HARDWARE,        
       #endif
    };
    unsigned int numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_SHADER_INPUT;
    sd.OutputWindow = g_hWnd;
    sd.Windowed = TRUE;
    //sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // try dx 11
    D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;
    hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, &FeatureLevel,1,  D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pImmediateContext );
	if (hr == S_OK)
	{
		hasComputeShaders = true;
		printf("- Create DX11 device and swap chain successful.\n");
	}
    else
    {
      // try dx 10
      FeatureLevel = D3D_FEATURE_LEVEL_10_0;
      sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, &FeatureLevel,1,  D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pImmediateContext );
      if (hr == S_OK)
        hasComputeShaders = false;
    }
    useComputeShaders = hasComputeShaders;
	if (FAILED(hr))
	{
		printf("- Create DX device and swap chain failed,exit.\n");
		return hr;
	}

    // Create compute shader
    DWORD dwShaderFlags = 0;
    #if defined( DEBUG ) || defined( _DEBUG )
      dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif

    if (hasComputeShaders)
    { CreateComputeShaderObject(); }
    CreateGraphicShaderObject();

    // Create constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = ((sizeof( QJulia4DConstants ) + 15)/16)*16; // must be multiple of 16 bytes
    g_pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbFractal);
 
    Resize();
    srand((unsigned int)GetCurrentTime());
    return S_OK;
}

static void CreateGraphicShaderObject()
{
    HRESULT hr = S_OK;
    ID3D10Blob* pVSBuf = NULL;
    ID3DBlob* pErrorBlob = nullptr;
    ID3DBlob* pBlob = nullptr;
    
    // Create compute shader
    DWORD dwShaderFlags = 0;
    #if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif
    
    #if D3D_COMPILER_VERSION >= 46
    hr = D3DCompileFromFile(L"qjulia4D.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_4_0", dwShaderFlags, 0, &pBlob, &pErrorBlob);
    #else
    hr = D3DX11CompileFromFile( L"qjulia4D.hlsl", NULL, NULL, "VS", "vs_4_0", dwShaderFlags, 0, NULL, &pBlob, &pErrorBlob, NULL );
    #endif
    if (FAILED(hr))
    {
        if (pErrorBlob) OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        SAFE_RELEASE(pBlob);
        exit(1);
    }
    else
    {
        printf("- Compiling vertex shader OK.\n");
    }
    hr = g_pd3dDevice->CreateVertexShader( ( DWORD* )pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pVS10 );
    SAFE_RELEASE(pBlob);
    
    #if D3D_COMPILER_VERSION >= 46
    hr = D3DCompileFromFile(L"qjulia4D.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_4_0", dwShaderFlags, 0, &pBlob, &pErrorBlob);
    #else
    hr = D3DX11CompileFromFile( L"qjulia4D.hlsl", NULL, NULL, "PS", "ps_4_0", dwShaderFlags, 0, NULL, &pBlob, &pErrorBlob, NULL );
    #endif
    if (FAILED(hr))
    {
        if (pErrorBlob) OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        SAFE_RELEASE(pBlob);
        exit(1);
    }
    else
    {
        printf("- Compile pixel shader OK.\n");
    }
    hr = g_pd3dDevice->CreatePixelShader( ( DWORD* )pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pPS10 );
    SAFE_RELEASE(pErrorBlob);
    SAFE_RELEASE(pBlob);

    D3D11_RASTERIZER_DESC RSDesc;
    RSDesc.FillMode = D3D11_FILL_SOLID;
    RSDesc.CullMode = D3D11_CULL_NONE;
    RSDesc.FrontCounterClockwise = FALSE;
    RSDesc.DepthBias = 0;
    RSDesc.DepthBiasClamp = 0;
    RSDesc.SlopeScaledDepthBias = 0;
    RSDesc.ScissorEnable = FALSE;
    RSDesc.MultisampleEnable = TRUE;
    RSDesc.AntialiasedLineEnable = FALSE;
    g_pd3dDevice->CreateRasterizerState( &RSDesc, &g_pRasterizerState );

    D3D11_DEPTH_STENCIL_DESC DSDesc;
    ZeroMemory( &DSDesc, sizeof( D3D11_DEPTH_STENCIL_DESC ) );
    DSDesc.DepthEnable = FALSE;
    DSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    g_pd3dDevice->CreateDepthStencilState( &DSDesc, &g_pDepthState );
}

static void CreateComputeShaderObject()
{
    HRESULT hr = S_OK;
    ID3DBlob* pErrorBlob = nullptr;
    ID3DBlob* pBlob = nullptr;
    
    // Create compute shader
    DWORD dwShaderFlags = 0;
    #if defined( DEBUG ) || defined( _DEBUG )
      dwShaderFlags |= D3D10_SHADER_DEBUG;
    #endif
    
    #if D3D_COMPILER_VERSION >= 46
    hr = D3DCompileFromFile(L"qjulia4D.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CS_QJulia4D", "cs_5_0",  dwShaderFlags, 0, &pBlob, &pErrorBlob);
    #else
    hr = D3DX11CompileFromFile( L"qjulia4D.hlsl", NULL, NULL, "CS_QJulia4D", "cs_5_0", dwShaderFlags, 0, NULL, &pBlob, &pErrorBlob, NULL );
    #endif

    if (FAILED(hr))
    {
        if (pErrorBlob) OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
        SAFE_RELEASE(pErrorBlob);
        SAFE_RELEASE(pBlob);
    }
    else
    {
       printf("- Compile compute shader OK.\n");
    }
    hr = g_pd3dDevice->CreateComputeShader( ( DWORD* )pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &g_pCS_QJulia4D );
    SAFE_RELEASE(pBlob);
    SAFE_RELEASE(pErrorBlob);
}

//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    SAFE_RELEASE( g_pCS_QJulia4D );
    SAFE_RELEASE(g_pComputeOutput);
    SAFE_RELEASE(g_pcbFractal);

    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

    SAFE_RELEASE(g_pComputeOutput);
    SAFE_RELEASE(g_pRenderTargetView);

    if( g_pSwapChain )
    {
      g_pSwapChain->SetFullscreenState(false, NULL); // switch back to full screen else not working ok
      g_pSwapChain->Release();
    }

    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam )
{
    const float fStepSize = 0.05f;
    static bool lbdown = false;

    PAINTSTRUCT ps;
    HDC hdc;

  // Consolidate the mouse button messages and pass them to the right volume renderer
  if( msg == WM_LBUTTONDOWN ||
      msg == WM_LBUTTONUP ||
      msg == WM_LBUTTONDBLCLK ||
      msg == WM_MBUTTONDOWN ||
      msg == WM_MBUTTONUP ||
      msg == WM_MBUTTONDBLCLK ||
      msg == WM_RBUTTONDOWN ||
      msg == WM_RBUTTONUP ||
      msg == WM_RBUTTONDBLCLK ||
      msg == WM_MOUSEWHEEL || 
      msg == WM_MOUSEMOVE )
  {
      int xPos = (short)LOWORD(lParam);
      int yPos = (short)HIWORD(lParam);
      int nMouseWheelDelta = 0;

      if( msg == WM_MOUSEWHEEL ) 
      {
        nMouseWheelDelta = (short) HIWORD(wParam);
        if (nMouseWheelDelta < 0)
          zoom = zoom * 1.1;
        else
          zoom = zoom / 1.1;
      }
      if (msg == WM_LBUTTONDOWN && !lbdown)
      {
          trackBall.DragStart(xPos, yPos, g_width, g_height);
          lbdown = true;
      }
      if (msg == WM_MOUSEMOVE && lbdown)
      {
        trackBall.DragMove(xPos, yPos, g_width, g_height);
      }
      if (msg == WM_LBUTTONUP)
      {
        trackBall.DragEnd();
        lbdown = false;
      }
  }
  else switch( msg)
  {
      case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;
      case WM_CREATE:
        break;
      case WM_CLOSE:
      {
            DestroyWindow( hWnd );
            UnregisterClass( L"QJulia4DWindowClass", NULL );
            return 0;
      }
      case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
      case WM_SIZE:          
          if ( wParam != SIZE_MINIMIZED )
            Resize();
        break;
      case WM_KEYDOWN:
      {
          char key = tolower((int)wParam);
          switch (key)
          {
            case 'v':
            {
              vsync = !vsync;
              break;
            }
            case 'p':  // toggle between compute and pixel shaders (if has cs)
            {
              useComputeShaders = (hasComputeShaders) ? !useComputeShaders : false;
              break;
            }
            case 's':  // toggle selfshadow
            {
              selfShadow = !selfShadow;
              break;
            }
            case '+':
            case '=':
               if(Epsilon >= 0.002f)
                   Epsilon *= (1.0f / 1.05f);
               break;
            case '-':
               if(Epsilon < 0.01f)
                   Epsilon *= 1.05f;
               break;
            case 'w':
               MuC[0] += fStepSize; 
               break;
            case 'x':
               MuC[0] -= fStepSize; 
               break;
            case 'q':
               MuC[1] += fStepSize; 
               break;
            case 'z':
               MuC[1] -= fStepSize; 
               break;
            case 'a':
               MuC[2] += fStepSize; 
               break;
            case 'd':
               MuC[2] -= fStepSize; 
               break;
            case 'e':
               MuC[3] += fStepSize; 
               break;
            case 'c':
               MuC[3] -= fStepSize; 
               break;
          }
	      switch( wParam )
		  {
			case VK_ESCAPE:
              SendMessage( hWnd, WM_CLOSE, 0, 0 );
				    break;
            case VK_ADD :
              if(Epsilon >= 0.002f)
                   Epsilon *= (1.0f / 1.05f);
            break;
            case VK_SUBTRACT :
             if(Epsilon < 0.01f)
                 Epsilon *= 1.05f;
            break;
            case VK_SPACE :
              animated = !animated;
            break;
          } 
          break;
        }
        default:
            return DefWindowProc( hWnd, msg, wParam, lParam );
    }
    return 0;
}

static void Interpolate( float m[4], float t, float a[4], float b[4] )
{
    for ( int i = 0; i < 4; i++ )
        m[ i ] = ( 1.0f - t ) * a[ i ] + t * b[ i ];
}


float dt; // time increment depending on frame rendering time for same animation speed independent of rendering speed

static void UpdateMu( float *t, float a[4], float b[4] )
{
    *t += 0.01f * dt;
    
    if ( *t >= 1.0f )
    {
        *t = 0.0f;

        a[ 0 ] = b[ 0 ];
        a[ 1 ] = b[ 1 ];
        a[ 2 ] = b[ 2 ];
        a[ 3 ] = b[ 3 ];

        b[ 0 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
        b[ 1 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
        b[ 2 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
        b[ 3 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
    }
}

static void RandomColor( float v[4] )
{
    do
    {
		v[ 0 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
		v[ 1 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
		v[ 2 ] = 2.0f * rand() / (float) RAND_MAX - 1.0f;
    } while (v[0] < 0 && v[1] <0 && v[2]<0); // prevent black colors
    v[ 3 ] = 1.0f;
}

static void UpdateColor( float t[4], float a[4], float b[4] )
{
    *t += 0.01f *dt;
   
    if ( *t >= 1.0f )
    {
        *t = 0.0f;

        a[ 0 ] = b[ 0 ];
        a[ 1 ] = b[ 1 ];
        a[ 2 ] = b[ 2 ];
        a[ 3 ] = b[ 3 ];

        RandomColor(b);
    }
}

void Render()
{
	static float elapsedPrev = 0;
	float t = time.Elapsed();
	dt = (elapsedPrev == 0) ? 1 : (t-elapsedPrev)/1000 * 20;
	elapsedPrev = t;
	if(animated)
	{
      UpdateMu( &MuT, MuA, MuB );
      Interpolate( MuC, MuT, MuA, MuB );
      UpdateColor( &ColorT, ColorA, ColorB );
      Interpolate(ColorC, ColorT, ColorA, ColorB );
	}

    D3D11_MAPPED_SUBRESOURCE msr;
    g_pImmediateContext->Map(g_pcbFractal, 0, D3D11_MAP_WRITE_DISCARD, 0,  &msr);
	QJulia4DConstants mc;
    mc.c_height = g_height;
    mc.c_width  = g_width;
    mc.selfShadow = selfShadow;
    mc.zoom = zoom;
    mc.epsilon = Epsilon;
    mc.diffuse[0] = ColorC[0];
    mc.diffuse[1] = ColorC[1];
    mc.diffuse[2] = ColorC[2];
    mc.diffuse[3] = ColorC[3];
    mc.mu[0] = MuC[0];
    mc.mu[1] = MuC[1];
    mc.mu[2] = MuC[2];
    mc.mu[3] = MuC[3];
    for (int j=0; j<3; j++)
    for (int i=0; i<3; i++)
      mc.orientation[i + 4*j] = trackBall.GetRotationMatrix()(j,i);
    *(QJulia4DConstants *)msr.pData = mc;
    g_pImmediateContext->Unmap(g_pcbFractal,0);

    if (useComputeShaders)
    {
      ID3D11UnorderedAccessView* aUAViews[ 1 ] = { g_pComputeOutput };
      g_pImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, (unsigned int*)(&aUAViews) );
      g_pImmediateContext->CSSetConstantBuffers( 0, 1, &g_pcbFractal );
      g_pImmediateContext->CSSetShader( g_pCS_QJulia4D, NULL, 0 );
      g_pImmediateContext->Dispatch( (g_width+3)/4, (g_height+63)/64, 1 );
    }
    else
    {
      g_pImmediateContext->VSSetShader(g_pVS10, NULL, 0);
      g_pImmediateContext->PSSetShader(g_pPS10, NULL, 0);
      g_pImmediateContext->CSSetShader(NULL   , NULL, 0);

      g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );
      g_pImmediateContext->OMSetDepthStencilState(g_pDepthState, 0);
      g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
      g_pImmediateContext->RSSetState(g_pRasterizerState);
      g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pcbFractal );
      g_pImmediateContext->Draw(4, 0); // draw screen quad
    }

    // Present our back buffer to our front buffer
    g_pSwapChain->Present( vsync ? 1 : 0, 0 );

    if ( (t=time.Elapsed()/1000) - timer > 1) // every second
    {  
	    float td = t - timer; // real time interval a bit more than a second
        timer = t;
        if (useComputeShaders)
            swprintf(message,L"     QJulia4D V1.5 FPS %.2f, using compute shader (dx11)", (float)fps/td);
        else
            swprintf(message,L"     QJulia4D V1.5 FPS %.2f, using pixel shader (dx10)", (float)fps/td);
        SetWindowText(g_hWnd, message);
        fps=0;
    }
    fps++;
}