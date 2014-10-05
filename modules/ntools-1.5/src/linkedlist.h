/*************************************************************************

 linkedlist.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <iostream>
#include <pthread.h>
#include <list>

using namespace std;

/*
 This template class implements a thread-safe linked list. The list elements
 contain only pointers to user data, and not the data itself, for efficiency.
 It means that before each insert, you have to create a new data element
 with the new operator. The class supports thread specific cursors.
 This feature is enabled with the threadSafe() function. This will initialize
 the thread key that will be used for storing the thread specific cursors.
 (See the man page for pthread_key_create.) This function must be called in the
 main process before other threads are created. After that each thread will use
 it's own cursor for all operations.

 NOTE ABOUT THE REMOVE() AND CLEAR() FUNCTIONS:
 If you want to use these functions in multi-threaded application,
 and other threads may have access to the list during the process,
 then all cursor functions must be protected with mutex or read-write
 lock. This function removes an entry that can be pointed by other
 cursors. After deletion all cursors are checked, the ones pointing
 to the removed entry are updated (set to the next). It is possible
 however, that before the update another thread tries to use a cursor
 which points to the removed entry, which can cause the program to crash.

 Furthermore, keep in mind, that the data elements are not protected either,
 more threads can access the same entry, and get the pointer to the same
 data element.
*/

// the key cleanup function
static void key_delete( void *key )
{
	if( key ) delete ( void ** )key;
}

template<class T>
class LinkedList
{
 public:
	LinkedList();
	~LinkedList();
	void threadSafe();  // inits the thread key
	void begin();  // set cursor to 0, to end of the list ( to be used with next() )
	T *first();  // set cursor to first element, and get the entry
	T *get();  // get the actual entry at cursor
	T *next();  // set cursor to the next entry, and get it
					// at the end of the list returns 0, and after that loops to the first entry
	T *previous();  // set cursor to the previous entry, and get it
	T *last();  // set cursor the last element, and get the entry
	int isLast();  // returns TRUE if the cursor is on the last entry
	int end();  // returns TRUE if the cursor points to 0
	void insertBefore( T *);  // insert an enrty before the cursor, and set the cursor to the new entry
	void insertAfter( T * );  // insert an enrty after the cursor and set the cursor to the new entry
	void remove( int keep_data );  // remove the entry at the cursor, delete the data, and set cursor to the next entry
	void unlink();  // unlink the entry at the cursor, keep the data, and set cursor to the next entry
	void clear();  // clears the list
	int empty();  // returns TRUE if the list is empty
	int size();  // returns the size of the list
	void dump();  // print the content of the list
	int wrlock();  // write lock
	int rdlock();  // read lock
	int unlock();  // unlock
	
 private:
	pthread_key_t *tkey;
	struct ListEntry
	{
		T *data;
		struct ListEntry *prev;
		struct ListEntry *next;
	};
	ListEntry *head, *tail, *cursor;  // start, end, and global cursor
	int lsize;  // list size
	list<void *> cursors;  // list holding the thread specific cursors
	pthread_rwlock_t lock;  // read-write lock
};

template <class T>
LinkedList<T>::LinkedList()
{
	tkey = 0;
	lsize = 0;
	head = tail = cursor = 0;
	pthread_rwlock_init( &lock, 0 );
}

template <class T>
LinkedList<T>::~LinkedList()
{
	ListEntry *actual, *next;
	
	actual = head;
	while( actual != 0 )
	{
		next = actual->next;
		delete actual->data;
		delete actual;
		actual = next;
	}
}

template <class T>
void LinkedList<T>::threadSafe()
{
	if( tkey ) return;
	tkey = new pthread_key_t;
 	pthread_key_create( tkey, key_delete );
	lsize = 0;
	head = tail = cursor = 0;
}

template <class T>
void LinkedList<T>::begin()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	*curp = 0;
}

template <class T>
T *LinkedList<T>::first()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	*curp = head;
	if( !*curp ) return 0;
	else return (*curp)->data;
}

template <class T>
T *LinkedList<T>::get()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	if( !*curp ) return 0;  // no entry
	return (*curp)->data;
}

template <class T>
T *LinkedList<T>::next()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	if( !*curp ) *curp = head;
	else *curp = (*curp)->next;  // step the cursor
	if( !*curp ) return 0;  // no entry
	return (*curp)->data;
}

template <class T>
T *LinkedList<T>::previous()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	if( *curp == 0 ) return 0;
	else *curp = (*curp)->prev;  // step the cursor
	if( *curp == 0 ) return 0;  // no entry
	return (*curp)->data;
}

template <class T>
T *LinkedList<T>::last()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	*curp = tail;
	if( *curp == 0 ) return 0;
	return (*curp)->data;
}

