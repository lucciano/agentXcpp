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
#ifndef __AGENTX_H__
#define __AGENTX_H__

#include <boost/asio.hpp>

#include <fstream>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include "types.hpp"
#include "oid.hpp"
#include "ClosePDU.hpp"
#include "ResponsePDU.hpp"

namespace agentxcpp
{
    /**
     * \brief This class represents the master agent in a subagent program.
     *
     * This class is used on the subagent's side of a connection between 
     * subagent and master agent. It serves as a proxy which represents the 
     * master agent. It is possible for a subagent to hold connections to more 
     * than one master agents. For each connection one master_proxy object is 
     * created. Multiple connections to the same master agent are possible, 
     * too (although this is probably not useful), in which case one 
     * master_proxy per connection is needed.
     *
     * The master_proxy is always in one of the following states:
     *
     * -# connected
     * -# disconnected
     *
     * The session to the master agent is established when creating a 
     * master_proxy object, thus the object usually starts in connected state.  
     * If that fails, the object will start in disconnected state. A connected 
     * master_proxy object may also loose the connection to the master agent 
     * and consequently become disconnected, without informing the user of the 
     * AgentXcpp library. It is possible to re-connect with the reconnect() 
     * function at any time (even if the session is currently established - it 
     * will be shut down and re-established in this case).  When the object is 
     * destroyed, the session will be cleanly shut down. The connection state 
     * can be inspected with the is_connected() function.  Some functions 
     * throw a disconnected exception if the session is not currently 
     * established.
     *
     * This class uses the boost::asio library for networking and therefore 
     * needs a boost::asio::io_service object. This object can either be 
     * provided by the user or created automatically. There are two 
     * constructors available therefore. The used object (whether auto-created 
     * or not) can be obtained using the get_io_service() function. If the 
     * io_service object was autocreated by a constructor, it will be 
     * destroyed by the destructor. If the user provided the io_service, it 
     * will NOT be destroyed. It is possible to create multiple master_proxy 
     * objects using the same io_service object, or using different io_service 
     * objects.
     *
     * \internal
     * The io_service_by_user variable is used to store whether the io_service 
     * object was generated automatically. It is set to true or false by the 
     * respective constructors and evaluated by the destructor.
     *
     * Receiving and processing PDU's coming from the master is done 
     * asyncronously. The receive() function is used as a callback for this 
     * purpose.  When connecting to the master agent (i.e. in the connect() 
     * function), an asyncronous read operation is started to read the PDU 
     * header. The receive() function is executed when a header arrived. It 
     * reads the payload of the PDU, parses it (using PDU::parse_pdu()) and 
     * processes it.  Then it sets up an asyncronous read operation to wait 
     * for the next header. The disconnect() function cancels the current read 
     * operation before shutting down the session.
     */
    // TODO: describe timeout handling
    // TODO: byte ordering is constant for a session. See rfc 2741, 7.1.1
    class master_proxy
    {
	private:

	    /**
	     * \brief The mandatory io_service object.
	     *
	     * This object is needed for boost::asio sockets. Depending on the 
	     * constructor used, the object is either provided by the user or 
	     * generated automatically.
	     */
	    boost::asio::io_service* io_service;

	    /**
	     * \brief Was the io_service object provided by the user?
	     */
	    bool io_service_by_user;
	    
	    /**
	     * \brief The socket.
	     */
	    boost::asio::local::stream_protocol::socket socket;

	    /**
	     * \brief The endpoint used fo unix domain sockets.
	     */
	    boost::asio::local::stream_protocol::endpoint endpoint;

	    /**
	     * \brief The session ID of the current session.
	     *
	     * If disconnected, the value is undefined.
	     */
	    uint32_t sessionID;

	    /**
	     * \brief A string describing the subagent.
	     *
	     * Set upon object creation.
	     */
	    std::string description;

	    /**
	     * \brief Default timeout of the session (in seconds).
	     *
	     * A value of 0 indicates that there is no session-wide default.
	     */
	    byte_t default_timeout;

	    /**
	     * \brief An Object Identifier that identifies the subagent. May be
	     *        the null OID.
	     */
	    oid id;

	    /**
	     * \brief The SNMP objects registered with this master.
	     */
	    std::map<oid, variable*> registrations;

	    /**
	     * \brief Buffer to receive a PDU header
	     *
	     * When receiving a PDU asynchronously, the header is read into 
	     * this buffer. Then the receive() callback is invoked, which does 
	     * further processing.
	     *
	     * Since the AgentX-header is always 20 bytes in length, this 
	     * buffer is 20 bytes in size.
	     */
	    // TODO: avoid magic numbers, even if they are documented.
	    byte_t header_buf[20];

