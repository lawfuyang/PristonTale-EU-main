#include "stdafx.h"
#include "CServerWindow.h"
#include "Logger.h"
#include "CServer.h"
#include "CConfigFileReader.h"

CServerWindow::CServerWindow() : CWindow()
{
	pConfig = new CServerConfig( "server.ini" );
	pServer = new CServer();
}

CServerWindow::~CServerWindow()
{
	SAFE_DELETE( pServer );
	SAFE_DELETE( pConfig );
}

UINT CServerWindow::Init()
{
	//LOGEx( "SERVER", "NOTICE : Init" );

	if( !Register( "Server", CS_HREDRAW | CS_VREDRAW, IDI_ICON, (HBRUSH)( COLOR_WINDOW ) ) )
	{
		LOGERROR( "Register() failed!\n\nGetLastError returned %d", GetLastError() );
		return 0;
	}

	if( !MakeWindow( "Fortress World Server", TRUE, FALSE, FALSE, WINDOW_WIDTH, WINDOW_HEIGHT ) )
	{
		LOGERROR( "MakeWindow() failed!" );
		return 0;
	}
	
	Server::CreateInstance();
	Server::GetInstance()->Init( hWnd );
	Server::GetInstance()->Load();

	if( !pServer->Init() )
		return FALSE;
	
	Server::GetInstance()->Begin();

	bExit = FALSE;

	//Clock thread interval.
	//
	//SERVER_UPDATE_INTERVAL_MS was already populated by ServerCore::LoadDirty(),
	//which runs inside Server::Load() above, so server.ini has been parsed by
	//this point.
	//
	//The frame accumulator in Update() targets 15.625ms (64 FPS). The legacy
	//hardcoded Sleep(15) beats against that target - at default 15.6ms timer
	//granularity a "15ms" sleep overshoots, so Loop() runs 0, 1 or 2 times per
	//wake instead of once. Requesting 1ms timer resolution and a shorter sleep
	//lets the accumulator fire on schedule.
	//
	//This does not spin: UpdaterThread uses SendMessageA(), which blocks until
	//the main thread has finished the frame, so the loop is self-limiting.
	dwUpdateTimeInterval = (DWORD)SERVER_UPDATE_INTERVAL_MS;

	if( dwUpdateTimeInterval < 15 )
	{
		//Raise system timer resolution so short sleeps are honoured.
		//Released in Shutdown().
		if( timeBeginPeriod( 1 ) == TIMERR_NOERROR )
			bHighResolutionTimer = TRUE;
		else
			LOGERROR( "timeBeginPeriod(1) failed; clock may be coarse" );
	}

	INFO( "Performance> Clock thread interval: %dms%s",
		dwUpdateTimeInterval,
		bHighResolutionTimer ? " (1ms timer resolution)" : "" );

	return 1;
}

BOOL CServerWindow::Shutdown()
{
	//LOGEx( "SERVER", "NOTICE : Shutdown" );

	bExit = TRUE;

	Server::GetInstance()->End();

	SHUTDOWN( pServer );

	Server::GetInstance()->UnLoad();
	Server::GetInstance()->UnInit();
	Server::DeleteInstance();
	LOGGER->Flush ();
	LOGGER->Close();

	RemoveWindow();
	Unregister();

	if( bHighResolutionTimer )
	{
		timeEndPeriod( 1 );
		bHighResolutionTimer = FALSE;
	}

	
	return FALSE;
}

BOOL CServerWindow::Run()
{
	//LOGEx( "SERVER", "NOTICE : Run" );

	LoopWindowMessages();

	bExit = TRUE;

	return TRUE;
}

LRESULT CServerWindow::WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	union
	{
		double fTime;
		uint32_t fTimeU32[2];
	} fTimeStruct;

	switch( uMsg )
	{
	/* System Defined Message */
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	case WM_PAINT:
		Render();
		break;

	/* User Defined Messages */
	case WM_UPDATE:
		fTimeStruct.fTimeU32[0] = (uint32_t)wParam;
		fTimeStruct.fTimeU32[1] = (uint32_t)lParam;
		Update(fTimeStruct.fTime);
		break;

	case WM_SOCKETACCEPT:
		SOCKETACCEPT( (SOCKET)wParam, (sockaddr_in *)lParam );
		break;
		
	case WM_SOCKETPACKET:
		SOCKETPACKET( (SocketData *)wParam, (PacketReceiving *)lParam );
		break;

	case WM_SOCKETCLOSE:
		SOCKETCLOSE( (SocketData *)wParam );
		break;

	/* Unhandled Messages */
	default:
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
		break;
	}

	return FALSE;
}

//50FPS
void CServerWindow::Update( double fTime )
{
	static double fTick = (1000.0f / ((double)64));
	static double fOffs = 0.0f;

	SERVER_MUTEX->Lock( 3000 );	

	//Split time into frames
	fOffs += fTime;
	if( fOffs >= fTick )
	{
		do
		{
			GSERVER->Loop();

			fOffs -= fTick;
		} 
		while( fOffs >= fTick );
	}

	GSERVER->Time( fTime, pServer);

	SERVER_MUTEX->Unlock();
}

void CServerWindow::Render()
{
	static HFONT hFont = NULL;

	PAINTSTRUCT sPS;
	HDC hDC;
	RECT sRect;

	hDC = BeginPaint( hWnd, &sPS );

	if( hFont == NULL )
		hFont = CreateFontA( 18, 0, 0, 0, FW_ULTRALIGHT, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, "Arial" );

	SetBkMode( hDC, TRANSPARENT );
	SetTextColor( hDC, RGB( 0, 0, 0 ) );
	SelectObject( hDC, (HGDIOBJ)hFont );

	switch( SERVER_TYPE )
	{
	case SERVERTYPE_Login:
		sRect.left		= 4;
		sRect.top		= 6;
		sRect.right		= WINDOW_WIDTH;
		sRect.bottom	= sRect.top + 20;
		DrawTextA( hDC, "[Login Server] ONLINE!", -1, &sRect, DT_LEFT );
		break;
	case SERVERTYPE_Game:
		sRect.left		= 4;
		sRect.top		= 6;
		sRect.right		= WINDOW_WIDTH;
		sRect.bottom	= sRect.top + 20;
		DrawTextA( hDC, "[Game Server] ONLINE!", -1, &sRect, DT_LEFT );
		break;
	case SERVERTYPE_Multi:
		sRect.left		= 4;
		sRect.top		= 6;
		sRect.right		= WINDOW_WIDTH;
		sRect.bottom	= sRect.top + 20;
		DrawTextA( hDC, "[Multi Server] ONLINE!", -1, &sRect, DT_LEFT );
		break;
	default:
		sRect.left		= 4;
		sRect.top		= 6;
		sRect.right		= WINDOW_WIDTH;
		sRect.bottom	= sRect.top + 20;
		DrawTextA( hDC, "Loading Server...", -1, &sRect, DT_LEFT );
		break;
	};

	EndPaint( hWnd, &sPS );
}