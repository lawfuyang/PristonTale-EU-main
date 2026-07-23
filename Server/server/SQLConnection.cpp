#include "stdafx.h"
#include "SQLConnection.h"


using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::duration;

std::vector<SQLConnection*> vSQLConnection;

std::chrono::steady_clock::time_point startTime;

//uncomment this to help you narrow down where the connection wasn't being closed.
//review the logs afterwards.
//#define SQL_LOG_PREPARE_AND_CLOSE

BOOL show_errorSQL( unsigned int handletype, const SQLHANDLE& handle )
{
	static SQLCHAR sqlstateSqlError[ 1024 ];
	static SQLCHAR messageSqlError[ 1024 ];
	static char szBufSQLError[ 2048 ];
	ZeroMemory( szBufSQLError, 2048 );
	ZeroMemory( sqlstateSqlError, 1024 );
	ZeroMemory( messageSqlError, 1024 );
	if ( SQL_SUCCESS == SQLGetDiagRecA( handletype, handle, 1, sqlstateSqlError, NULL, messageSqlError, 1024, NULL ) )
	{
		STRINGFORMAT( szBufSQLError, "Message: %s\nSQLSTATE: %s\n", messageSqlError, sqlstateSqlError );
		LOGERROR( szBufSQLError );
		return TRUE;
	}
	return FALSE;
}

DatabaseStructureData saDatabaseData[]
{
	{ DATABASEID_GameDB,				 "GameDB" },
	{ DATABASEID_GameDB_Event,			 "GameDB" },
	{ DATABASEID_GameDB_Aging,			 "GameDB" },
	{ DATABASEID_GameDB_Npc,			 "GameDB" },
	{ DATABASEID_GameDB_Monsters,		 "GameDB" },
	{ DATABASEID_GameDB_Mixing,			 "GameDB" },

	{ DATABASEID_GameDB_QuestList,		 "GameDB" },
	{ DATABASEID_GameDB_QuestList_Top,	 "GameDB" },
	{ DATABASEID_GameDB_QuestEventList,	 "GameDB" },
	{ DATABASEID_GameDB_PetSystem,		 "GameDB" },
	{ DATABASEID_GameDB_PetSystem_Skin,	 "GameDB" },

	{ DATABASEID_ServerDB,			"ServerDB" },
	{ DATABASEID_LogDB,				"LogDB" },
	{ DATABASEID_LogDB_ServerLog,	"LogDB" },
	{ DATABASEID_LogDB_Cleaner,		"LogDB" },
	{ DATABASEID_LogDB_PacketLog,	"LogDB" },
	{ DATABASEID_LogDB_DatabaseLog,	"LogDB" },
	{ DATABASEID_LogDB_EventKill,	"LogDB" },

	{ DATABASEID_SkillDB,			"SkillDB" },
	{ DATABASEID_SkillDBNew,        "SkillDBNew" },

	{ DATABASEID_EventDB,			"EventDB" },
	{ DATABASEID_ItemDB,			"ItemDB" },

	{ DATABASEID_ClanDB,			"ClanDB" },
	{ DATABASEID_ClanDB_2,			"ClanDB" },
	{ DATABASEID_ClanDB_3,			"ClanDB" },
	{ DATABASEID_ClanDB_4,			"ClanDB" },

	{ DATABASEID_ChatDB,			"ChatDB" },
	{ DATABASEID_ChatDB_Cleaner,	"ChatDB" },

	{ DATABASEID_UserDB_LocalServer_Misc,		 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_UserInfo,	 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_CharInfo,	 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_Bless,		 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_Quest,		 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_Quest_Sub,	 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_Timers,		 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_TitleAura,	 "UserDB" },
	{ DATABASEID_UserDB_LocalServer_KillTracker, "UserDB" },
	{ DATABASEID_UserDB_LocalServer_ItemBox,	 "UserDB" },


	{ DATABASEID_UserDB_PrimaryServer,			"UserDB" },
	{ DATABASEID_UserDB_PrimaryServer_2,		"UserDB" },

	{ DATABASEID_UserDB_PrimaryServerSyncer,	"UserDB" },

};


SQLConnection::SQLConnection()
{
}


SQLConnection::~SQLConnection()
{
}


