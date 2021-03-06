
#include "client_packet_recv.h"
#include "client_packet_send.h"
#include "log_thread.h"
#include "misc_enum.h"
#include "packet_create.h"
#include "packet_static.h"
#include "zone.h"
#include "enum_opcode.h"

static void cpr_warn_unknown_opcode(Client* client, uint16_t opcode)
{
    Zone* zone = client_get_zone(client);
    log_writef(zone_log_queue(zone), zone_log_id(zone), "WARNING: client_packet_recv: received unexpected opcode: 0x%04x", opcode);
}

static void cpr_handle_op_inventory_request(Client* client)
{
    (void)client;
    /*fixme: handle this*/
}

static void cpr_handle_op_zone_info_request(Client* client)
{
    Zone* zone = client_get_zone(client);
    RingBuf* udpQueue = zone_udp_queue(zone);
    TlgPacket* packet;
    StaticPackets* staticPackets;
    
    /* Send OP_ZoneInfo */
    packet = packet_create_zone_info(zone);
    if (!packet) return;
    client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    
    /* Send OP_EnterZone */
    staticPackets = zone_static_packets(zone);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnterZone);
    
    /*fixme: send spawns, doors, objects*/
}

static void cpr_handle_op_enter_zone(Client* client)
{
    Zone* zone = client_get_zone(client);
    RingBuf* udpQueue = zone_udp_queue(zone);
    StaticPackets* staticPackets;
    TlgPacket* packet;
    int rc;
    
    /* Officially add the Client to all the Zone lists, so that we can get them their entityId */
    rc = zone_add_client_fully_zoned_in(zone, client);
    if (rc) goto fail;
    
    /* Inform the client of their entityId */
    packet = packet_create_spawn_appearance(0, SPAWN_APPEARANCE_SetEntityId, client_entity_id(client));
    if (!packet) goto fail;
    client_schedule_packet_with_udp_queue(client, udpQueue, packet);
    
    /* Broadcast the spawn packet, including to the client who is spawning */
    packet = packet_create_spawn(client_mob(client));
    if (!packet) goto fail;
    zone_broadcast_to_all_clients(zone, packet);
    
    /* Send the client some things it expects */
    staticPackets = zone_static_packets(zone);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnteredZoneUnknown);
    client_schedule_packet_with_udp_queue(client, udpQueue, staticPackets->packetEnterZone);
    
    /*fixme: Send guild rank SpawnAppearance, if applicable */
    /*fixme: Send GM SpawnAppearance, if applicable */
    
    /*fixme: calc the client's stats?*/
    /*fixme: send hp and mana updates*/
    
fail:;
}

void client_packet_recv(Client* client, ZPacket* zpacket)
{
    ToServerPacket packet;

    packet.opcode = zpacket->udp.zToServerPacket.opcode;
    packet.length = zpacket->udp.zToServerPacket.length;
    packet.data = zpacket->udp.zToServerPacket.data;
    
    switch (packet.opcode)
    {
    case OP_InventoryRequest:
        cpr_handle_op_inventory_request(client);
        break;
    
    case OP_ZoneInfoRequest:
        cpr_handle_op_zone_info_request(client);
        break;
    
    case OP_EnterZone:
        cpr_handle_op_enter_zone(client);
        break;
    
    /* Packets to be echoed with all their content */
    case OP_ZoneInUnknown:
        client_send_echo_copy(client, &packet);
        break;
    
    default:
        cpr_warn_unknown_opcode(client, packet.opcode);
        break;
    }
    
    if (packet.data)
        free(packet.data);
}
