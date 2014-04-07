/* -*- C++ -*-

This file implements the Job class.

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

$Id: Job.cpp 20 2005-08-08 21:02:51Z mirko $
*/

#include "job.h"
#include "job_p.h"

#include <QtCore/QList>
#include <QtCore/QMutex>
#include "debuggingaids.h"
#include "thread.h"
#include <QAtomicPointer>
#include <QAtomicInt>

#include "queuepolicy.h"
#include "dependencypolicy.h"
#include "executor_p.h"
#include "executewrapper_p.h"
#include "managedjobpointer.h"
#include "exception.h"

namespace ThreadWeaver
{

Job::Job()
    : d_(new Private::Job_Private())
{
#if !defined(NDEBUG)
    d()->debugExecuteWrapper.wrap(setExecutor(&(d()->debugExecuteWrapper)));
#endif
    d()->status.storeRelease(Status_New);
}

Job::Job(Private::Job_Private *d__)
    : d_(d__)
{
#if !defined(NDEBUG)
    d()->debugExecuteWrapper.wrap(setExecutor(&(d()->debugExecuteWrapper)));
#endif
    d()->status.storeRelease(Status_New);
}

Job::~Job()
{
    for (int index = 0; index < d()->queuePolicies.size(); ++index) {
        d()->queuePolicies.at(index)->destructed(this);
    }
    delete d_;
}

void Job::execute(const JobPointer& self, Thread *th)
{
    Executor *executor = d()->executor.loadAcquire();
    Q_ASSERT(executor); //may never be unset!
    Q_ASSERT(self);
    executor->begin(self, th);
    self->setStatus(Status_Running);
    try {
        executor->execute(self, th);
        if (self->status() == Status_Running) {
            self->setStatus(Status_Success);
        }
    } catch (JobAborted &) {
        self->setStatus(Status_Aborted);
    } catch (JobFailed &) {
        self->setStatus(Status_Failed);
    } catch (...) {
        TWDEBUG(0, "Uncaught exception in Job %p, aborting.", self.data());
        throw;
    }
    Q_ASSERT(self->status() > Status_Running);
    executor->end(self, th);
    executor->cleanup(self, th);
}

void Job::blockingExecute()
{
    execute(ManagedJobPointer<Job>(this), 0);
}

Executor *Job::setExecutor(Executor *executor)
{
    return d()->executor.fetchAndStoreOrdered(executor == 0 ? &Private::defaultExecutor : executor);
}

Executor *Job::executor() const
{
    return d()->executor.loadAcquire();
}

int Job::priority() const
{
    return 0;
}

void Job::setStatus(JobInterface::Status status)
{
    d()->status.storeRelease(status);
}

JobInterface::Status Job::status() const
{
    // since status is set only through setStatus, this should be safe:
    return static_cast<Status>(d()->status.loadAcquire());
}

bool Job::success() const
{
    return d()->status.loadAcquire() == Status_Success;
}

void Job::defaultBegin(const JobPointer&, Thread *)
{
}

void Job::defaultEnd(const JobPointer& job, Thread *)
{
    d()->freeQueuePolicyResources(job);
}

void Job::aboutToBeQueued(QueueAPI *api)
{
    QMutexLocker l(mutex()); Q_UNUSED(l);
    aboutToBeQueued_locked(api);
}

void Job::aboutToBeQueued_locked(QueueAPI *)
{
}

void Job::aboutToBeDequeued(QueueAPI *api)
{
    QMutexLocker l(mutex()); Q_UNUSED(l);
    aboutToBeDequeued_locked(api);
}

void Job::aboutToBeDequeued_locked(QueueAPI *)
{
}

void Job::assignQueuePolicy(QueuePolicy *policy)
{
    Q_ASSERT(!mutex()->tryLock());
    if (! d()->queuePolicies.contains(policy)) {
        d()->queuePolicies.append(policy);
    }
}

void Job::removeQueuePolicy(QueuePolicy *policy)
{
    Q_ASSERT(!mutex()->tryLock());
    int index = d()->queuePolicies.indexOf(policy);
    if (index != -1) {
        d()->queuePolicies.removeAt(index);
    }
}

QList<QueuePolicy *> Job::queuePolicies() const
{
    Q_ASSERT(!mutex()->tryLock());
    return d()->queuePolicies;
}

/** @brief Set a status property on the job.
 *
 * To implement arbitrary status properties without incurring implemention overhead in the base class,
 * it is possible to set arbitrary status properties in the form of QVariants on a job. The base class
 * implementation will ignore them, leaving the actual handling of the status to decorators of the job.
 * The Job class itself never calls this method.
 */
void Job::setStatusProperty(int, const QVariant&)
{
    //empty
}

/** @brief Set the name of the job for status reporting.
 *
 * The base class implementation is empty, leaving the actual handling of the name to decorators of the
 * job. The Job class itself never calls this method.
 */
void Job::setName(const QString&)
{
    //empty
}

/** @brief Set the description of the job for status reporting.
 *
 * The base class implementation is empty, leaving the actual handling of the description to decorators of the
 * job. The Job class itself never calls this method.
 */
void Job::setDescription(const QString&)
{
    //empty
}

/** @brief Set the progress of the job for status reporting.
 *
 * Progress status is made up of three attributes: the current progress value, a total and a weight. Progress
 * starts counting at zero. A value of zero or less will be represented as zero percent, any value of total or
 * more as 100 percent.
 *
 * The total is an arbitrary number larger than 0. The completion percentage is calculated as value divided by
 * total.
 *
 * Weight is only relevant when completion is calculated across multiple jobs, for example as part of a
 * sequence. It represents the cost or contribution of the job to the group. A weight value of 1 is the default.
 * If all jobs of the respective group have a weight value of 1, they all contribute the same progress to the
 * group. If a job in the group has a weight value of 10, it completion is supposed to have 10 times the impact
 * on overall progress as for a job with weight 1.
 *
 * The base class implementation is empty, leaving the actual handling of the name to decorators of the
 * job. The Job class itself never calls this method.
 */
void Job::setProgress(int, int, int)
{
    //empty
}

/** @brief Retrieve a status property for the job.
 *
 * The base class implementation returns an invalid QVariant, since it does not store status properties. It is
 * up to status handling decorators to implement this method.
 */
QVariant Job::statusProperty(int)
{
    return QVariant();
}

Private::Job_Private *Job::d()
{
    return d_;
}

const Private::Job_Private *Job::d() const
{
    return d_;
}

bool Job::isFinished() const
{
    const Status s = status();
    return s == Status_Success || s == Status_Failed || s == Status_Aborted;
}

QMutex *Job::mutex() const
{
    return &(d()->mutex);
}

}

#include "managedjobpointer.h"