void SQLConnection::LogDatabaseStats()
{
	SQLConnection * pcDB = SQLCONNECTION ( DATABASEID_LogDB_DatabaseLog );

	if ( pcDB->Open () )
	{
		const char * szQuery = "INSERT INTO DatabaseLog ([ServerType],[DatabaseID],[SubID],[TotalCount],[TotalDurationMS],[MinDurationMS],[MaxDurationMS],[DateTime]) VALUES (?,?,?,?,?,?,?,GETDATE())";
		DWORD iOpenID = 0;
		DatabaseStat * pStat = NULL;

		for ( auto & kv : sDBStat )
		{
			iOpenID = kv.first;
			pStat = &kv.second;

			if ( pcDB->Prepare( szQuery ) )
			{
				pcDB->BindParameterInput( 1, EDatabaseDataType::PARAMTYPE_Integer, &GAME_SERVER );
				pcDB->BindParameterInput( 2, EDatabaseDataType::PARAMTYPE_Integer, &eID );
				pcDB->BindParameterInput( 3, EDatabaseDataType::PARAMTYPE_Integer, &iOpenID );
				pcDB->BindParameterInput( 4, EDatabaseDataType::PARAMTYPE_Integer, &pStat->iCounter );
				pcDB->BindParameterInput( 5, EDatabaseDataType::PARAMTYPE_Float, &pStat->fTotalDuration );
				pcDB->BindParameterInput( 6, EDatabaseDataType::PARAMTYPE_Float, &pStat->fMinDuration );
				pcDB->BindParameterInput( 7, EDatabaseDataType::PARAMTYPE_Float, &pStat->fMaxDuration );

				pcDB->Execute();
			}

			pcDB->Clear();
		}
		pcDB->Close ();

		pStat = NULL;
	}
}

DatabaseStructureData * SQLConnection::GetDatabaseData( EDatabaseID eDatabaseID )
{
	for ( auto & v : saDatabaseData )
	{
		if ( v.eDatabaseID == eDatabaseID )
			return &v;
	}

	return NULL;
}

SQLINTEGER * SQLConnection::BlockNext()
{
	for ( int i = 0; i < MAX_PARAMS; i++ )
	{
		if ( baBlock[i] == false )
		{
			baBlock[i] = true;
			return &iaBlock[i];
		}
	}

	return NULL;
}

void SQLConnection::BlockFree()
{
	for ( int i = 0; i < MAX_PARAMS; i++ )
		baBlock[i] = false;
}

void SQLConnection::Init( EDatabaseID eDatabaseID, std::string configPrefix )
{
	//Copy database name to class
	strcpy_s( szDatabaseName, GetDatabaseData( eDatabaseID )->pszDatabaseName );
	eID = eDatabaseID;

	//Try alloc Handle SQL ODBC
	if ( SQL_SUCCESS != SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv ) )
		show_errorSQL( SQL_HANDLE_ENV, hStatement );

	//Set Version ODBC
	if ( SQL_SUCCESS != SQLSetEnvAttr( hEnv, SQL_ATTR_ODBC_VERSION, ( SQLPOINTER )SQL_OV_ODBC3, 0 ) )
		return;

	//Try alloc handle connection
	if ( SQL_SUCCESS != SQLAllocHandle( SQL_HANDLE_DBC, hEnv, &hConnection ) )
		show_errorSQL( SQL_HANDLE_DBC, hStatement );

	//Set connection timeout
	SQLSetConnectAttrA( hConnection, SQL_LOGIN_TIMEOUT, ( SQLPOINTER )5, SQL_IS_INTEGER );


	char szStringConnectionSQL[ 512 ];
	SQLCHAR strconn[ 512 ];
	ZeroMemory( szStringConnectionSQL, 512 );



	//Set string connection

	INI::CReader * pcReader = new INI::CReader( "server.ini" );
	if ( pcReader )
	{
		std::string szHost;
		std::string	szUser;
		std::string szPasswrd;

		if ( !configPrefix.empty() )
		{
			szHost = pcReader->ReadString( "Database", FormatString( "%s_%s", configPrefix.c_str(), "Host" ) );
			szUser = pcReader->ReadString( "Database", FormatString( "%s_%s", configPrefix.c_str(), "User" ) );
			szPasswrd = pcReader->ReadString( "Database", FormatString( "%s_%s", configPrefix.c_str(), "Password" ) );
		}

		if ( szHost.empty() )
			szHost = pcReader->ReadString( "Database", "Host" );

		if ( szUser.empty() )
			szUser = pcReader->ReadString( "Database", "User" );

		if ( szPasswrd.empty() )
			szPasswrd = pcReader->ReadString( "Database", "Password" );

		STRINGFORMAT( szStringConnectionSQL, "Persist Security Info=False; Integrated Security=False; TrustServerCertificate=Yes; Driver=%s; Server=%s; Database=%s; Uid=%s; Pwd=%s;",
			pcReader->ReadString( "Database", "Driver" ).c_str(), szHost.c_str(), szDatabaseName, szUser.c_str(), szPasswrd.c_str() );
	}

	SAFE_DELETE( pcReader );

	memcpy( strconn, szStringConnectionSQL, 512 );

	SQLCHAR retconstring[ 1024 ];

	//Connect to Database
	SQLDriverConnectA( hConnection, NULL, strconn, lstrlenA((char*)strconn), retconstring, 1024, NULL, SQL_DRIVER_NOPROMPT );

	//Create Statement handler
	if ( SQL_SUCCESS != SQLAllocHandle( SQL_HANDLE_STMT, hConnection, &hStatement ) )
		show_errorSQL( SQL_HANDLE_DBC, hStatement );

	SQLSetStmtAttr( hStatement, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER)SQL_SCROLLABLE, SQL_IS_INTEGER );

	//Create mutex
	InitializeCriticalSection( &sCriticalSection );

	BlockFree();

	vSQLConnection.push_back( this );
}

