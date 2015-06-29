//#include "main.h"

template <class T> class queue {
private:
	int csize;
	int asize;
	int start;
	T* data;
public:
   queue();
   void push(T obj);
   void pop();
   void clear();
   T front() const;
   bool empty() const;
};

template <class T>
queue<T>::queue() {
	csize=0;
	asize=1;
	start=0;
	data=new T[1];
}

template <class T>
void queue<T>::push(T obj) {
	if(csize==asize) {
		asize*=2;
		T* newData=new T[asize];
		int pos=start-1;
		for(int i=0;i<csize;i++) {
			if(++pos==csize) pos=0;
			newData[i]=data[pos];
		}
		delete[] data;
		data=newData;
		start=0;
	}
	data[(start+csize)%asize] = obj;
	csize++;
}

template <class T>
void queue<T>::pop() {
	if(csize==0) return;
	start++;
	csize--;
	if(start==asize) start=0;
}

template <class T>
T queue<T>::front() const { return data[start]; }

template <class T>
void queue<T>::clear() { csize=0; start=0; }

template <class T>
bool queue<T>::empty() const { return csize==0; }
