
#ifndef ENUM_ZOP_H
#define ENUM_ZOP_H

enum ZOP
{
    ZOP_None,
    /* UDP thread */
    ZOP_UDP_OpenPort,
    ZOP_UDP_TerminateThread,
    ZOP_UDP_NewClient,
    ZOP_UDP_DropClient,
    ZOP_UDP_ToServerPacket,
    ZOP_UDP_ToClientPacketScheduled,
    ZOP_UDP_ToClientPacketImmediate,
    /* Log thread */
    ZOP_LOG_Write,
    ZOP_LOG_OpenFile,
    ZOP_LOG_CloseFile,
    ZOP_LOG_TerminateThread,
    /* DB thread */
    ZOP_DB_TerminateThread,
    ZOP_DB_OpenDatabase,
    ZOP_DB_CloseDatabase,
    ZOP_DB_QueryLoginCredentials,
    ZOP_DB_QueryLoginNewAccount,
    ZOP_COUNT
};

#endif/*ENUM_ZOP_H*/