BOOL SQLConnection::SQLTimeStampToSystemTime( SYSTEMTIME * psSystemTime, SQL_TIMESTAMP_STRUCT * psSQLTimeStamp )
{
	if ( !psSystemTime || !psSQLTimeStamp )
		return FALSE;

	psSystemTime->wYear			= psSQLTimeStamp->year;
	psSystemTime->wMonth		= psSQLTimeStamp->month;
	psSystemTime->wDayOfWeek	= 0;
	psSystemTime->wDay			= psSQLTimeStamp->day;
	psSystemTime->wHour			= psSQLTimeStamp->hour;
	psSystemTime->wMinute		= psSQLTimeStamp->minute;
	psSystemTime->wSecond		= psSQLTimeStamp->second;
	psSystemTime->wMilliseconds = (WORD)psSQLTimeStamp->fraction;

	return TRUE;
}

void SQLConnection::SetUniqueSubID( int iUniqueSubID )
{
	this->iLastUniqueSubID = this->iUniqueSubID; //for lockup checks

	this->iUniqueSubID = iUniqueSubID;
}

BOOL SQLConnection::Open( )
{
	//Lock Mutex
	EnterCriticalSection( &sCriticalSection );


#ifdef MEASURE_SQL_EXECUTION_TIME

	startTime = high_resolution_clock::now();


	if ( uNextDBLogTime == 0 )
	{
#ifdef DEBUG
		uNextDBLogTime = TIME + 10; //every 10 sec
#else
		uNextDBLogTime = TIME + 600; //every 10 min
#endif


	}
#endif

	if (bOpen)
	{
		LOGERROR( "[DB %s : %d : %d][Open()] : A previous connection was not closed! (Connection was last opened by sub id: %d)", szDatabaseName, eID, iUniqueSubID, this->iLastUniqueSubID ); //Tip: use #define SQL_LOG_PREPARE_AND_CLOSE to debug this if needed
	}

	bOpen = TRUE;

	return TRUE;
}