template <class T>
int LinkedList<T>::size()
{
	return lsize;
}

template <class T>
void LinkedList<T>::insertBefore( T *newt )
{
	ListEntry *p, *prev;
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	p = new ListEntry;
	p->data = newt;
	if( lsize == 0 )  // if there is no entry
	{
		p->next = 0;
		p->prev = 0;  // no previous entry
		*curp = p;
		head = p;
		tail = p;
		lsize++;
	}
	else if( *curp == head )  // we are at the head entry
	{
		p->next = head;
		p->prev = 0;
		(*curp)->prev = p;
		head = p;
		*curp = p;
		lsize++;
	}
	else  // otherwise we are within or at the end of the list
	{
		prev = (*curp)->prev;
		prev->next = p;
		(*curp)->prev = p;
		p->prev = prev;
		p->next = *curp;
		*curp = p;
		lsize++;
	}
}

template <class T>
void LinkedList<T>::insertAfter( T *newt )
{
	ListEntry *p, *next;
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	p = new ListEntry;
	p->data = newt;
	if( lsize == 0 )  // if there is no entry
	{
		p->next = 0;
		p->prev = 0;  // no previous entry
		*curp = p;
		head = p;
		tail = p;
		lsize++;
	}
	else  // otherwise
	{
		if( *curp == 0 )  // if we are at the end of the list
		{
			*curp = tail;
		}
		if( (*curp)->next == 0 ) tail = p;
		next = (*curp)->next;
		p->next = next;
		if( next != 0 ) next->prev = p;
		p->prev = *curp;
		(*curp)->next = p;
		*curp = p;
		lsize++;
	}
}

template <class T>
int LinkedList<T>::end()
{
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	if( *curp == 0 ) return -1;
	else return 0;
}

template <class T>
int LinkedList<T>::isLast()
{
	ListEntry *p, *next;
	ListEntry **curp;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	if( *curp == 0 ) return 0;
	if( (*curp)->next == 0 ) return -1;
	return 0;
}

template <class T>
int LinkedList<T>::empty()
{
	if( lsize == 0 ) return -1;
	else return 0;
}

template <class T>
void LinkedList<T>::remove( int keep_data = 0 )
{
	ListEntry *prev, *next, *deleted;
	ListEntry **curp;
	list<void *>::iterator it;
	
	curp = &cursor;
	if( tkey )
	{
		curp = ( ListEntry ** )pthread_getspecific( *tkey );
		if( !curp )
		{
			curp = new ListEntry*;
			pthread_setspecific( *tkey, ( void * )curp );
		}
	}
	deleted = *curp;  // store the original pointer
	if( *curp == 0 )  // no entry to remove
	{
		return;
	}
	if( (*curp)->next == 0 ) tail = (*curp)->prev;
	if( *curp == head )  // this is the head entry
	{
		head = (*curp)->next;
		if( !keep_data ) delete (*curp)->data;
		delete *curp;
		*curp = head;
		if( *curp != 0 ) (*curp)->prev = 0;
		lsize--;
	}
	else
	{
		prev = (*curp)->prev;
		next = (*curp)->next;
		if( !keep_data ) delete (*curp)->data;
		delete *curp;
		if( prev != 0 ) prev->next = next;
		if( next != 0 ) next->prev = prev;
		*curp = next;
		lsize--;
	}
	// now update the cursors which point to the deleted entry
	for( it = cursors.begin(); it != cursors.end(); it++ )
	{
		if( *( ListEntry ** )( *it ) == deleted )
		{
			*( ListEntry ** )( *it ) = *curp;
		}
	}
}

template <class T>
void LinkedList<T>::unlink()
{
	return remove( -1 );
}

template <class T>
void LinkedList<T>::clear()
{
	list <void *>::iterator it;
	ListEntry *actual, *next;
	actual = head;
	while( actual != 0 )
	{
		next = actual->next;
		delete actual->data;
		delete actual;
		actual = next;
	}
	lsize = 0;
	head = tail = 0;
	// clear the cursors
	for( it = cursors.begin(); it != cursors.end(); it++ )
	{
		*( ListEntry ** )( *it ) = 0;
	}
}

template <class T>
void LinkedList<T>::dump()
{
	ListEntry *pt;
	
	cout << "List size: " << lsize << endl;
	pt = head;
	while( pt != 0 )
	{
		cout << *( pt->data ) << endl;
		pt = pt->next;
	}
}

template <class T>
int LinkedList<T>::wrlock()
{
	return pthread_rwlock_wrlock( &lock );
}

template <class T>
int LinkedList<T>::rdlock()
{
	return pthread_rwlock_rdlock( &lock );
}

template <class T>
int LinkedList<T>::unlock()
{
	return pthread_rwlock_unlock( &lock );
}

#endif
