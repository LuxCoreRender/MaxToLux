/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/


/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
* 
* Custom Iterator, Array and Stack implementations to replace std::vector.
*********************************************************************************************************************************/

#pragma once
//#include "Common.h"
#include <sstream>
#include <max.h>
#include <string>
#include <algorithm>
// We use these to put all our code in a namespace. This prevents name clashes with 3ds Max API which tends to define some very 
// common words as its classes.
#define LUXCORE_NAMESPACE_BEGIN namespace MaxToLux {
#define LUXCORE_NAMESPACE_END }

LUXCORE_NAMESPACE_BEGIN;

/// Single iterator class for our custom arrays, allowing to use them with STL algorithms
template<typename IterType>
class Iterator
{
protected:

    IterType* ptr;

#ifdef FIREMAX_DEBUG
    int currentPos;
    int maxPos;
#endif

public:

    /// Constructs iterator for "array" pointing to position "ptr"
    template<typename TArray>
    inline Iterator(TArray* array, const size_t ptr)
	{
        this->ptr = array->ptr() + ptr;
#ifdef FIREMAX_DEBUG
        this->currentPos = int(ptr);
        this->maxPos = array->size();
#endif
    }

    /// allows to use pointer to single value where iterator is expected (acts as array of length 1)
    inline explicit Iterator(IterType* ptr) {
        this->ptr = ptr;
#ifdef FIREMAX_DEBUG
        this->currentPos = 0;
        this->maxPos = 1;
#endif
    }

    /// Constructs iterator from simple pointer (as opposed to an array object). Size has to be explicitly provided
    inline Iterator(IterType* ptr, const size_t offset, const size_t arraySize) {
        this->ptr = ptr + offset;
#ifdef FIREMAX_DEBUG
        this->currentPos = int(offset);
        this->maxPos = int(arraySize);
#endif
    }

    inline Iterator() {}

    inline operator Iterator<const IterType>() const {
        return *(Iterator<const IterType>*)this;
    }

    inline bool operator==(const Iterator& second) const {
#ifdef FIREMAX_DEBUG
        //FASSERT(this->maxPos == second.maxPos && this->ptr - this->currentPos == second.ptr - second.currentPos);
#endif
        return ptr == second.ptr;
    }

    inline bool operator!=(const Iterator& other) const {
        return !((*this) == other);
    }

    inline bool operator<(const Iterator& second) const {
#ifdef FIREMAX_DEBUG
        //FASSERT(this->maxPos == second.maxPos && this->ptr - this->currentPos == second.ptr - second.currentPos);
#endif
        return ptr < second.ptr;
    }

    inline size_t operator-(const Iterator& second) const {
#ifdef FIREMAX_DEBUG
        //FASSERT(this->maxPos == second.maxPos && this->ptr - this->currentPos == second.ptr - second.currentPos);
#endif
        return ptr - second.ptr;
    }

    inline IterType& data() {
#ifdef FIREMAX_DEBUG
        //FASSERT(unsigned(currentPos) < unsigned(this->maxPos));
#endif
        return *ptr;
    }

    inline IterType& operator*() {
        return data();
    }


    inline const IterType& data() const {
        //FASSERT(unsigned(currentPos) < unsigned(this->maxPos));
        return *ptr;
    }

    inline const IterType& operator*() const {
        return data();
    }

    inline IterType& operator[](const int offset) {
        //FASSERT(unsigned(currentPos + offset) < unsigned(this->maxPos));
        return ptr[offset];
    }

    inline const IterType& operator[](const int offset) const {
        //FASSERT(unsigned(currentPos + offset) < unsigned(this->maxPos));
        return ptr[offset];
    }

    inline IterType* operator->() {
        //FASSERT(unsigned(currentPos) < unsigned(this->maxPos));
        return (&**this);
    }

    inline Iterator& operator++() {
#ifdef FIREMAX_DEBUG
        ++currentPos;
#endif
        ++ptr;
        return *this;
    }