BOOL SQLConnection::Prepare( const char * pszQuery, bool p_IgnoreUnsecure)
{
	if ( !bOpen )
	{
		LOGERROR( "[DB %s : %d : %d][Prepare()] : %s [%s]", szDatabaseName, eID, iUniqueSubID, pszQuery, bOpen ? "TRUE" : "FALSE" );
		return FALSE;
	}

#if defined (DEV_MODE) && defined (SQL_LOG_PREPARE_AND_CLOSE)
	//uncomment this to figure out which connection didn't get closed properly
	LOG( "[DB %s : %d : %d][Prepare()]: %s", szDatabaseName, eID, iUniqueSubID, pszQuery );




#endif


	//Copy Query
	strcpy_s( szQuery, pszQuery );

	//Check SQLI
	for ( int i = 0; i < _countof( szQuery ); i++ )
	{
		//End string? break it
		if ( szQuery[i] == 0 )
			break;

		//Have quote mark? SQLI, return error
		if (!p_IgnoreUnsecure && szQuery[i] == 0x27 )
		{
			//Query Not secure
			WARN("[DB %s : %d : %d] [Prepare()] : Query not secure! %s", szDatabaseName, eID, iUniqueSubID, szQuery);
			return FALSE;
		}
	}

	//Prepare Query error? return
	if ( SQLPrepareA( hStatement, (SQLCHAR*)szQuery, SQL_NTS ) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][Prepare()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	bPrepared = TRUE;

	//Success
	return TRUE;
}

/// <summary>
/// Same as Execute() but also checks affected row count
/// </summary>
/// <returns></returns>
BOOL SQLConnection::ExecuteUpdate()
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][ExecuteUpdate()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	SQLRETURN iRetCode = SQL_SUCCESS;

	SQLINTEGER row_count;

	//Try Execute query
	if ( (iRetCode = SQLExecute( hStatement )) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][ExecuteUpdate()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}
	else
	{
		if (SQLRowCount( hStatement, &row_count ) == SQL_ERROR)
		{
			LOGERROR( "[DB %s : %d : %d][ExecuteUpdate()] SQLRowCount Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
			show_errorSQL( SQL_HANDLE_STMT, hStatement );
			return FALSE;
		}
		else
		{
			if (row_count <= 0)
			{
				WARN( "[DB %s : %d : %d][ExecuteUpdate()] No row was updated : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
				show_errorSQL( SQL_HANDLE_STMT, hStatement );
				return FALSE;
			}
		}
	}

	return TRUE;
}


BOOL SQLConnection::Execute(bool bSupressError)
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][Execute()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	SQLRETURN iRetCode = SQL_SUCCESS;

	//Try Execute query
	if ( (iRetCode = SQLExecute( hStatement )) == SQL_ERROR )
	{
		if (!bSupressError)
		{
			LOGERROR("[DB %s : %d : %d ][Execute()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery);
			show_errorSQL(SQL_HANDLE_STMT, hStatement);
		}
		return FALSE;
	}

	return TRUE;
}


BOOL SQLConnection::Execute( const char * pszQuery )
{
	if ( !bOpen )
	{
		LOGERROR( "[DB %s : %d : %d][Execute()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	SQLRETURN iRetCode = SQL_SUCCESS;

	//Try Execute query
	if ( (iRetCode = SQLExecDirectA( hStatement, (SQLCHAR*)pszQuery, SQL_NTS )) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][Execute()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	return TRUE;
}

BOOL SQLConnection::BindParameterInput( int iPosition, EDatabaseDataType eDataType, void * pParameter, int iSize )
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][BindParameterInput()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	int iType		= 0;
	int iSQLType	= 0;
	int iLen		= 0;

	SQLINTEGER * piInputOutput = BlockNext();

	//Set Data Type
	switch ( eDataType )
	{
		case PARAMTYPE_Byte:
			iType = SQL_C_TINYINT;
			iSQLType = SQL_TINYINT;
			*piInputOutput = 0;
			break;
		case PARAMTYPE_Integer:
			iType = SQL_C_LONG;
			iSQLType = SQL_INTEGER;
			*piInputOutput = 0;
			break;
		case PARAMTYPE_Short:
			iType = SQL_C_SHORT;
			iSQLType = SQL_INTEGER;
			*piInputOutput = 0;
			break;

		case PARAMTYPE_String:
			iType = SQL_C_CHAR;
			iSQLType = SQL_VARCHAR;
			iLen = lstrlenA( (char*)pParameter );
			*piInputOutput = SQL_NTS;
			break;
		case PARAMTYPE_Float:
			iType = SQL_C_FLOAT;
			iSQLType = SQL_REAL;
			*piInputOutput = 0;
			break;
		case PARAMTYPE_Int64:
			iType = SQL_C_SBIGINT;
			iSQLType = SQL_BIGINT;
			*piInputOutput = 0;
			break;
		case PARAMTYPE_Time:
			iType = SQL_C_TIMESTAMP;
			iSQLType = SQL_TYPE_TIMESTAMP;
			*piInputOutput = 0;
			break;
		case PARAMTYPE_Binary:
			iType = SQL_C_BINARY;
			iSQLType = SQL_BINARY;
			iLen = iSize;
			*piInputOutput = iSize;
			break;
		default:
			break;
	}

	//Bind parameters
	if ( SQLBindParameter( hStatement, iPosition, SQL_PARAM_INPUT, iType, iSQLType, iLen, 0, pParameter, 0, piInputOutput ) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][BindParameterInput()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	return TRUE;
}


BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, BYTE * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Byte )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter );
}


BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, short * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Short )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter );
}


BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, unsigned short * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Short )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, int * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Integer )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, unsigned int * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Integer )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, SYSTEMTIME * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Time )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, signed __int64 * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Int64 )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, uint64_t * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Int64 )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter );
}


BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, unsigned long * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Integer )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, long * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Integer )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}


