//#include "main.h"

#pragma once

template <class T> class vector {
private:
	int csize;
	int asize;
	T* data;
public:
   vector();
   void push_back(T obj);
   void clear();
   unsigned int size() const;
   T& operator[](int i) const;
   void remove_at(int i);
};

template <class T>
vector<T>::vector() {
	csize=0;
	asize=8;
	data=new T[8];
}

template <class T>
void vector<T>::push_back(T obj) {
	if(csize==asize) {
		asize*=2;
		T* newData=new T[asize];
		for(int i=0;i<csize;i++) newData[i]=data[i];
		delete[] data;
		data=newData;
	}
	data[csize++] = obj;
}

template <class T>
unsigned int vector<T>::size() const { return csize; }

template <class T>
void vector<T>::clear() { csize=0; }

template <class T>
T& vector<T>::operator[](int i) const {
	return data[i];
}

template <class T>
void vector<T>::remove_at(int i) {
	if(i>=csize) return;
	for(int j=i;j<csize-1;j++) data[j]=data[j+1];
	csize--;
}