/*
 * Copyright 2011 Tanjeff-Nicolai Moos <tanjeff@cccmz.de>
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

#include "IndexAllocatePDU.hpp"


using namespace agentxcpp;

IndexAllocatePDU::IndexAllocatePDU(data_t::const_iterator& pos,
				   const data_t::const_iterator& end,
				   bool big_endian)
    : PDUwithContext(pos, end, big_endian)
{
    // Get VarBind's until the PDU is completely parsed
    while( pos < end )
    {
	vb.push_back(varbind(pos, end, big_endian));
    }
}
	    



data_t IndexAllocatePDU::serialize()
{
    data_t serialized;

    // Add VarBind's
    vector<varbind>::const_iterator i;
    for(i = vb.begin(); i < vb.end(); i++)
    {
	serialized += i->serialize();
    }

    // Add header
    add_header(PDU::agentxIndexAllocatePDU, serialized);

    // return serialized form of PDU
    return serialized;
}