    inline Iterator operator++(int) {
        Iterator temp = *this;
#ifdef FIREMAX_DEBUG
        ++currentPos;
#endif
        ++ptr;
        return temp;
    }

    inline Iterator& operator--() {
#ifdef FIREMAX_DEBUG
        --currentPos;
#endif
        --ptr;
        return *this;
    }

    inline Iterator operator--(int) {
        Iterator temp = *this;
#ifdef FIREMAX_DEBUG
        --currentPos;
#endif
        --ptr;
        return temp;
    }

    inline Iterator& operator+=(const size_t x) {
#ifdef FIREMAX_DEBUG
        currentPos += int(x);
#endif
        ptr += x;
        return *this;
    }

    inline Iterator operator+(const size_t x) const {
        Iterator res(*this);
#ifdef FIREMAX_DEBUG
        res.currentPos += int(x);
#endif
        res.ptr += x;
        return res;
    }

    inline Iterator operator-(const size_t x) const {
        Iterator res(*this);
#ifdef FIREMAX_DEBUG
        res.currentPos -= int(x);
#endif
        res.ptr -= x;
        return res;
    }

    // STL black magic
    typedef std::random_access_iterator_tag iterator_category;
    typedef IterType value_type;
    typedef size_t difference_type;
    typedef IterType* pointer;
    typedef IterType& reference;
};



template<typename Type> class Stack;

/// Safe encapsulation of a dynamic array. Very similar to std::vector, but faster and safer (has better bounds checking)
template <typename Type>
class Array
{
    template<typename Type> friend class Iterator;
    template<typename Type> friend class ::MaxToLux::Stack; // namespace here is needed because of naming clashes with 3ds Max
protected:

    Type* data;

    int capacity;

    int stackStored;

    inline Type* ptr()
	{
        return data;
    }

    inline const Type* ptr() const
	{
        return data;
    }

public:

    /// Default constructor, creates array with zero size. No memory is allocated.
    inline Array() : data(NULL), capacity(0) {}

    /// Performs deep copy
    Array(const Array& second)
	{
        this->capacity = second.capacity;
        if (this->capacity > 0)
		{
            this->data = new Type[capacity];
            for (int i = 0; i < capacity; ++i)
			{
                this->data[i] = second.data[i];
            }
        } else
		{
            this->data = NULL;
        }
    }

    /// Assignment operator, performs deep copy of second array to this array.
    Array& operator= (const Array& second) {
        //FASSERT((capacity == 0 || data) && this != &second);
        if (this->capacity != second.capacity) {
            delete[] this->data;            
            this->capacity = second.capacity;
            if (this->capacity == 0) {
                this->data = NULL;
            } else {
                this->data = new Type[capacity];
                for (int i = 0; i < capacity; ++i) {
                    this->data[i] = second.data[i];
                }
            }
        } else {
            for (int i = 0; i < capacity; ++i) {
                this->data[i] = second.data[i];
            }
        }
        //FASSERT(capacity == 0 || data);
        return *this;
    }

    /// Constructs new array with given capacity
    explicit Array(const int capacity) {
        //FASSERT(capacity >= 0);
        this->capacity = capacity;
        if (this->capacity > 0) {
            this->data = new Type[capacity];
        } else {
            this->data = NULL;
        }
    }

#if _MSC_VER >= 1800
    /// Constructs array from an initializer list, only supported in MSVS 2013+
    Array(std::initializer_list<Type> list) {
        this->capacity = int(list.size());
        if (this->capacity > 0) {
            auto ptr = list.begin();
            this->data = new Type[capacity];
            for (int i = 0; i < capacity; ++i) {
                this->data[i] = *ptr++;
            }
        } else {
            this->data = NULL;
        }
    }
#endif

    ~Array() {
        dealloc();
    }

    inline const Type& operator[](const int index) const {
        return this->get(index);
    }

    inline Type& operator[](const int index) {
        return this->get(index);
    }

