/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdio.h>

#include <libcpphaggle/Heap.h>

namespace haggle {

HeapItem::HeapItem() : index(npos), active(false) 
{	
}
	
HeapItem::~HeapItem() 
{
}
	
void HeapItem::enable() 
{
	active = true;
}

void HeapItem::disable() 
{
	active = false;
}

	
Heap::Heap(unsigned long max_size) : _max_size(max_size), _size(0), heap(new HeapItem*[max_size]) 
{
}
	
Heap::~Heap() 
{ 
	delete [] heap; 
}

bool Heap::empty() const
{ 
	return (_size == 0); 
}

bool Heap::full() const
{ 
	return (_size >= _max_size); 
}

HeapItem *Heap::front()
{ 
	return heap[0]; 
}

unsigned long Heap::size() const 
{ 
	return _size; 
}
	
void Heap::heapify(unsigned long i)
{
	unsigned long l, r, smallest;
	HeapItem *tmp;

	l = (2 * i) + 1;	/* left child */
	r = l + 1;		/* right child */

	if ((l < _size) && (*heap[l] < *heap[i]))
		smallest = l;
	else
		smallest = i;

	if ((r < _size) && (*heap[r] < *heap[smallest]))
		smallest = r;

	if (smallest == i)
		return;

	/* exchange to maintain heap property */
	tmp = heap[smallest];
	heap[smallest] = heap[i];
	heap[smallest]->index = smallest;
	heap[i] = tmp;
	heap[i]->index = i;
	heapify(smallest);
}

bool Heap::increaseSize(unsigned long increase_size)
{
	HeapItem **new_heap;

	new_heap = new HeapItem *[_max_size + increase_size];

	if (!new_heap) {
		return false;
	}

	memcpy(new_heap, heap, _size * sizeof(HeapItem *));

	delete[] heap;

	heap = new_heap;

	_max_size += increase_size;

	return true;
}

bool Heap::insert(HeapItem *item)
{
	unsigned long i, parent;

	if (full()) {
		if (increaseSize() == false) {
			fprintf(stderr, "Heap is full and could not increase heap size, _size=%lu\n", _size);
			return false;
		}
	}

	i = _size;
	parent = (i - 1) / 2;

	/* find the correct place to insert */
	while ((i > 0) && (*heap[parent] > *item)) {
		heap[i] = heap[parent];
		heap[i]->index = i;
		i = parent;
		parent = (i - 1) / 2;
	}
	heap[i] = item;
	item->index = i;
	_size++;

	return true;
}

HeapItem *Heap::extractFirst(void)
{
	HeapItem *max;

	if (empty())
		return NULL;

	max = heap[0];
	_size--;
	heap[0] = heap[_size];
	heap[0]->index = 0;
	heapify(0);

	return max;
}

	
bool operator< (const HeapItem& i1, const HeapItem& i2)
{
	return i1.compare_less(i2);
}

bool operator> (const HeapItem& i1, const HeapItem& i2)
{
	return i1.compare_greater(i2);
}
	
void Heap::pop_front()
{
	extractFirst();
}

// CBMEN, HL, Begin
HeapItem **Heap::getHeapItems() 
{
	HeapItem ** array = new HeapItem *[_max_size];

	if (!array) {
		return NULL;
	}

	for (size_t i = 0; i < _size; i++) {
		array[i] = heap[i];
	}

	return array;
}

HeapItem **Heap::getHeapArray()
{
	return heap;
}
// CBMEN, HL, End

}; // namespace haggle
