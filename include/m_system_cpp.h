/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef M_SYSTEM_CPP_H__
#define M_SYSTEM_CPP_H__ 1

#include "m_system.h"

#if defined(_UNICODE)
	#define tstring wstring
#else
	#define tstring string
#endif

#if defined(__cplusplus)

///////////////////////////////////////////////////////////////////////////////
// mir_ptr - automatic pointer for buffers, allocated using mir_alloc/mir_calloc

template<class T> class mir_ptr
{
	T* data;

public:
	__inline mir_ptr() : data((T*)mir_calloc(sizeof(T))) {}
	__inline mir_ptr(T* _p) : data(_p) {}
	__inline ~mir_ptr() { mir_free(data); }
	__inline T* operator = (T* _p) { if (data) mir_free(data); data = _p; return data; }
	__inline T* operator->() const { return data; }
	__inline operator T*() const { return data; }
	__inline operator INT_PTR() const { return (INT_PTR)data; }
};

///////////////////////////////////////////////////////////////////////////////
// mir_cslock - simple locker for the critical sections

class mir_cslock
{
	CRITICAL_SECTION& cs;

public:
	__inline mir_cslock(CRITICAL_SECTION& _cs) : cs(_cs) { EnterCriticalSection(&cs); }
	__inline ~mir_cslock() { LeaveCriticalSection(&cs); }
};

///////////////////////////////////////////////////////////////////////////////
// mir_cslockfull - controllable locker for the critical sections

class mir_cslockfull
{
	CRITICAL_SECTION& cs;
	bool bIsLocked;

public:
	__inline void lock() { bIsLocked = true; EnterCriticalSection(&cs); }
	__inline void unlock() { bIsLocked = false; LeaveCriticalSection(&cs); }

	__inline mir_cslockfull(CRITICAL_SECTION& _cs) : cs(_cs) { lock(); }
	__inline ~mir_cslockfull() { if (bIsLocked) unlock(); }
};

///////////////////////////////////////////////////////////////////////////////
// basic class for classes that should be cleared inside new()

class MZeroedObject
{
public:
	__inline void* operator new( size_t size )
	{	return calloc( 1, size );
	}
	__inline void operator delete( void* p )
	{	free( p );
	}
};

///////////////////////////////////////////////////////////////////////////////
// general lists' templates

#define	NumericKeySortT -1
#define	HandleKeySortT  -2
#define	PtrKeySortT     -3

template<class T> struct LIST
{
	typedef int (*FTSortFunc)(const T* p1, const T* p2);

	__inline LIST(int aincr, FTSortFunc afunc = NULL)
	{	memset(this, 0, sizeof(*this));
		increment = aincr;
		sortFunc = afunc;
	}

	__inline LIST(int aincr, INT_PTR id)
	{	memset(this, 0, sizeof(*this));
		increment = aincr;
		sortFunc = FTSortFunc(id);
	}

	__inline T* operator[](int idx) const { return (idx >= 0 && idx < count) ? items[idx] : NULL; }
	__inline int getCount(void)     const { return count; }
	__inline T** getArray(void)     const { return items; }

    __inline LIST(const LIST& x)
    {	 items = NULL;
	    List_Copy((SortedList*)&x, (SortedList*)this, sizeof(T));
    }

	__inline LIST& operator = (const LIST& x)
	{	destroy();
		List_Copy((SortedList*)&x, (SortedList*)this, sizeof(T));
		return *this;
	}

	__inline int getIndex(T* p) const
	{	int idx;
		return ( !List_GetIndex((SortedList*)this, p, &idx)) ? -1 : idx;
	}

	__inline void destroy(void)        { List_Destroy((SortedList*)this); }

	__inline T*  find(T* p)            { return (T*)List_Find((SortedList*)this, p); }
	__inline int indexOf(T* p)         { return List_IndexOf((SortedList*)this, p); }
	__inline int insert(T* p, int idx) { return List_Insert((SortedList*)this, p, idx); }
	__inline int remove(int idx)       { return List_Remove((SortedList*)this, idx); }

	__inline int insert(T* p)          { return List_InsertPtr((SortedList*)this, p); }
	__inline int remove(T* p)          { return List_RemovePtr((SortedList*)this, p); }

protected:
	T**        items;
	int        count, limit, increment;
	FTSortFunc sortFunc;
};

template<class T> struct OBJLIST : public LIST<T>
{
	typedef int (*FTSortFunc)(const T* p1, const T* p2);

	__inline OBJLIST(int aincr, FTSortFunc afunc = NULL) :
		LIST<T>(aincr, afunc)
		{}

	__inline OBJLIST(int aincr, INT_PTR id) :
		LIST<T>(aincr, (FTSortFunc) id)
		{}

	__inline OBJLIST(const OBJLIST& x) :
		LIST<T>(x.increment, x.sortFunc)
		{	items = NULL;
			List_ObjCopy((SortedList*)&x, (SortedList*)this, sizeof(T));
		}

	__inline OBJLIST& operator = (const OBJLIST& x)
		{	destroy();
			List_ObjCopy((SortedList*)&x, (SortedList*)this, sizeof(T));
			return *this;
		}

	~OBJLIST()
	{
		destroy();
	}

	__inline void destroy(void)
	{
		for (int i=0; i < this->count; i++)
			delete this->items[i];

		List_Destroy((SortedList*)this);
	}

	__inline int remove(int idx) {
		delete this->items[idx];
		return List_Remove((SortedList*)this, idx);
	}

	__inline int remove(T* p)
	{
		if (List_RemovePtr((SortedList*)this, p) != -1) {
			delete p;
			return 1;
		}
		return 0;
	}

	__inline T& operator[](int idx) const { return *this->items[idx]; }
};

#endif

#endif // M_SYSTEM_CPP_H
