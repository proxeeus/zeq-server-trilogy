
#include "client.h"
#include "define_netcode.h"
#include "misc_struct.h"
#include "mob.h"
#include "util_alloc.h"

struct Client {
    Mob             mob;
    StaticBuffer*   surname;
    int64_t         clientId;
    int64_t         accountId;
    IpAddr          ipAddr;
    bool            isLocal;
    Zone*           zone;
    int64_t         experience;
    uint64_t        harmtouchTimestamp;
    uint64_t        disciplineTimestamp;
    uint64_t        creationTimestamp;
    uint16_t        trainingPoints;
    uint16_t        hunger;
    uint16_t        thirst;
    uint16_t        drunkeness;
    uint32_t        guildId;
    Coin            coin;
    Coin            coinCursor;
    Coin            coinBank;
    bool            isGM;
    uint8_t         anon;
    uint8_t         guildRank;
};

Client* client_create_unloaded(StaticBuffer* name, int64_t accountId, IpAddr ipAddr, bool isLocal)
{
    Client* client = alloc_type(Client);

    if (!client) return NULL;

    memset(client, 0, sizeof(Client));

    client->mob.parentType = MOB_PARENT_TYPE_Client;
    mob_set_name_sbuf(&client->mob, name);
    sbuf_grab(name);

    client->surname = NULL;
    client->clientId = -1;
    client->accountId = accountId;
    client->ipAddr = ipAddr;
    client->isLocal = isLocal;

    return client;
}

Client* client_destroy(Client* client)
{
    if (client)
    {
        mob_deinit(&client->mob);
        client->surname = sbuf_drop(client->surname);
    }

    return NULL;
}

void client_load_character_data(Client* client, ClientLoadData_Character* data)
{
    client->surname = data->surname;
    client->mob.level = data->level;
    client->mob.classId = data->classId;
    client->mob.baseGenderId = data->genderId;
    client->mob.genderId = data->genderId;
    client->mob.faceId = data->faceId;
    client->mob.baseRaceId = data->raceId;
    client->mob.raceId = data->raceId;
    client->mob.deityId = data->deityId;
    client->mob.loc = data->loc;
    mob_set_cur_hp_no_cap_check(&client->mob, data->currentHp);
    mob_set_cur_mana_no_cap_check(&client->mob, data->currentMana);
    mob_set_cur_endurance_no_cap_check(&client->mob, data->currentEndurance);
    client->mob.baseStats.STR = data->STR;
    client->mob.baseStats.STA = data->STA;
    client->mob.baseStats.DEX = data->DEX;
    client->mob.baseStats.AGI = data->AGI;
    client->mob.baseStats.INT = data->INT;
    client->mob.baseStats.WIS = data->WIS;
    client->mob.baseStats.CHA = data->CHA;
    
    client->experience = data->experience;
    client->harmtouchTimestamp = data->harmtouchTimestamp;
    client->disciplineTimestamp = data->disciplineTimestamp;
    client->creationTimestamp = data->creationTimestamp;
    client->trainingPoints = data->trainingPoints;
    client->hunger = data->hunger;
    client->thirst = data->thirst;
    client->drunkeness = data->drunkeness;
    client->guildId = data->guildId;
    client->coin = data->coin;
    client->coinCursor = data->coinCursor;
    client->coinBank = data->coinBank;
    client->isGM = data->isGM;
    client->anon = data->anon;
    client->guildRank = data->guildRank;

    /*fixme: calc base resists and max hp & mana based on level, class, race*/
}

StaticBuffer* client_name(Client* client)
{
    return client->mob.name;
}

Zone* client_get_zone(Client* client)
{
    return mob_get_zone(&client->mob);
}

void client_set_zone(Client* client, Zone* zone)
{
    mob_set_zone(&client->mob, zone);
}

bool client_is_local(Client* client)
{
    return client->isLocal;
}

IpAddr client_ip_addr(Client* client)
{
    return client->ipAddr;
}

uint32_t client_ip(Client* client)
{
    return client->ipAddr.ip;
}

uint16_t client_port(Client* client)
{
    return client->ipAddr.port;
}

void client_set_port(Client* client, uint16_t port)
{
    client->ipAddr.port = port;
}