BOOL SQLConnection::GetBinaryData( int iPosition, void * pParameter, int iSize )
{
	return GetDataInternal( iPosition, EDatabaseDataType::PARAMTYPE_Binary, pParameter, iSize );
}

BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, char * pParameter, int iSize )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_String )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, iSize );
}


BOOL SQLConnection::GetData( int iPosition, EDatabaseDataType eDataType, float * pParameter )
{
	if ( eDataType != EDatabaseDataType::PARAMTYPE_Float )
		LOGERROR( "Wrong data type for query '%s' at pos: %d. Can cause memory corruption", szQuery, iPosition );

	return GetDataInternal( iPosition, eDataType, (void *)pParameter, 0 );
}


BOOL SQLConnection::GetDataInternal( int iPosition, EDatabaseDataType eDataType, void * pParameter, int iSize )
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d ][GetData()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	int iType				= 0;
	SQLINTEGER iDataSize	= 0;
	void * pParam			= pParameter;
	int iSizeParam			= iSize;
	SQL_TIMESTAMP_STRUCT	sTimeStamp;

	//Set Data Type
	switch ( eDataType )
	{
		case PARAMTYPE_Byte:
			iType = SQL_C_TINYINT;
			break;
		case PARAMTYPE_Integer:
			iType = SQL_C_LONG;
			break;
		case PARAMTYPE_Short:
			iType = SQL_C_SHORT;
			break;

		case PARAMTYPE_String:
			iType = SQL_C_CHAR;

			if (iSize == 0)
				WARN("[DB %s : %d : %d][GetData()] Length is 0 for String GetData at pos %d for query '%s'", szDatabaseName, eID, iUniqueSubID, iPosition, szQuery);

			break;
		case PARAMTYPE_Float:
			iType = SQL_C_FLOAT;
			break;
		case PARAMTYPE_Int64:
			iType = SQL_C_SBIGINT;
			break;
		case PARAMTYPE_Time:
			iType		= SQL_C_TYPE_TIMESTAMP;
			pParam		= &sTimeStamp;
			iSizeParam	= 23;
			break;
		case PARAMTYPE_Binary:
			iType = SQL_C_BINARY;
			break;
		default:
			break;
	}

	//Get Data
	if ( SQLGetData( hStatement, iPosition, iType, pParam, iSizeParam, &iDataSize ) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][GetData()] Error : %d : %s", szDatabaseName, eID, iUniqueSubID, iPosition, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	if ( eDataType == PARAMTYPE_Time )
		SQLTimeStampToSystemTime( (SYSTEMTIME*)pParameter, &sTimeStamp );

	return TRUE;
}

BOOL SQLConnection::GetColumnName( int iPosition, void * pParameter, int iSize )
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][GetColumnName()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	SQLSMALLINT cbSize = 0;
	SQLSMALLINT cbDataType = 0;
	SQLUINTEGER cbColumn = 0;
	SQLSMALLINT cbScale = 0;
	SQLSMALLINT uNullType = 0;

	if ( SQLDescribeColA( hStatement, iPosition, (SQLCHAR*)pParameter, iSize, &cbSize, &cbDataType, &cbColumn, &cbScale, &uNullType ) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][GetColumnName()] Error : %d : %s", szDatabaseName, eID, iUniqueSubID, iPosition, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	return TRUE;
}

BOOL SQLConnection::GetColumnCount( int & iCount )
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][GetColumnCount()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	iCount = 0;

	SQLSMALLINT cbCount = 0;

	if( SQLNumResultCols(hStatement, &cbCount ) == SQL_ERROR )
	{
		LOGERROR( "[DB %s : %d : %d][GetColumnCount()] Error : %s", szDatabaseName, eID, iUniqueSubID, szQuery );
		show_errorSQL( SQL_HANDLE_STMT, hStatement );
		return FALSE;
	}

	iCount = (int)cbCount;

	return TRUE;
}

