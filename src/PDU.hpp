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

#ifndef __PDU_H__
#define __PDU_H__

#include "types.hpp"
#include "exceptions.hpp"
#include <boost/asio.hpp>

namespace agentxcpp
{
    /**
     * \internal
     *
     * \brief The base class of all PDU's
     *
     * This class is never instantiated itself, but serves as the base class 
     * for concrete %PDU classes which don't have a context field (e.g.  
     * OpenPDU). For concrete %PDU's with context field, the class 
     * PDUwithContext is used.
     * 
     * This class represents the %PDU header and contains data and 
     * functionality common to all %PDU types. It provides an automatic 
     * packetID generator. Whenever a PDU object is created, the packetID is 
     * automatically incremented. The two exceptions are:
     * - The parse contructor fills the packetID from the received %PDU
     * - The ResponsePDU class overwrites the packetID
     *
     * The mechanism uses a static class member packetID_cnt to store the last 
     * used packetID. The counter wraps at its limit.
     */
    class PDU
    {
	private:

	    /**
	     * \brief the instance_registration flag
	     */
	    bool instance_registration;

	    /**
	     * \brief the new_index flag
	     */
	    bool new_index;

	    /**
	     * \brief the any_index flag
	     */
	    bool any_index;

	    /**
	     * \brief h.sessionID field
	     *
	     * According to RFC 2741, 6.1. "AgentX PDU Header"
	     */
	    uint32_t sessionID;
	    
	    /**
	     * \brief h.transactionID field
	     *
	     * According to RFC 2741, 6.1. "AgentX PDU Header"
	     */
	    uint32_t transactionID;
	    
	    /**
	     * \brief Counter for automatic packetID generator
	     *
	     * The packetID member is set automatically by the constructors; 
	     * each new PDU gets a new packetID. This member contains the last 
	     * used packetID.
	     *
	     * The parse constructor does not use this member, because it reads 
	     * the packetID from a stream.
	     */
	    static uint32_t packetID_cnt;
	    

	protected:
	    /**
	     * \brief Whether the PDU has a non-default context
	     *
	     * This field is only meaningful for certain PDU's (see RFC 2741, 
	     * section 6.1.1 "Context" for a description). 
	     * 
	     */
	    bool non_default_context;

	    /**
	     * \brief The PDU types
	     *
	     * According to RFC 2741, section 6.1 "AgentX PDU Header".
	     */
	    enum type_t
	    {
		agentxOpenPDU             = 1,
		agentxClosePDU            = 2,
		agentxRegisterPDU         = 3,
		agentxUnregisterPDU       = 4,
		agentxGetPDU              = 5,
		agentxGetNextPDU          = 6,
		agentxGetBulkPDU          = 7,
		agentxTestSetPDU          = 8,
		agentxCommitSetPDU        = 9,
		agentxUndoSetPDU          = 10,
		agentxCleanupSetPDU       = 11,
		agentxNotifyPDU           = 12,
		agentxPingPDU             = 13,
		agentxIndexAllocatePDU    = 14,
		agentxIndexDeallocatePDU  = 15,
		agentxAddAgentCapsPDU     = 16,
		agentxRemoveAgentCapsPDU  = 17,
		agentxResponsePDU         = 18

	    };

	    /**
	     * \brief h.packetID field according to RFC 2741, 6.1. "AgentX PDU
	     *        Header".
	     * 
	     * Is automatically filled by constructors, is set to another value 
	     * by ResponsePDU. The PDU class has no setter for this member.
	     *
	     * The ResponsePDU is a special case in resepect of packetIDs, 
	     * therefore this member is protected to allow the ResponsePDU to 
	     * alter it.
	     */
	    uint32_t packetID;
	    
	    /**
	     * \brief Parse constructor
	     *
	     * Read the %PDU header from a buffer and initialize part of the 
	     * %PDU object. Is called by the parse constructors of derived 
	     * classes. See \ref parsing for details about %PDU parsing.
	     * 
	     * \param pos Iterator pointing to the current stream position. The
	     *		  iterator is advanced while reading the header.
	     *
	     * \param end Iterator pointing one element past the end of the
	     *            current stream. This is needed to mark the end of the 
	     *            buffer.
	     *
	     * \param big_endian Whether the serialized form of the %PDU is in
	     *                   big_endian format.
	     *
	     * \exception parse_error If parsing fails, for example because
	     *			      reading the stream fails or the %PDU is 
	     *			      malformed.
	     */
	    PDU(data_t::const_iterator& pos,
		const data_t::const_iterator& end,
		bool big_endian);

