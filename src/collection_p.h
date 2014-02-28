/* -*- C++ -*-

This file is part of ThreadWeaver.

$ Author: Mirko Boehm $
$ Copyright: (C) 2004-2013 Mirko Boehm $
$ Contact: mirko@kde.org
http://www.kde.org
http://creative-destruction.me $

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef COLLECTION_COLLECTION_P_H
#define COLLECTION_COLLECTION_P_H

#include <QVector>
#include <QMutex>

#include "executewrapper_p.h"

namespace ThreadWeaver {

class Collection;

class CollectionSelfExecuteWrapper : public ThreadWeaver::ExecuteWrapper
{
public:
    void begin(JobPointer, Thread *) Q_DECL_OVERRIDE;
    void end(JobPointer, Thread *) Q_DECL_OVERRIDE;
};

class Collection_Private
{
public:
    Collection_Private();
    ~Collection_Private();

    /** Dequeue all elements of the collection.
     * Note: This will not dequeue the collection itself.
     */
    void dequeueElements(Collection* collection, bool queueApiIsLocked);

    /** Perform the task usually done when one individual job is
     * finished, but in our case only when the whole collection
     * is finished or partly dequeued.
     */
    void finalCleanup(Collection* collection);

    /* The elements of the collection. */
    QVector<JobPointer> elements;

    /* The Weaver interface this collection is queued in. */
    QueueAPI *api;

    /* Counter for the finished jobs.
       Set to the number of elements when started.
       When zero, all elements are done.
    */
    QAtomicInt jobCounter;
    QAtomicInt jobsStarted;
    CollectionSelfExecuteWrapper selfExecuteWrapper;
    JobPointer self;
    bool selfIsExecuting;
};

}

#endif // COLLECTION_COLLECTION_P_H
