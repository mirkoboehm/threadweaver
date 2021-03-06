/* -*- C++ -*-

   This file is part of ThreadWeaver.

   Author: Mirko Boehm
   Copyright: (C) 2005-2014 Mirko Boehm
   Contact: mirko@kde.org
         http://www.kde.org
         http://creative-destruction.me

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

#ifndef COMPUTETHUMBNAILJOB_H
#define COMPUTETHUMBNAILJOB_H

#include <ThreadWeaver/ThreadWeaver>

class Image;

namespace ThreadWeaver {
class ResourceRestrictionPolicy;
}

class ComputeThumbNailJob : public ThreadWeaver::Job
{
public:
    ComputeThumbNailJob(Image* image, ThreadWeaver::ResourceRestrictionPolicy* throttle);
    int priority() const override;
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override;

private:
    Image* m_image;

};

#endif // COMPUTETHUMBNAILJOB_H
