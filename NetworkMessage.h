#ifndef NETWORK_MESSAGE_H
#define NETWORK_MESSAGE_H

#include <vector>

#include "Packet.h"

//--------------------------------------------------------------------------------------------
// Higher-level view packets, used for Network transmission.
//--------------------------------------------------------------------------------------------
class NetworkMessage
{
private:
    size_t rpos, wpos;          // Read Pos, Write Pos
    std::vector<uint8_t> data;  // Raw Data

public:
    // Packet Constructor
    NetworkMessage() : rpos(0), wpos(0)
    {
        // Data is reserved, not allocated. Reserving data allows to avoid stress on the allocator when appending bytes to the packet.
        // This means when we'll send the packet to the network, we'll still use 'data.size()' which will only contain the bytes we've actually written and need to send.
        data.reserve(Packet::DEFAULT_PCKT_SIZE);
    }

    NetworkMessage(size_t reservedSize) : rpos(0), wpos(0)
    {
        data.reserve(reservedSize);
    }

    void Clear()
    {
        data.clear();
        wpos = wpos = 0;
    }

    size_t  Size()  const { return data.size(); }
    bool    Empty() const { return data.empty(); }
};

#endif
