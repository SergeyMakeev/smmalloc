// -*- C++ -*-

/*

  The Hoard Multiprocessor Memory Allocator
  www.hoard.org

  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
  Copyright (c) 1998-2012 Emery Berger
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/


#ifndef HOARD_LOCKMALLOCHEAP_H
#define HOARD_LOCKMALLOCHEAP_H

// Just lock malloc (unlike LockedHeap, which locks both malloc and
// free). Meant to be combined with something like RedirectFree, which will
// implement free.

namespace Hoard {

  template <typename Heap>
    class LockMallocHeap : public Heap {
  public:
    MALLOC_FUNCTION INLINE void * malloc (size_t sz) {
      HL::Guard<Heap> l (*this);
      return Heap::malloc (sz);
    }
  };

}

#endif