    inline const Type& get(const int index) const {
        //FASSERT(unsigned(index) < unsigned(size()));
        return this->data[index];
    }

    inline Type& get(const int index) {
        //FASSERT(unsigned(index) < unsigned(size()));
        return this->data[index];
    }

    /// Resizes the array to new size. When preserveOld is true, elements 0 - min(new size, old size) get safely copied, 
    /// otherwise their content is undefined
    void resize(const int newSize, const bool preserveOld)
	{
        //FASSERT(newSize >= 0);
        if (newSize == this->capacity)
            return;
		else if (newSize == 0)
            dealloc();
		else
		{
            Type* newData = new Type[newSize];
            const int limit = std::min(newSize, capacity) * preserveOld;
            for (int i = 0; i < limit; i++)
                newData[i] = this->data[i];
            delete[] this->data;            
            this->data = newData;
            this->capacity = newSize;
        }
    }

    /// deallocates the internal storage, freeing memory, and setting size to 0.
    void dealloc() {
        delete[] this->data;            
        this->data = NULL;
        this->capacity = 0;
    }

    inline int size() const {
        return this->capacity;
    }

    /// Returns constant iterator pointing at the first element of the array
    inline Iterator<const Type> cbegin() const {
        return Iterator<const Type>(this, 0);
    }

    /// Returns constant iterator pointing past the last element of the array
    inline Iterator<const Type> cend() const {
        return Iterator<const Type>(this, size());
    }

    /// Returns iterator pointing at the first element of the array
    inline Iterator<Type> begin() {
        return Iterator<Type>(this, 0);
    }

    /// Returns iterator pointing past the last element of the array
    inline Iterator<Type> end() {
        return Iterator<Type>(this, size());
    }
    inline Iterator<const Type> begin() const {
        return Iterator<const Type>(this, 0);
    }

    inline Iterator<const Type> end() const {
        return Iterator<const Type>(this, size());
    }

    /// Dereferences the last element in the array
    inline const Type& back() const {
        return (*this)[size() - 1];
    }

    /// Dereferences the first element in the array
    inline const Type& front() const {
        return (*this)[0];
    }

    /// Dereferences the last element in the array
    inline Type& back() {
        return (*this)[size() - 1];
    }

    /// Dereferences the first element in the array
    inline Type& front() {
        return (*this)[0];
    }

    /// Swaps the content of this and other array, without actually copying the data
    void swap(Array& other) {
        std::swap(this->data, other.data);
        std::swap(this->capacity, other.capacity);
    }
};

template <typename Type>
/// Improvement on the Array class, which allows pushing/erasing elements.
class Stack {
    template<typename Type> friend class Iterator;
protected:

    Array<Type> data;

    inline Type* ptr() {
        return this->data.ptr();
    }
    inline const Type* ptr() const {
        return this->data.ptr();
    }

public:

    /// Creates new instance with empty storage
    inline Stack() {
        this->data.stackStored = 0;
    }
    inline Stack(const Stack& other) : data((other.data)) {
        this->data.stackStored = other.data.stackStored;
    }
    inline Stack& operator=(const Stack& other) {
        //FASSERT(this != &other);
        this->data = other.data;
        this->data.stackStored = other.data.stackStored;
        return *this;
    }

    inline const Type& operator[](const int index) const {
        return this->get(index);
    }

    inline Type& operator[](const int index) {
        return this->get(index);
    }

    inline const Type& get(const int index) const {
        //FASSERT(unsigned(index) < unsigned(this->size()));
        return this->data[index];
    }

    inline Type& get(const int index) {
        //FASSERT(unsigned(index) < unsigned(this->size()));
        return this->data[index];
    }

    /// Clears all entries (but allocated storage is preserved)
    inline void clear() {
        data.stackStored = 0;
    }

    /// Clears all entries and deletes allocated memory
    void dealloc() {
        data.stackStored = 0;
        data.dealloc();
    }

