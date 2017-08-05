
#include "main_thread.h"
#include "buffer.h"
#include "char_select_thread.h"
#include "client_mgr.h"
#include "db_thread.h"
#include "log_thread.h"
#include "login_thread.h"
#include "login_client.h"
#include "main_timers.h"
#include "ringbuf.h"
#include "timer.h"
#include "udp_thread.h"
#include "util_alloc.h"
#include "util_clock.h"
#include "zone_mgr.h"
#include "zpacket.h"
#include "enum_zop.h"
#include "enum2str.h"

#define MAIN_THREAD_LOG_PATH "log/main_thread.txt"

struct MainThread {
    DbThread*           db;
    LogThread*          log;
    UdpThread*          udp;
    LoginThread*        login;
    CharSelectThread*   cs;
    TimerPool           timerPool;
    RingBuf*            mainQueue;
    RingBuf*            logQueue;
    RingBuf*            dbQueue;
    Timer*              timerCSAuthTimeouts;
    StaticBuffer*       motd;
    int                 dbId;
    int                 logId;
    int                 loginServerId;
    ClientMgr           cmgr;
    ZoneMgr             zmgr;
};

static int mt_start_timers(MainThread* mt)
{
    mt->timerCSAuthTimeouts = timer_new(&mt->timerPool, 5000, mtt_cs_auth_timeouts, mt, true);
    if (!mt->timerCSAuthTimeouts) goto oom;

    return ERR_None;
oom:
    return ERR_OutOfMemory;
}

MainThread* mt_create()
{
    MainThread* mt = alloc_type(MainThread);
    int rc;

    if (!mt)
    {
        fprintf(stderr, "ERROR: mt_create: failed to allocate MainThread object\n");
        goto fail_alloc;
    }

    memset(mt, 0, sizeof(MainThread));
    
    timer_pool_init(&mt->timerPool);
    
    mt->motd = sbuf_create_from_literal("Welcome to ZEQ!");
    if (!mt->motd)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create default MotD string via sbuf_create_from_literal()\n");
        goto fail;
    }
    
    mt->mainQueue = ringbuf_create_type(ZPacket, 1024);
    if (!mt->mainQueue)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create RingBuf object for MainThread\n");
        goto fail;
    }

    /* Start the LogThread before anything else */
    mt->log = log_create();
    if (!mt->log)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create LogThread object\n");
        goto fail;
    }

    mt->logQueue = log_get_queue(mt->log);
    rc = log_open_file_literal(mt->log, &mt->logId, MAIN_THREAD_LOG_PATH);
    if (rc)
    {
        fprintf(stderr, "ERROR: mt_create: failed to open log file for MainThread at '" MAIN_THREAD_LOG_PATH "': %s\n", enum2str_err(rc));
        goto fail;
    }
    
    mt->db = db_create(mt->log);
    if (!mt->db)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create DbThread object\n");
        goto fail;
    }
    
    mt->dbQueue = db_get_queue(mt->db);
    rc = db_open_literal(mt->db, mt->mainQueue, &mt->dbId, DB_THREAD_DB_PATH_DEFAULT, DB_THREAD_DB_SCHEMA_PATH_DEFAULT);
    if (rc)
    {
        fprintf(stderr, "ERROR: mt_create: failed to open database file '" DB_THREAD_DB_PATH_DEFAULT "', schema '" DB_THREAD_DB_SCHEMA_PATH_DEFAULT "': %s\n", enum2str_err(rc));
        goto fail;
    }

    mt->udp = udp_create(mt->log);
    if (!mt->udp)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create UdpThread object\n");
        goto fail;
    }
    
    mt->cs = cs_create(mt->mainQueue, mt->log, mt->dbQueue, mt->dbId, mt->udp);
    if (!mt->cs)
    {
        fprintf(stderr, "ERROR: mt_create: failed to create CharSelectThread object\n");
        goto fail;
    }

    rc = cmgr_init(mt);
    if (rc)
    {
        fprintf(stderr, "ERROR: mt_create: cmgr_init() failed: %s\n", enum2str_err(rc));
        goto fail;
    }

    rc = zmgr_init(mt);
    if (rc)
    {
        fprintf(stderr, "ERROR: mt_create: zmgr_init() failed: %s\n", enum2str_err(rc));
        goto fail;
    }

    rc = mt_start_timers(mt);
    if (rc)
    {
        fprintf(stderr, "ERROR: mt_create: mt_start_timers() failed: %s\n", enum2str_err(rc));
        goto fail;
    }

    return mt;