	    /**
	     * \brief Construct the PDU header and add it to the payload
	     *
	     * Add the PDU header to the payload. Called by derived classes 
	     * during serialization.
	     * 
	     * \warning The payload must not grow or shrink after a call to
	     *          this function as its size is encoded into the header.
	     * \warning The function must not be called twice for the same PDU,
	     *          because this would add two headers and thus generate a 
	     *          malformed PDU.
	     *
	     * The header is encoded in big endian format.
	     *
	     * \param type The PDU type, according to RFC 2741, 6.1. "AgentX
	     *             PDU Header".
	     *
	     * \param payload The payload of the PDU. The header is added to
	     *                the payload, i.e. payload is altered by this 
	     *                function.
	     */
	    void add_header(type_t type, data_t& payload) const;

	    /**
	     * \brief Default constructor
	     *
	     * The members are set as follows:
	     * - packetID_cnt is incremented and the packetID is set to the new 
	     *   packetID_cnt value.
	     * - sessionID = 0
	     * - transactionID = 0
	     * - instance_registration = false
	     * - new_index = false
	     * - any_index = false
	     * - non_default_context = false
	     */
	    PDU();

	public:
	    /**
	     * \brief Get instance_registration flag
	     */
	    bool get_instance_registration() { return instance_registration; }
	    /**
	     * \brief Set instance_registration flag
	     */
	    void set_instance_registration(bool ir) { this->instance_registration = ir; }
	    
	    /**
	     * \brief Get new_index flag
	     */
	    bool get_new_index() { return new_index; }
	    /**
	     * \brief Set new_index flag
	     */
	    void set_new_index(bool new_index) { this->new_index = new_index; }
	    
	    /**
	     * \brief Get any_index flag
	     */
	    bool get_any_index() { return any_index; }
	    /**
	     * \brief Set any_index flag
	     */
	    void set_any_index(bool any_index) { this->any_index = any_index; }
	    
	    /**
	     * \brief Get sessionID
	     */
	    uint32_t get_sessionID() { return sessionID; }
	    /**
	     * \brief Set sessionID
	     */
	    void set_sessionID(uint32_t id) { this->sessionID = id; }

	    /**
	     * \brief Get transactionID
	     */
	    uint32_t get_transactionID() { return transactionID; }
	    
	    /**
	     * \brief Set transactionID
	     */
	    void set_transactionID(uint32_t id) { transactionID = id; }
	    
	    /**
	     * \brief Get packetID
	     */
	    uint32_t get_packetID() { return packetID; }
	    
	    /**
	     * \brief Set packetID
	     */
	    void set_packetID(uint32_t packetID) { this->packetID = packetID; }
	    
	    /**
	     * \brief Parse a %PDU from an input stream
	     *
	     * Read the %PDU into a buffer, then create a %PDU of the according 
	     * type (e.g. OpenPDU) from that buffer. See \ref parsing for 
	     * details about %PDU parsing.
	     *
	     * \exception parse_error If parsing fails, for example because
	     *			      reading the stream fails or the PDU is 
	     *			      malformed. The stream position is 
	     *			      undefined after this error.
	     * 
	     * \exception version_mismatch If the AgentX version of the %PDU
	     *                             in the stream is not 1.  The stream 
	     *                             position is undefined after this 
	     *                             error.
	     *
	     * \param in The input stream to read from.
	     *
	     * \return Pointer to PDU object of according type; the user must
	     *	       delete the object if it is not longer needed.
	     */
	    static PDU* get_pdu(boost::asio::local::stream_protocol::socket& in);
	    /* TODO: I originally wanted to use this one to be more flexible 
	     * (e.g.  read from cin, write to cout):
	     *
	     * static PDU* get_pdu(input_stream& in);
	     */
    };
}


#endif

