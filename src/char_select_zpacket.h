
#ifndef CHAR_SELECT_ZPACKET_H
#define CHAR_SELECT_ZPACKET_H

#include "define.h"
#include "buffer.h"
#include "guild.h"

typedef struct {
    bool    isLast;
    Guild   guild;
} CharSelect_ZAddGuild;

typedef struct {
    int64_t     accountId;
    uint32_t    ip;
    char        sessionKey[16];
} CharSelect_ZLoginAuth;

typedef struct {
    void*           client;
    int             zoneId;
    uint16_t        port;
    StaticBuffer*   ipAddress;
    StaticBuffer*   motd;
} CharSelect_ZZoneSuccess;

typedef struct {
    void*   client;
} CharSelect_ZZoneFailure;

typedef union {
    CharSelect_ZAddGuild    zAddGuild;
    CharSelect_ZLoginAuth   zLoginAuth;
    CharSelect_ZZoneSuccess zZoneSuccess;
    CharSelect_ZZoneFailure zZoneFailure;
} CharSelect_ZPacket;

#endif/*CHAR_SELECT_ZPACKET_H*/
