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

#include <iterator>

#include "Octet_String.h"

using namespace agentxcpp;

Octet_String::Octet_String(std::string v)
{
    // Here we convert initial value to a data_t string. We do this in three 
    // steps:
    // 1. get the bare data: v.data()
    // 2. cast the data to the value type of data_t (i.e. byte_t)
    // 3. calculate the size of the data
    //    - v.size() gives us the number of characters
    //    - sizeof(std::string::value_type) gives us the size of an character
    // 4. Assign v to value, giving it the bare data and its size
    //
    // This seems to be goofy, but it ensures that our code works even on a 
    // machine where a char is not 8 bit wide, i.e. when char has another size 
    // than byte_t.
    value.assign(
	    reinterpret_cast<const data_t::value_type*>( v.data() ),
	    v.size() * sizeof( std::string::value_type )
	        );
}

data_t Octet_String::serialize()
{
    data_t serialized;

    // encode size (big endian)
    int size = value.size();
    serialized.push_back(size >> 24 & 0xff);
    serialized.push_back(size >> 16 & 0xff);
    serialized.push_back(size >> 8 & 0xff);
    serialized.push_back(size >> 0 & 0xff);

    // encode value
    serialized += value;

    // Padding bytes
    int padsize = 4 - (size % 4);
    if( padsize == 4 ) padsize = 0; // avoid adding 4 padding bytes
    while( padsize-- )
    {
	serialized.push_back(0);
    }

    return serialized;
}


Octet_String::Octet_String(data_t::const_iterator& pos, bool big_endian)
{
    uint32_t size;

    // Get size
    if( big_endian )
    {
	size =  *pos++ << 24;
	size |= *pos++ << 16;
	size |= *pos++ << 8;
	size |= *pos++ << 0;
    }
    else
    {
	size =  *pos++ << 0;
	size |= *pos++ << 8;
	size |= *pos++ << 16;
	size |= *pos++ << 24;
    }
    
    // Octet String emtpy?
    if( size == 0 )
    {
	// nothing left to parse
	return;
    }

    // Get value
    value.assign(pos, pos+size);
    pos += size;
    
    // Eat padding bytes
    int padsize = 4 - (size % 4);
    if( padsize == 4 ) padsize = 0;
    while( padsize-- )
    {
	pos++;
    }
}
