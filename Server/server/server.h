#pragma once

#include "CServer.h"

#include "servercore.h"

#include "common.h"
#include "mapserver.h"
#include "userserver.h"
#include "packetserver.h"
#include "accountserver.h"
#include "characterserver.h"
#include "sqlconnection.h"
#include "netserver.h"
#include "pvpserver.h"
#include "socketserver.h"
#include "servercommand.h"
#include "itemserver.h"
#include "questserver.h"
#include "eventserver.h"
#include "bellatraserver.h"
#include "blesscastleserver.h"
#include "characterserver.h"
#include "unitserver.h"
#include "chatserver.h"
#include "cheatserver.h"
#include "logserver.h"
#include "SQLSkill.h"
#include "unitinfo.h"
#include "lootserver.h"
#include "logger.h"

#include "CConfigFileReader.h"

extern BOOL g_bLoginServer;
extern BOOL g_bGameServer;
extern BOOL g_bServerIsSeasonal;

extern CLogger * g_pcLogger;

// Master gate for offline item cache optimization.
// When true, both OldItemDefinition and ItemList data loaded from SQL is
// serialized to disk on first run and read back on subsequent runs,
// skipping ~3s of SQL queries. The old item definitions are included
// in the same cache file so neither query runs on a cache hit.
// Set to false to always load from SQL (e.g. if DB data changes).
constexpr bool CACHE_OLD_ITEM_DEFS = false;

#define GSERVER					Server::GetInstance()

#define SERVER_TYPE				(GSERVER->GetServerType())
#define LOGIN_SERVER			g_bLoginServer
#define GAME_SERVER				g_bGameServer
#define SERVER_IS_SEASONAL		g_bServerIsSeasonal
#define LOGGER					g_pcLogger

#define LOG						g_pcLogger->Log
#define INFO					g_pcLogger->LogInfo
#define WARN					g_pcLogger->LogWarn
#define LOGERROR				g_pcLogger->LogError
#define DEBUG					g_pcLogger->LogDebug
#define DEBUGPACKET				g_pcLogger->LogDebugPacket
#define DEBUGIF					g_pcLogger->LogDebugIf
#define STATUS					g_pcLogger->LogStatus
#define DEBUGSTATUS				g_pcLogger->LogStatus

#define SERVER_PORT				(GSERVER->GetServerInfo()->iPort)

#define TICKCOUNT64				GetTickCount64()	//does not roll over
#define TICKCOUNT				Server::TickCount()
#define TIME					time (nullptr)		//Get time now in seconds since Jan 1, 1970

#define SYSTEMTIMEDWORD			Server::GetSystemTimeDWORD();


#define SERVER_MUTEX			Server::GetInstance()->GetMutex()

#define SOCKETSERVER					(GSERVER->GetSocketServer())
#define	PACKETSERVER					(GSERVER->GetPacketServer())
#define	ACCOUNTSERVER					(GSERVER->GetAccountServer())
#define	CHARACTERSERVER					(GSERVER->GetCharacterServer())
#define CHATSERVER						(GSERVER->GetChatServer())
#define SERVERCOMMAND					(GSERVER->GetServerCommand())
#define UNITSERVER						(GSERVER->GetUnitServer())
#define USERSERVER						(GSERVER->GetUserServer())
#define SQLSKILL						(GSERVER->GetSQLSkill())
#define NETSERVER						(GSERVER->GetNetServer())
#define ITEMSERVER						(GSERVER->GetItemServer())
#define LOOTSERVER						(GSERVER->GetLootServer())
#define LOGSERVER						(GSERVER->GetLogServer())
#define CHEATSERVER						(GSERVER->GetCheatServer())
#define MAPSERVER						(GSERVER->GetMapServer())
#define QUESTSERVER						(GSERVER->GetQuestServer())
#define UNITINFODATA					(GSERVER->GetUnitInfoServer())
#define BELLATRASERVER					(GSERVER->GetBellatraServer())
#define PVPSERVER						(GSERVER->GetPvPServer())
#define BLESSCASTLESERVER				(GSERVER->GetBlessCastleServer())

class Server : public ServerCore
{
private:
	static Server				* pcInstance;

	EServerType					  iServerType;
	SYSTEMTIME					  sLocalTime;

	HINSTANCE					  hInstance;
	HWND						  hWnd;

	CMutex						* pcMutex;

	int							  iServers;
	ServerInfo					  saServerInfo[MAX_SERVERINFO];
	ServerInfo					* psServerInfo;

	bool						  bMaintenanceCountdown;
	int							  iMaintenanceCountdown;
	DWORD						  dwLastAnnounce;

	int							  iGameVersion;
	int							  iServerVersion;

	PacketServerList			  sPacketServerList;

