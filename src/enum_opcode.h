
#ifndef ENUM_OPCODE_H
#define ENUM_OPCODE_H

enum Opcode
{
    /* Zone entry */
    OP_SetDataRate          = 0x21e8,
    OP_ZoneEntry            = 0x202a,
    OP_PlayerProfile        = 0x202d,
    OP_Weather              = 0x2136,
    OP_InventoryRequest     = 0x205d,
    OP_ZoneInfoRequest      = 0x200a,
    OP_ZoneInfo             = 0x205b,
    OP_ZoneInUnknown        = 0x2147,
    OP_EnterZone            = 0x20d8,
    OP_EnteredZoneUnknown   = 0x21c3,
    
    OP_Inventory            = 0x2031,
    OP_PositionUpdate       = 0x20a1,
    OP_ClientPositionUpdate = 0x20f3,
    OP_SpawnAppearance      = 0x20f5,
    OP_Spawn                = 0x2149,
    OP_CustomMessage        = 0x2180,
    
    OP_SwapItem             = 0x212c,
    OP_MoveCoin             = 0x212d,
};

#endif/*ENUM_OPCODE_H*/
