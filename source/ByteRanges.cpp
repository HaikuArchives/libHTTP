#include "ByteRanges.h"

#include <stdlib.h>
#include <stdio.h>

ByteRangeSet::ByteRangeSet() { }


ByteRangeSet::ByteRangeSet(const char *setString)
{
	AddByteRange(setString);
}


ByteRangeSet::~ByteRangeSet()
{
	MakeEmpty();
}

void
ByteRangeSet::AddByteRange(ByteRangeSpec *range)
{
	AddByteRange(range->first, range->last);
}


void
ByteRangeSet::AddByteRange(int32 first, int32 last)
{
	ByteRangeSpec *s = new ByteRangeSpec();
	s->first = first;
	s->last = last;

	rangeSet.AddItem(s);
}


void
ByteRangeSet::AddByteRange(const char *setString)
{
	if (strncmp(setString, "bytes ", 6) != 0)
		return;

	setString += 6;
	int32 first;
	int32 last;
	
	if (sscanf(setString, "%d-%d", &first, &last) != 2) {
		return; // ignore
	}
	
	AddByteRange(first, last);
}


char *
ByteRangeSet::RangeString(char *buffer)
{
	*buffer = 0;
	strcat(buffer, "bytes ");
	char *bpointer = buffer + 6;
	for (int32 i = 0; i < rangeSet.CountItems(); i++) {
		bpointer += sprintf(bpointer, "%d-%d", GetRange(i)->first, GetRange(i)->last);
		if (i + 1 != rangeSet.CountItems())
			*(bpointer++) = ',';
	}
	return buffer;
}


char *
ByteRangeSet::ContentRangeString(char *buffer, int32 rangeIndex, int32 entityLength)
{
	strcpy(buffer, "bytes ");
	sprintf(buffer + 6, "%d-%d/%d",
		GetRange(rangeIndex)->first, GetRange(rangeIndex)->last, entityLength);
	return buffer;
}


int32
ByteRangeSet::ContentLength(int32 entityLength)
{
	int32 length = 0;

	for (int32 i = 0; i < rangeSet.CountItems(); i++) {
		ByteRangeSpec s = *GetRange(i);
		if (s.last > entityLength)
			s.last = entityLength;
		length += s.last - s.first;
	}
	
	return length;
}

int32
ByteRangeSet::CountRanges()
{
	return rangeSet.CountItems();
}


ByteRangeSpec *
ByteRangeSet::GetRange(int32 i)
{
	return (ByteRangeSpec *)rangeSet.ItemAt(i);
}


ByteRangeSpec *
ByteRangeSet::GetFileOffset(FileOffsetSpec *off, int32 length, int32 index)
{
	ByteRangeSpec *s = GetRange(index);
	off->offset = s->first;
	if (s->last > length)
		off->size = length - off->offset;
	else
		off->size = s->last - off->offset;

	return s;
}


void
ByteRangeSet::MakeEmpty()
{
	for (int32 i = 0; i < rangeSet.CountItems(); i++)
		delete GetRange(i);

	rangeSet.MakeEmpty();
}
