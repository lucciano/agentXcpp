#include "ResponsePDU.h"
#include "helper.h"

using namespace agentxcpp;


void ResponsePDU::set_error(error_t e)
{
    /* We accept only allowed values: */
    switch(e)
    {
	// fall-through technology :-)
	case noAgentXError:
	case openFailed:
	case notOpen:
	case indexWrongType:
	case indexAlreadyAllocated:
	case indexNoneAvailable:
	case indexNotAllocated:
	case unsupportedContext:
	case duplicateRegistration:
	case unknownRegistration:
	case unknownAgentCaps:
	case parseError:
	case requestDenied:
	case processingError:
	    error = e;
	    break;
	default:
	    throw(inval_param());
    }
}


ResponsePDU::ResponsePDU() :
    PDU()
{
    this->error = noAgentXError;
    this->index = 0;
    this->sysUpTime = 0;
}



ResponsePDU::ResponsePDU(data_t::const_iterator& pos, bool big_endian)
    : PDU(pos, big_endian)
{
    // header is parsed by base class constructor

    uint16_t err;

    this->sysUpTime = read32(pos, big_endian);
    err = read16(pos, big_endian);
    this->index = read16(pos, big_endian);
    
    switch(err)
    {
	// fall-through technology :-)
	case noAgentXError:
	case openFailed:
	case notOpen:
	case indexWrongType:
	case indexAlreadyAllocated:
	case indexNoneAvailable:
	case indexNotAllocated:
	case unsupportedContext:
	case duplicateRegistration:
	case unknownRegistration:
	case unknownAgentCaps:
	case parseError:
	case requestDenied:
	case processingError:
	    this->error = static_cast<error_t>(err);
	    break;
	default:
	    throw(inval_param());
    }

}



data_t ResponsePDU::serialize()
{
    data_t serialized;

    // Encode reason and reserved fields
    write32(serialized, this->sysUpTime);
    write16(serialized, this->error); 
    write16(serialized, this->index); 

    // Add Header
    add_header(PDU::agentxClosePDU, serialized);

    return serialized;
}