	    /**
	     * \brief Callback function to read and process a PDU
	     *
	     * This function is called when data is ready on the socket (the 
	     * header was already read into the header_buf buffer). It will 
	     * read one PDU from the socket, process it and set up the next 
	     * async read operation, so that it is called again for new data.  
	     * 
	     * Recieved ResponsePDU's are stored into the responses map if a 
	     * null pointer was stored there in advance.
	     */
	    void receive(const boost::system::error_code& result);

	    /**
	     * \brief The received, yet unprocessed ReponsePDU's.
	     *
	     * The wait_for_response() function stores a null pointer to this 
	     * map to indicate that it is waiting for a certain response. The 
	     * map key is the packetID.
	     * 
	     * When a response is received, the receive() function stores it 
	     * into the map, if a null pointer is found for the packetID of the 
	     * received ResponsePDU. Otherwise, the received ResponsePDU is 
	     * discarded.
	     *
	     * After a ResponsePDU was received and stored into the map, the 
	     * wait_for_response() function processes it and erases it from the 
	     * map.
	     */
	    std::map< uint32_t, boost::shared_ptr<ResponsePDU> > responses;


	    /**
	     * \brief Wait with timeout for a reponse.
	     *
	     * This function blocks until a ResponsePDU with the given 
	     * packetID is received or until the timeout expires, whichever 
	     * comes first.  The received ResponsePDU (if any) is returned.
	     *
	     * This function calls run_one() on the io_service object until 
	     * the desired ResponsePDU arrives or the timeout expires. This 
	     * may cause other asynchronous operations to be served, as well.  
	     * As a side effect, the function may return later than the 
	     * timeout value requests.
	     *
	     * The unprocessed ResponsePDU's are put into the reponses map by 
	     * the receive() function. This map is inspected by this function, 
	     * and the desired ResponsePDU is removed from the map before 
	     * returning it.
	     *
	     * \param packetID The packetID to wait for.
	     *
	     * \param timeout The timeout in milliseconds. Th default is 0,
	     *                meaning "use the session's default timeout".
	     *
	     * \exception timeout If the timeout expired before the ResponsePDU
	     *                    was received. 
	     *
	     * \return The received ResponsePDU.
	     */
	    boost::shared_ptr<ResponsePDU>
		wait_for_response(uint32_t packetID,
				  unsigned timeout=0);

	    /**
	     * \brief Disconnect instantly, without notifying the master agent
	     *
	     * This simply closes down the socket, without sending a ClosePDU.  
	     * This function does not throw.
	     */
	    void kill_connection()
	    {
		try {
		    this->socket.close();
		}
		catch(...)
		{
		    // Ignore
		}
	    }

	public:
	    /**
	     * \brief Create a session object connected via unix domain
	     *        socket
	     *
	     * This constructor tries to connect to the master agent. If that 
	     * fails, the object is created nevertheless and will be in state 
	     * disconnected.
	     *
	     * This constructor takes an io_service object as parameter. This 
	     * io_service is not destroyed by the consntructor.
	     *
	     * \param io_service The io_service object.
	     *
	     * \param description A string describing the subagent. This
	     *                    description cannot be changed later.
	     *
	     * \param default_timeout The length of time, in seconds, that
	     *                        the master agent should allow to elapse 
	     *                        after receiving a message before it 
	     *                        regards the subagent as not responding.  
	     *                        Allowed values are 0-255, with 0 meaning 
	     *                        "no default for this session".
	     *
	     * \param ID An Object Identifier that identifies the subagent.
	     *           Default is the null OID (no ID).
	     *
	     * \param unix_domain_socket The socket file to connect to.
	     *                           Defaults to /var/agentx/master, as 
	     *                           desribed in RFC 2741, section 8.2.1 
	     *                           "Well-known Values".
	     */
	    master_proxy(boost::asio::io_service* io_service,
		   std::string description="",
		   byte_t default_timeout=0,
		   oid ID=oid(),
		   std::string unix_domain_socket="/var/agentx/master");

	    /**
	     * \brief Create a session object connected via unix domain
	     *        socket
	     *
	     * This constructor tries to connect to the master agent. If that 
	     * fails, the object is created nevertheless and will be in state 
	     * disconnected.
	     *
	     * This constructor creates an io_service object which is destroyed 
	     * by the desctructor.
	     *
	     * \param description A string describing the subagent. This
	     *                    description cannot be changed later.
	     *
	     * \param default_timeout The length of time, in seconds, that the
	     *                        master agent should allow to elapse 
	     *                        after receiving a message before it 
	     *                        regards the subagent as not responding. 
	     *                        Allowed values are 0-255, with 0 meaning 
	     *                        "no default for this session".
	     *
	     * \param ID An Object Identifier that identifies the subagent.
	     *           Default is the null OID (no ID).
	     *
	     * \param unix_domain_socket The socket file to connect to.
	     *                           Defaults to /var/agentx/master, as 
	     *                           desribed in RFC 2741, section 8.2.1 
	     *                           "Well-known Values".
	     */
	    master_proxy(std::string description="",
		   byte_t default_timeout=0,
		   oid ID=oid(),
		   std::string unix_domain_socket="/var/agentx/master");

