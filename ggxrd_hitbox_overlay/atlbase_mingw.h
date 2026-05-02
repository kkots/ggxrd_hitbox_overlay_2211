#pragma once
#include <assert.h>
// MinGW-w64 doesn't have atlbase.h and I literally only use one thing from it. ReactOS seems to have its own counterfeit version https://github.com/reactos/reactos/blob/master/sdk/lib/atl/atlbase.h
// ended up making shit up
template<typename T>
class CComPtr {
public:
	T* p;
	CComPtr() : p(nullptr) { }
	CComPtr(T* p) : p(p) { if (p) p->AddRef(); }
	~CComPtr() { if (p) p->Release(); }
	void Attach(T* newP) {
		if (p) p->Release();
		p = newP;
	}
	T* Detach() {
		T* oldP = p;
		p = nullptr;
		return oldP;
	}
	T** operator&() { assert(p == nullptr); return &p; }
	operator T*() { return p; }
	T& operator*() { assert(p != nullptr); return *p; }
	T* operator->() { assert(p != nullptr); return p; }
	operator bool() { return p != nullptr; }
	bool operator!() { return p == nullptr; }
	bool operator==(T* other) { return p == other; }
	bool operator!=(T* other) { return p != other; }
	CComPtr& operator=(T* other) {
		if (p == other) return *this;  // will cause a Release + AddRef situation (in that exact order)
		if (p) p->Release();
		p = other;
		if (p) p->AddRef();
		return *this;
	}
	CComPtr(const CComPtr& other) {
		p = other.p;
		if (p) p->AddRef();
	}
	CComPtr& operator=(const CComPtr& other) {
		if (p == other.p) return *this;  // will cause a Release + AddRef situation (in that exact order)
		if (p) p->Release();
		p = other.p;
		if (p) p->AddRef();
		return *this;
	}
	CComPtr(CComPtr&& other) {
		p = other.p;
		other.p = nullptr;
	}
	CComPtr& operator=(CComPtr&& other) {
		if (p) p->Release();
		p = other.p;
		other.p = nullptr;
		return *this;
	}
};