	//Servers
	SocketServer					* pcSocketServer;
	PacketServer					* pcPacketServer;
	AccountServer					* pcAccountServer;
	SQLConnection					* pcSQLServerConn;
	CharacterServer					* pcCharacterServer;
	ChatServer						* pcChatServer;
	ServerCommand					* pcServerCommand;
	UnitServer						* pcUnitServer;
	UserServer						* pcUserServer;
	CSQLSkill						* pcSQLSkill;
	NetServer						* pcNetServer;
	ItemServer						* pcItemServer;
	LootServer						* pcLootServer;
	LogServer						* pcLogServer;
	CheatServer						* pcCheatServer;
	MapServer						* pcMapServer;
	QuestServer						* pcQuestServer;
	UnitInfoServer					* pcUnitInfoServer;
	BellatraServer					* pcBellatraServer;
	BlessCastleServer				* pcBlessCastleServer;
	PvPServer						* pcPvPServer;

public:
	static void					  CreateInstance() { pcInstance = new Server(); }
	static Server				* GetInstance() { return pcInstance; }
	static void					  DeleteInstance() { delete pcInstance; }

	static void					  CreateSQLConnection( EDatabaseID eID, std::string configPrefix = std::string() );

	Server();
	virtual						 ~Server();

	void						  Init(HWND hWnd);
	void						  UnInit();
	void						  Load();
	void						  UnLoad();

	void						  DumpItemsIntoDatabase ();
	void						  RunBackgroundTask ();
	void						  Begin();
	void						  End();
	//void						  Test();

	void						  Tick(ETickType iTickType);
	void						  Loop();

	void						  Time(double fTime, CServer * pServer);

	void						  LoadServerInfo(CConfigFileReader * pcConfigFileReader, ServerInfo * si, EServerType eServerType, string strSection);

	inline EServerType			  GetServerType() { return iServerType; }
	inline SYSTEMTIME			* GetServerTime() { return &sLocalTime; }

	CMutex						* GetMutex() { return pcMutex; }

	ServerInfo					* GetServerInfo() { return psServerInfo; }

	char						* GetServerName(int iIndex);
	int							  GetServerIndex(char * pszIP, int iPort);


	void						  Tick10s ();
	void						  Tick60s ();

	void						  SqlUpdateOrInsertMetadata ( const char * key, int value );


	EGameLevel					  GetGameLevel(char * pszIP);

	int							  GetGameVersion() { return iGameVersion; }
	void						  SetGameVersion(int i) { iGameVersion = i; }
	int							  GetServerVersion() { return iServerVersion; }
	void						  SetServerVersion(int i) { iServerVersion = i; }

	void						  StartMaintenanceCountdown(int iSeconds);
	void						  StopMaintenanceCountdown();
	void						  AnnounceMaintenanceCountdown();

	BOOL						  SQLGetMaintenace();

	ID							  GetNextID();

	void						  PHServerList(SocketData * pcSocketData, int iTicket);
	void						  PHBossTime(User * pcUser);
	void						  PHLocalTime(User * pcUser);
	void						  PHSiegeTime(User * pcUser);

	static DWORD				  TickCount() { return GetTickCounts(); }
	static void					  TickCount(DWORD dw) { SetTickCounts(dw); }

	static DWORD				  GetSystemTimeDWORD()
								  {
										SYSTEMTIME	sCurrentTime;
										GetLocalTime(&sCurrentTime);
										return  SystemTimeToDWORD(&sCurrentTime);
								  }

	//SQL
	static void					  SQLUserOnline(char * pszAccountName, char * pszCharacterName, char * pszIP, int iTicket, EUsersOnline iAction, int iCharacterClass, int iCharacterLevel);
	static void					  SQLUpdateUserOnline(char * pszAccountName, char * pszGameServerName);
	static int					  SQLGetStatus();
	static void					  SQLSetStatus(int iStatus);

	SocketServer					* GetSocketServer() { return pcSocketServer; }
	PacketServer					* GetPacketServer() { return pcPacketServer; }
	AccountServer					* GetAccountServer() { return pcAccountServer; }
	SQLConnection					* GetSQLServerConnection() { return pcSQLServerConn; }
	CharacterServer					* GetCharacterServer() { return pcCharacterServer; }
	ChatServer						* GetChatServer() { return pcChatServer; }
	ServerCommand					* GetServerCommand() { return pcServerCommand; }
	UnitServer						* GetUnitServer() { return pcUnitServer; }
	UserServer						* GetUserServer() { return pcUserServer; }
	CSQLSkill						* GetSQLSkill() { return pcSQLSkill; }
	NetServer						* GetNetServer() { return pcNetServer; }
	ItemServer						* GetItemServer() { return pcItemServer; }
	LootServer						* GetLootServer() { return pcLootServer; }
	LogServer						* GetLogServer() { return pcLogServer; }
	CheatServer						* GetCheatServer() { return pcCheatServer; }
	MapServer						* GetMapServer() { return pcMapServer; }
	QuestServer						* GetQuestServer() { return pcQuestServer; }
	UnitInfoServer					* GetUnitInfoServer() { return pcUnitInfoServer; }
	BellatraServer					* GetBellatraServer() { return pcBellatraServer; }
	BlessCastleServer				* GetBlessCastleServer() { return pcBlessCastleServer; }
	PvPServer						* GetPvPServer() { return pcPvPServer; }
};