	    /**
	     * \brief Register a subtree with the master agent
	     *
	     * Before the master agent sends SNMP requests to a subagent, the 
	     * subagent must register some OIDs. Doing so informs the master 
	     * agent that the subagent wishes to handle requests for these 
	     * OIDs.
	     *
	     * This function registers a subtree (or MIB region). It is 
	     * typically called for the highest-level OID of the MIB which is 
	     * implemented by the subagent. The subagent will then receive 
	     * requests for the registered OID and its subtree. For example, 
	     * registering 1.3.6.1.4.1.42.13 registers all OIDs beginning with 
	     * this one, e.g. 1.3.6.1.4.1.42.13.1.1.2 and 
	     * 1.3.6.1.4.1.42.13.200.1.3.2.1.1.2.
	     *
	     * \param subtree The (root of the) subtree to register.
	     *
	     * \param priority The priority with which to register the subtree.
	     *                 Default is 127 according to RFC 2741, 6.2.3.  
	     *                 "The agentx-Register-PDU".
	     *
	     * \param timeout The timeout value for the registered subtree, in
	     *		      seconds. This value overrides the timeout of the 
	     *		      session.  Default value is 0 (no override) 
	     *		      according to RFC 2741, 6.2.3.  "The 
	     *		      agentx-Register-PDU".
	     *
	     * \exception disconnected If the master_proxy is currently in
	     *                         state 'disconnected'.
	     *
	     * \exception timeout_exception If the master agent does not
	     *                              respond within the timeout 
	     *                              interval.
	     *
	     * \exception internal_error If the master received a malformed
	     *                           PDU. This is probably a programming 
	     *                           error within the agentXcpp library.
	     *
	     * \exception master_is_unable The master agent was unable to
	     *                             perform the desired register 
	     *                             request.  The reason for that is 
	     *                             unknown.
	     *
	     * \exception duplicate_registration If the exact same subtree was
	     *                                   alread registered, either by 
	     *                                   another subagent or by this 
	     *                                   subagent.
	     *
	     * \exception master_is_unwilling If the master was unwilling for
	     *                                some reason to make the desired 
	     *                                registration.
	     *
	     * \exception parse_error If an unexpected response was sent by the
	     *                        master. This is probably a programming 
	     *                        error within the master agent. It is 
	     *                        possible that the master actually 
	     *                        performed the desired registration and 
	     *                        that a retry will result in a 
	     *                        duplicate_registration error.
	     */
	    void register_subtree(oid subtree,
				  byte_t priority=127,
				  byte_t timeout=0);

	    /**
	     * \brief Get the io_service object used by this master_proxy
	     */
	    boost::asio::io_service* get_io_service() const
	    {
		return this->io_service;
	    }

	    /**
	     * \brief Check whether the session is in state connected
	     *
	     * \returns true if the session is connected, false otherwise.
	     */
	    bool is_connected()
	    {
		return socket.is_open();
	    }

	    /**
	     * \brief Connect to the master agent.
	     *
	     * Note that upon creation of a session object, the connection is 
	     * automatically established. If the current state is "connected", 
	     * the function does nothing.
	     */
	    void connect();

	    /**
	     * \brief Shutdown the session.
	     *
	     * Disconnect from the master agent. Note that upon destruction of 
	     * a session object the session is automatically shutdown. If the 
	     * session is in state "disconnected", the function does nothing.
	     *
	     * \param reason The shutdown reason is reported to the master
	     *               agent during shutdown.
	     */
	    void disconnect(ClosePDU::reason_t reason=ClosePDU::reasonShutdown);

	    /**
	     * \brief Reconnect to the master agent.
	     *
	     * Disconnects, then connects to the master agent. If the status is 
	     * diconnected, the master is connected.
	     */
	    void reconnect();

	    /**
	     * \brief Get the sessionID of the session
	     *
	     * Get the session ID of the last established session, even if the 
	     * current state is "disconnected".
	     *
	     * \return The session ID of the last established session. If
	     *         object was never connected to the master, 0 is 
	     *         returned.
	     */
	    uint32_t get_sessionID()
	    {
		return sessionID;
	    }
    
	    /**
	     * \brief Default destructor
	     *
	     * The default destructor cleanly shuts down the session (if it is 
	     * currently established) and destroys the session object. It also 
	     * destroys the io_service object if it was created 
	     * automatically.
	     */
	    ~master_proxy();

    };
}

#endif
