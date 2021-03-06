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

bool Table::contains(QSharedPointer<TableEntry> entry) const
{
    return entries.contains(entry);
}

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
        QSharedPointer<AbstractVariable> var = iter.next();
        if(!var)
        {
            // No variable -> fail
            return false;
        }
        Oid variableIndex = var->toOid();
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
        if(! iter2.value())
        {
            // No variable given -> fail
            entries.remove(entry);
            return false;
        }
        // Add variable to list
        toRegister.append(qMakePair(myOid + entry->subid + iter2.key() + entryIndex, iter2.value()));
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
        toUnregister.append(myOid + entry->subid + iter2.key() + entryIndex);
    }
    myMasterProxy->removeVariables(toUnregister);

    // Remove entry from local storage
    entries.remove(entry);

    // All went well, as far as we can tell.
    return true;
}
