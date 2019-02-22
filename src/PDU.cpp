/*
 * Copyright 2011-2016 Tanjeff-Nicolai Moos <tanjeff@cccmz.de>
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


#include "PDU.hpp"
#include "OpenPDU.hpp"
#include "ClosePDU.hpp"
#include "RegisterPDU.hpp"
#include "UnregisterPDU.hpp"
#include "ResponsePDU.hpp"
#include "CommitSetPDU.hpp"
#include "UndoSetPDU.hpp"
#include "TestSetPDU.hpp"
#include "CommitSetPDU.hpp"
#include "CleanupSetPDU.hpp"
#include "GetPDU.hpp"
#include "GetNextPDU.hpp"
#include "GetBulkPDU.hpp"
#include "NotifyPDU.hpp"
#include "util.hpp"

using namespace agentxcpp;


quint32 PDU::packetID_cnt = 0;



PDU::PDU()
{
    packetID = ++packetID_cnt;

    sessionID = 0;
    transactionID = 0;

    instance_registration=false;
    new_index=false;
    any_index=false;
    non_default_context = false;

}

PDU::PDU(binary::const_iterator& pos,
	 const binary::const_iterator& end,
	 bool big_endian)
{
    if(end - pos < 20)
    {
	throw(parse_error());
    }

    // skip protocol version (already checked by get_pdu()) and
    // type field (not needed here)
    pos += 2;

    // read flags
    quint8 flags = *pos++;
    instance_registration    = ( flags & (1<<0) ) ? true : false;
    new_index                = ( flags & (1<<1) ) ? true : false;
    any_index                = ( flags & (1<<2) ) ? true : false;
    non_default_context      = ( flags & (1<<3) ) ? true : false;

    // skip reserved field
    pos++;

    // read sessionID
    sessionID = read32(pos, big_endian);

    // read transactionID
    transactionID = read32(pos, big_endian);

    // read packetID
    packetID = read32(pos, big_endian);

    // skip payload length (not needed here)
    pos += 4;
}




QSharedPointer<PDU> PDU::parse_pdu(binary buf)
{
    // needed for parsing
    binary::const_iterator pos;

    // check protocol version
    quint8 version = buf[0];
    if( version != 1 )
    {
	// Wrong protocol:
	// throw exception
	throw( version_error() );
    }

    // read endianess flag
    quint8 flags = buf[2];
    bool big_endian = ( flags & (1<<4) ) ? true : false;

    // read payload length
    quint32 payload_length;
    pos = buf.begin() + 16;
    payload_length = read32(pos, big_endian);
    if( payload_length % 4 != 0 )
    {
	// payload length must be a multiple of 4!
	// See RFC 2741, 6.1. "AgentX PDU Header"
	// -> throw exception
	throw( parse_error() );
    }

    // read PDU type
    quint8 type = buf[1];

    // create PDU (TODO: complete the list!)
    QSharedPointer<PDU> pdu;
    pos = buf.begin();
    const binary::const_iterator end = buf.end();
    switch(type)
    {
	case agentxOpenPDU:
	    pdu = QSharedPointer<PDU>(new OpenPDU(pos, end, big_endian));
	    break;
	case agentxClosePDU:
	    pdu = QSharedPointer<PDU>(new ClosePDU(pos, end, big_endian));
	    break;
	case agentxRegisterPDU:
	    pdu = QSharedPointer<PDU>(new RegisterPDU(pos, end, big_endian));
	    break;
	case agentxUnregisterPDU:
	    pdu = QSharedPointer<PDU>(new UnregisterPDU(pos, end, big_endian));
	    break;
	case agentxResponsePDU:
	    pdu = QSharedPointer<PDU>(new ResponsePDU(pos, end, big_endian));
	    break;
	case agentxCommitSetPDU:
	    pdu = QSharedPointer<PDU>(new CommitSetPDU(pos, end, big_endian));
	    break;
	case agentxUndoSetPDU:
	    pdu = QSharedPointer<PDU>(new UndoSetPDU(pos, end, big_endian));
	    break;
	case agentxTestSetPDU:
	    pdu = QSharedPointer<PDU>(new TestSetPDU(pos, end, big_endian));
	    break;
	case agentxCleanupSetPDU:
	    pdu = QSharedPointer<PDU>(new CleanupSetPDU(pos, end, big_endian));
	    break;
	case agentxGetPDU:
	    pdu = QSharedPointer<PDU>(new GetPDU(pos, end, big_endian));
	    break;
	case agentxGetNextPDU:
	    pdu = QSharedPointer<PDU>(new GetNextPDU(pos, end, big_endian));
	    break;
	case agentxGetBulkPDU:
	    pdu = QSharedPointer<PDU>(new GetBulkPDU(pos, end, big_endian));
	    break;
	case agentxNotifyPDU:
	    pdu = QSharedPointer<PDU>(new NotifyPDU(pos, end, big_endian));
	    break;
	default:
	    // type is invalid
	    throw(parse_error());
    }

    // return created PDU
    return pdu;
}



void PDU::add_header(type_t type, binary& payload) const
{
    /* Construct header */
    binary header;

    // Protocol version
    header.push_back(1);

    // Type
    header.push_back(type);

    // flags
    quint8 flags = 0;
    if(instance_registration) flags |= (1<<0);
    if(new_index)             flags |= (1<<1);
    if(any_index)             flags |= (1<<2);
    if(non_default_context)   flags |= (1<<3);
    flags |= (1<<4);	// We always use big endian
    header.push_back(flags);

    // reserved field
    header.push_back(0);

    // remaining fields
    write32(header, sessionID);
    write32(header, transactionID);
    write32(header, packetID);
    write32(header, payload.size());	// payload length

    // Add header to payload
    payload.insert(0, header);
}
