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
#ifndef _VARBIND_H_
#define _VARBIND_H_

#include <istream>

#include <QSharedPointer>
#include <QtGlobal>

#include "Oid.hpp"
#include "AbstractVariable.hpp"

namespace agentxcpp
{
    /**
     * \internal
     *
     * \brief Represents a VarBind according to RFC 2741, section 5.4.
     */
    class Varbind
    {
	private:
	    /**
	     * \brief The name (OID) of the VarBind.
	     */
	    Oid name;

	    /**
	     * \brief The variable inside the varbind.
	     *
	     * This pointer may be 0 if the varbind has a type without a 
	     * variable (e.g. "NoSuchObject").
	     */
	    QSharedPointer<AbstractVariable> var;

	    /**
	     * \brief The type of the varbind.
	     *
	     * This field represents the type as described in RFC 2741, section 
	     * 5.4. The serialize() function will copy it directly into the 
	     * varbind.
	     */
	    quint16 type;
	public:

	    /**
	     * \brief Create a VarBind with an oid and a var.
	     *
	     * The variable must be one of the following types:
	     * - IntegerValue
	     * - OctetStringValue
	     * - OidVariable
	     * - IpAddressValue
	     * - Counter32Value
	     * - Gauge32Value
	     * - TimeTicksValue
	     * - OpaqueValue
	     * - Counter64Value
	     * If the type of the variable cannot be determined, inval_param is 
	     * thrown.
	     */
	    Varbind(const Oid&, QSharedPointer<AbstractVariable> v);
	    
	    /**
	     * \brief These values can be used to create a VarBind.
	     */
	    enum type_t {
		Null = 5,
		noSuchObject = 128,
		noSuchInstance = 129,
		endOfMibView = 130
	    };


	    /**
	     * \brief Create VarBind without var, but with a type.
	     *
	     * Only the constants defined by varbind::type_t are allowed.  A 
	     * wrong type will cause an inval_param exception.
	     */
	    Varbind(const Oid&, type_t);

	    /**
	     * \internal
	     *
	     * \brief Construct the object from input stream
	     *
	     * This constructor parses the serialized form of the object.
	     * It takes an iterator, starts parsing at the position of the 
	     * iterator and advances the iterator to the position right behind 
	     * the object.
	     * 
	     * The constructor expects valid data from the stream; if parsing 
	     * fails, parse_error is thrown. In this case, the iterator 
	     * position is undefined.
	     *
	     * \param pos Iterator pointing to the current stream position.
	     *            The iterator is advanced while reading the header.
	     *
	     * \param end Iterator pointing one element past the end of the
	     *            current stream. This is needed to mark the end of the 
	     *            buffer.
	     *
	     * \param big_endian Whether the input stream is in big endian
	     *                   format
	     */
	    Varbind(binary::const_iterator& pos,
		    const binary::const_iterator& end,
		    bool big_endian=true);

            /**
             * \brief Get the name (the OID) stored within the varbind.
             */
            Oid get_name() const
            {
                return name;
            }

            /**
             * \brief Get the variable stored within the varbind.
             *
             * \note This returns a smart pointer to the variable, i.e. the
             *       variable can be modified in-place.
             */
            QSharedPointer<AbstractVariable> get_var() const
            {
                return var;
            }

            /**
	     * \internal
	     *
	     * \brief Serialize the varbind.
	     *
	     * This creates the binary representation of the varbind.
	     */
	    binary serialize() const;

    };

}
#endif