    /// Tells if this stack is empty
    inline bool isEmpty() const {
        return size() == 0;
    }

    inline int size() const {
        return data.stackStored;
    }

    /// Pushes single element onto the stack. Reallocates internal storage if needed (with O(1) amortized time complexity)
    inline void push(const Type& item = Type()) {
        this->reserve(data.stackStored + 1);
        this->data[data.stackStored++] = item;
    }

    /// Returns reference to the top element in the stack (size()-1)
    inline Type& peek() {
        return this->data[data.stackStored - 1];
    }

    /// Returns constant reference to the top element in the stack (size()-1)
    inline const Type& peek() const {
        return this->data[data.stackStored - 1];
    }

    /// Removes the top entry from the stack
    inline void pop() {
        //FASSERT(!this->isEmpty());
        --data.stackStored;
    }

    /// Reserves at least given minimal capacity of the stack (but possibly more). The number of items stored is not affected
    inline void reserve(const int amount) {
        //FASSERT(amount >= 0);
        if (this->data.size() < amount) {
            this->data.resize(std::max(amount, 2 * data.size()), true);
        }
    }

    /// Reallocates the stack so that there is no unused space (number of elements stored is the same as internal array
    /// capacity). Note that during execution of this method memory usage can actually spike because of the reallocation
    inline void trim() {
        this->data.resize(stored);
    }

    /// Pushes all elements from second stack onto this stack. Content of current stack remains unchanged at the bottom,
    /// content of second stack is copied after that so that so second stack top becomes new top
    inline void pushAll(const Stack& other) {
        for (int i = 0; i < other.size(); ++i) {
            this->push(other[i]);
        }
    }

    inline void pushAll(const Array<Type>& other) {
        for (int i = 0; i < other.size(); ++i) {
            this->push(other[i]);
        }
    }

    /// Returns constant iterator pointing at the first element of the array
    inline Iterator<const Type> cbegin() const {
        return Iterator<const Type>(this, 0);
    }

    /// Returns constant iterator pointing past the last element of the array
    inline Iterator<const Type> cend() const {
        return Iterator<const Type>(this, size());
    }

    /// Returns iterator pointing at the first element of the array
    inline Iterator<Type> begin() {
        return Iterator<Type>(this, 0);
    }

    /// Returns iterator pointing past the last element of the array
    inline Iterator<Type> end() {
        return Iterator<Type>(this, size());
    }

    /// Returns iterator pointing at the first element of the array
    inline Iterator<const Type> begin() const {
        return Iterator<const Type>(this, 0);
    }

    /// Returns iterator pointing past the last element of the array
    inline Iterator<const Type> end() const {
        return Iterator<const Type>(this, size());
    }

    /// Saves elements stored in this stack into an array. Overwrites previous content of the array
    void saveToArray(Array<Type>& array) const {
        array.resize(this->data.stackStored);
        for (int i = 0; i < this->data.stackStored; ++i) {
            array[i] = this->data[i];
        }
    }

    /// Sets number of elements stored. When newSize is bigger than current number of stored elements, value of new elements
    /// is undefined
    inline void resize(const int newSize) {
       // //FASSERT(newSize >= 0);
        this->reserve(newSize);
        this->data.stackStored = newSize;
    }

    /// Swaps the content of this and other stack, without actually copying the data
    void swap(Stack& other) {
        static_assert(sizeof(*this) == (sizeof(Array<Type>)), "If new data member is added, this method needs updating");
        this->data.swap(other.data);
        Utils::swap(this->data.stackStored, other.data.stackStored);
    }

    /// Removes an element from the middle of the stack in O(n) time
    inline void remove(const int index) {
        //FASSERT(unsigned(index) < unsigned(this->size()));
        for (int i = index + 1; i < data.stackStored; ++i) {
            this->data[i - 1] = this->data[i];
        }
        --this->data.stackStored;
    }

};

LUXCORE_NAMESPACE_END;
