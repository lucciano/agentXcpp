/*
 * Copyright 2011-2013 Tanjeff-Nicolai Moos <tanjeff@cccmz.de>
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

#include "Table.hpp"

using namespace agentxcpp;

bool Table::addEntry(QSharedPointer<TableEntry> entry)
{
    // Check for MasterProxy object
    if(! myMasterProxy)
    {
        return false;
    }

    // Ensure that index is not registered
    if(entries.contains(entry))
    {
        // The entry was already added
        return false;
    }

    // Calculate entry's index
    Oid entryIndex;
    QVector< QSharedPointer<AbstractVariable> > indexVariables = entry->indexVariables();
    QVectorIterator< QSharedPointer<AbstractVariable> > iter(indexVariables);
    while(iter.hasNext())
    {
        Oid variableIndex = iter.next()->toOid();
        if(variableIndex.is_null())
        {
            // Variable cannot be converted to Oid -> not
            // allowed as index variable.
            return false;
        }
        else
        {
            // Variable contributes to index
            entryIndex += variableIndex;
        }
    }

    // Register entry
    entries[entry] = entryIndex;

    // Register all variables of the entry with the MasterProxy object
    QMap< quint32, QSharedPointer<AbstractVariable> > variables = entry->variables();
    QMapIterator< quint32, QSharedPointer<AbstractVariable> > iter2(variables);
    QVector< QPair< Oid,QSharedPointer<AbstractVariable> > > toRegister;
    while(iter2.hasNext())
    {
        iter2.next();
        // Add variable to list
        toRegister.append(qMakePair(myOid + entryIndex + iter2.key(), iter2.value()));
    }
    myMasterProxy->addVariables(toRegister);

    // All went well, as far as we can tell.
    return true;
}


bool Table::removeEntry(QSharedPointer<TableEntry> entry)
{
    // Check for MasterProxy object
    if(! myMasterProxy)
    {
        return false;
    }

    // Ensure that entry is known
    if(!entries.contains(entry))
    {
        // Entry not found
        return false;
    }

    // Unregister all variables of the entry
    Oid entryIndex = entries[entry]; // Use index at time of registration
    QMap< quint32, QSharedPointer<AbstractVariable> > variables = entry->variables();
    QMapIterator< quint32, QSharedPointer<AbstractVariable> > iter2(variables);
    QVector<Oid> toUnregister;
    while(iter2.hasNext())
    {
        iter2.next();
        // Add variable's Oid to list
        toUnregister.append(myOid + entryIndex + iter2.key());
    }
    myMasterProxy->removeVariables(toUnregister);

    // Remove entry from local storage
    entries.remove(entry);

    // All went well, as far as we can tell.
    return true;
}
