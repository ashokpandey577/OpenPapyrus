// SSTACK.CPP
// Copyright (c) Sobolev A. 1995..2000, 2002, 2003, 2007, 2008, 2010, 2016, 2017, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
// Definitions same as in sarray.cpp
//
#define _USERPTR(ptr) ((flags&aryPtrContainer)?(*(void **)(ptr)):(ptr))
#define _INTRPTR(ptr) ((flags&aryPtrContainer)?&(ptr):(ptr))
#define _BDATA        PTR8(data)

SLAPI SStack::SStack(uint itemSize, uint o) : SArray(itemSize, o)
{
	pointer = 0;
	// @v9.8.1 {
	if(flags & aryRing/* && delta*/) {
		// @v9.8.4 expand(/*delta*/32);
	}
	// } @v9.8.1 
}

int FASTCALL SStack::push(const void * pItem)
{
	int    ok = 0;
	if(pointer < count) {
		if(flags & aryEachItem) {
			void * p_item = _BDATA + pointer * isize;
			freeItem(_USERPTR(p_item));
		}
		atPut(pointer++, pItem);
		ok = 1;
	}
	else if(flags & aryRing && Limit && count >= Limit) {
		//
		// ��� ���������� ����� ��� ���������� ������� ������ (������ ������
		// �������� ���������� delta) �������� �������� �������� � ������ �������.
		//
		assert(count == Limit);
		if(flags & aryEachItem) {
			void * p_item = _BDATA + pointer * isize;
			freeItem(_USERPTR(p_item));
		}
		atPut(0, pItem);
		pointer = 1;
		ok = 2;
	}
	else if(insert(pItem)) {
		pointer++;
		ok = 1;
	}
	return ok;
}

void * SLAPI SStack::pop()
{
	if(pointer)
		return at(--pointer);
	else if(flags & aryRing && Limit && count >= Limit) {
		//
		// ��� ���������� ����� ��� ���������� ������� ������� �������
		// (���� ������ ����������� ����� ��� �� 0 ���������) ���������
		// ���������� ������� �� ����� �������.
		// @attention ����� ������� ��������� ���� ����� �������� ��
		//   ���������� ��������, ���� ����������� ������� �����.
		//
		assert(count == Limit);
		pointer = Limit-1;
		return at(pointer);
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, (void *)0);
}

int FASTCALL SStack::pop(void * pItem)
{
	if(pointer) {
		pointer--;
		if(pItem)
			memcpy(pItem, at(pointer), isize);
		return 1;
	}
	else if(flags & aryRing && Limit && count >= Limit) {
		assert(count == Limit);
		pointer = Limit-1;
		if(pItem)
			memcpy(pItem, at(pointer), isize);
		return 2;
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, 0);
}

int SLAPI SStack::undo(uint ptr)
{
	return checkupper(ptr, count+1) ? ((pointer = ptr), 1) : 0;
}

void * SLAPI SStack::peek() const
{
	if(pointer)
		return at(pointer-1);
	else if(flags & aryRing && Limit && count >= Limit) {
		assert(count == Limit);
		return at(Limit-1);
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, 0);
}
//
//
//
SLAPI SQueue::SQueue(uint itemSize, uint queueSize, uint o) : SArray(itemSize, /*8,*/ o), QueueSize(queueSize), Front(0), End(0)
{
}

SLAPI SQueue::SQueue(const SQueue & rS) : SArray(rS), QueueSize(rS.QueueSize), Front(rS.Front), End(rS.End)
{
}

int FASTCALL SQueue::copy(const SQueue & src)
{
	QueueSize = src.QueueSize;
	Front     = src.Front;
	End       = src.End;
	return SArray::copy(src);
}

void SLAPI SQueue::clear()
{
	SArray::clear();
	Front = End = 0;
}

int FASTCALL SQueue::push(const void * pItem)
{
	if(count < QueueSize) {
		if(!atInsert(End % QueueSize, pItem))
			return 0;
	}
	else {
		atPut(End % QueueSize, pItem);
	}
	End++;
	if((End - Front) > QueueSize)
		Front++;
	//
	// Normalyze values of Front and End
	//
	uint   f = Front / QueueSize;
	if(f > 1) {
		End -= (f-1) * QueueSize;
		Front -= (f-1) * QueueSize;
	}
	return 1;
}

void * SLAPI SQueue::pop()
{
	void * p_item;
	if(Front < End) {
		p_item = at(Front % QueueSize);
		Front++;
	}
	else
		p_item = 0;
	return p_item;
}

void * SLAPI SQueue::peek() const
{
	return (Front < End) ? at(Front % QueueSize) : 0;
}

uint SLAPI SQueue::getNumItems() const
{
	return (End - Front);
}

void * SLAPI SQueue::get(uint p) const
{
	return checkupper(p, End - Front) ? at((Front + p) % QueueSize) : 0;
}

#if 0 // {

SStack

class SStackRing

#endif // } 0
