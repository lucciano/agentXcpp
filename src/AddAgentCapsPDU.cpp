/*
 * Copyright 2011-2012 Tanjeff-Nicolai Moos <tanjeff@cccmz.de>
 *
 * This file is part of the agentXcpp library.
 *
 * AgentXcpp is free software: you can redistribute it and/or modify
 * it under the terms of the AgentXcpp library license, version 1, which 
 * consists of the GNU General Public License and some additional 
 * permissions.
 *
 * AgentXcpp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the AgentXcpp library license in the LICENSE file of this package 
 * for more details.
 */

#include "AddAgentCapsPDU.hpp"


using namespace agentxcpp;

AddAgentCapsPDU::AddAgentCapsPDU(binary::const_iterator& pos,
				 const binary::const_iterator& end,
				 bool big_endian)
    : PDUwithContext(pos, end, big_endian)
{
    // parse ID
    id = oid(pos, end, big_endian);

    // parse descr
    descr = OctetStringValue(pos, end, big_endian);
}
	    



binary AddAgentCapsPDU::serialize()
{
    binary serialized;

    // Serialize data
    serialized += id.serialize();
    serialized += descr.serialize();

    // Add header
    add_header(PDU::agentxAddAgentCapsPDU, serialized);

    // return serialized form of PDU
    return serialized;
}


AddAgentCapsPDU::AddAgentCapsPDU(oid id, OctetStringValue descr)
{
    this->id = id;
    this->descr = descr;
}