fail:
    mt_destroy(mt);
fail_alloc:
    return NULL;
}

MainThread* mt_destroy(MainThread* mt)
{
    if (mt)
    {
        if (mt->udp)
        {
            udp_trigger_shutdown(mt->udp);
            //fixme: wait here, then destroy udp
        }
        
        if (mt->db)
        {
            //fixme: shut down
            mt->db = db_destroy(mt->db);
        }

        if (mt->log)
        {
            log_shut_down(mt->log);
            mt->log = log_destroy(mt->log);
        }

        mt->mainQueue = ringbuf_destroy(mt->mainQueue);
        mt->motd = sbuf_drop(mt->motd);
        timer_pool_deinit(&mt->timerPool);
        cmgr_deinit(&mt->cmgr);
        zmgr_deinit(&mt->zmgr);
        free(mt);
    }

    return NULL;
}

static void mt_process_commands(MainThread* mt, RingBuf* mainQueue)
{
    ZPacket zpacket;
    int zop;
    int rc;

    for (;;)
    {
        rc = ringbuf_pop(mainQueue, &zop, &zpacket);
        if (rc) break;

        switch (zop)
        {
        case ZOP_DB_QueryMainGuildList:
            cmgr_handle_guild_list(mt, &zpacket);
            break;

        case ZOP_DB_QueryMainLoadCharacter:
            cmgr_handle_load_character(mt, &zpacket);
            break;

        case ZOP_MAIN_ZoneFromCharSelect:
            cmgr_handle_zone_from_char_select(mt, &zpacket);
            break;

        default:
            log_writef(mt->logQueue, mt->logId, "WARNING: mt_process_commands: received unexpected zop: %s", enum2str_zop(zop));
            break;
        }
    }
}

#include "enum_login_server_rank.h"
#include "enum_login_server_status.h"
void mt_main_loop(MainThread* mt)
{
    RingBuf* mainQueue = mt->mainQueue;
    int rc;
    
    mt->login = login_create(mt->log, mt->dbQueue, mt->dbId, udp_get_queue(mt->udp), cs_get_queue(mt->cs));
    if (!mt->login)
    {
        fprintf(stderr, "login_create() failed\n");
        return;
    }
    
    rc = udp_open_port(mt->udp, 5998, loginc_sizeof(), login_get_queue(mt->login));
    if (rc)
    {
        fprintf(stderr, "udp_open_port() failed: %s\n", enum2str_err(rc));
        return;
    }
    
    rc = login_add_server(mt->login, &mt->loginServerId, "ZEQ Test", zmgr_remote_ip(&mt->zmgr), zmgr_local_ip(&mt->zmgr), LOGIN_SERVER_RANK_Standard, LOGIN_SERVER_STATUS_Up, true);
    if (rc)
    {
        fprintf(stderr, "login_add_server() failed: %s\n", enum2str_err(rc));
        return;
    }
    
    for (;;)
    {
        mt_process_commands(mt, mainQueue);
        timer_pool_execute_callbacks(&mt->timerPool);

        clock_sleep(25);
    }
}

ClientMgr* mt_get_cmgr(MainThread* mt)
{
    return &mt->cmgr;
}

ZoneMgr* mt_get_zmgr(MainThread* mt)
{
    return &mt->zmgr;
}

LogThread* mt_get_log_thread(MainThread* mt)
{
    return mt->log;
}

UdpThread* mt_get_udp_thread(MainThread* mt)
{
    return mt->udp;
}

RingBuf* mt_get_queue(MainThread* mt)
{
    return mt->mainQueue;
}

RingBuf* mt_get_db_queue(MainThread* mt)
{
    return mt->dbQueue;
}

RingBuf* mt_get_log_queue(MainThread* mt)
{
    return mt->logQueue;
}

RingBuf* mt_get_cs_queue(MainThread* mt)
{
    return cs_get_queue(mt->cs);
}

int mt_get_db_id(MainThread* mt)
{
    return mt->dbId;
}

int mt_get_log_id(MainThread* mt)
{
    return mt->logId;
}

StaticBuffer* mt_get_motd(MainThread* mt)
{
    return mt->motd;
}