EDatabaseDataType SQLConnection::GetColumnType( const char * pszTableName, const char * pszColumnName )
{
	EDatabaseDataType i = PARAMTYPE_Null;

	char szResult[64] = { 0 };

	if ( Prepare( "SELECT DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME=? AND COLUMN_NAME=?" ) )
	{
		BindParameterInput( 1, PARAMTYPE_String, (char*)pszTableName, STRLEN( pszTableName ) );
		BindParameterInput( 2, PARAMTYPE_String, (char*)pszColumnName, STRLEN( pszColumnName ) );

		if ( Execute() && Fetch() )
		{
			GetDataInternal( 1, PARAMTYPE_String, szResult, _countof( szResult ) );

			if ( STRINGCOMPAREI( szResult, "varchar" ) )
				i = PARAMTYPE_String;
			else if ( STRINGCOMPAREI( szResult, "tinyint" ) )
				i = PARAMTYPE_Byte;
			else if ( STRINGCOMPAREI( szResult, "smallint" ) )
				i = PARAMTYPE_Short;
			else if ( STRINGCOMPAREI( szResult, "int" ) )
				i = PARAMTYPE_Integer;
			else if ( STRINGCOMPAREI( szResult, "real" ) )
				i = PARAMTYPE_Float;
			else if ( STRINGCOMPAREI( szResult, "bigint" ) )
				i = PARAMTYPE_Int64;
		}
	}

	return i;
}

BOOL SQLConnection::Fetch()
{
	if ( !bOpen || !bPrepared )
	{
		LOGERROR( "[DB %s : %d : %d][Fetch()] : %s [%s,%s]", szDatabaseName, eID, iUniqueSubID, szQuery, bOpen ? "TRUE" : "FALSE", bPrepared ? "TRUE" : "FALSE" );
		return FALSE;
	}

	//Fetch ( Next Row )
	if ( !SQL_SUCCEEDED( SQLFetch( hStatement ) ) )
		return FALSE;

	return TRUE;
}

BOOL SQLConnection::Close()
{
	if ( !bOpen )
		return FALSE;

#ifdef MEASURE_SQL_EXECUTION_TIME

	if ( eID != DATABASEID_LogDB_DatabaseLog &&
		 eID != DATABASEID_LogDB_PacketLog)
	{
		if ( sDBStat.find( this->iUniqueSubID ) == sDBStat.end() )
		{
			sDBStat.insert( { this->iUniqueSubID, DatabaseStat{} } );
			sDBStat[this->iUniqueSubID].fMinDuration = 10000.0;
		}

		duration<double, std::milli> ms_double = high_resolution_clock::now() - startTime;

		double fDuration = ms_double.count();

		sDBStat[this->iUniqueSubID].iCounter += 1;
		sDBStat[this->iUniqueSubID].fTotalDuration += static_cast<float>( fDuration );

		if ( fDuration > sDBStat[this->iUniqueSubID].fMaxDuration )
			sDBStat[this->iUniqueSubID].fMaxDuration = static_cast<float>( fDuration );

		if ( fDuration < sDBStat[this->iUniqueSubID].fMinDuration )
			sDBStat[this->iUniqueSubID].fMinDuration = static_cast<float>(fDuration);

		sDBStat[this->iUniqueSubID].iCounter += 1;

		if ( TIME > uNextDBLogTime )
		{
			if ( sDBStat[this->iUniqueSubID].iCounter > 0 )
			{
				LogDatabaseStats();
				sDBStat.clear();
			}

#ifdef DEBUG
			uNextDBLogTime = TIME + 10; //every 10 sec
#else
			uNextDBLogTime = TIME + 600; //every 10 min
#endif
		}
	}
#endif

	Clear();

#if defined (DEV_MODE) && defined (SQL_LOG_PREPARE_AND_CLOSE)
	LOG( "[DB %s : %d : %d][Close()]", szDatabaseName, eID, iUniqueSubID, );
#endif

	bOpen		= FALSE;

	//Unlock Mutex
	LeaveCriticalSection( &sCriticalSection );
	return TRUE;
}

BOOL SQLConnection::Clear()
{
	SQLCancel( hStatement );
	SQLFreeStmt( hStatement, SQL_CLOSE );
	SQLFreeStmt( hStatement, SQL_UNBIND );
	SQLFreeStmt( hStatement, SQL_RESET_PARAMS );

	BlockFree();

	//Clear Query
	ZeroMemory( szQuery, _countof( szQuery ) );

	bPrepared	= FALSE;

	return TRUE;
}

/// <summary>
/// Get pointer to SQL connection
/// </summary>
/// <param name="iSubConnectID">Optional sub connection id (For db logging purposes)</param>
/// <returns></returns>
SQLConnection * SQLConnection::GetConnection( EDatabaseID eDatabaseID, int iUniqueSubID )
{
	//Find connection pointer
	for ( const auto v : vSQLConnection )
	{
		//Same DatabaseID? return pointer of class
		if ( v->eID == eDatabaseID )
		{
			v->SetUniqueSubID( iUniqueSubID );
			return v;
		}
	}
	return NULL;
}