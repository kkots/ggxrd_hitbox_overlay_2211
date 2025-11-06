#pragma once
#include <type_traits>
#if _DEBUG
#include <stdexcept>
#endif

// std::shared_ptr has size 8, instead of 4, because it keeps track of the ref counter separately from the object
// our shitty substitute does not allow you to have the object originally not meant to be shared and then turn it into shared through make_shared
// no. Instead, the object must be created shared from the very start
// this allows us to store the ref counter actually together and enforce that they're always together
// and keep size 4 of the shared pointer
// and we don't need thread safety but we could add it just for shits'n'giggles
// without sacrificing size

template<typename T>
class ThreadUnsafeSharedResource {
public:
	template<class... _Types>
	inline ThreadUnsafeSharedResource(_Types&&... _Args) {
		 new ((T*)resourceBytes) T (std::forward<_Types>(_Args)...);
	}
	inline ThreadUnsafeSharedResource(const ThreadUnsafeSharedResource& other) = delete;
	inline ThreadUnsafeSharedResource(ThreadUnsafeSharedResource&& other) = delete;
	inline ThreadUnsafeSharedResource& operator=(const ThreadUnsafeSharedResource& other) = delete;
	inline ThreadUnsafeSharedResource& operator=(ThreadUnsafeSharedResource&& other) = delete;
	inline unsigned int addRef() { return ++refCount; }
	inline unsigned int release() { return --refCount; }
	inline unsigned int use_count() const { return refCount; }
	inline T& get() { return *(T*)resourceBytes; }
private:
	unsigned int refCount = 0;
	BYTE resourceBytes[sizeof(T)];  // can't instantiate abstract class
};

template<typename T>
class ThreadUnsafeSharedPtr {
public:
	template<typename otherT>
	inline operator ThreadUnsafeSharedPtr<otherT>() const {
		ThreadUnsafeSharedPtr<otherT> result;
		*(otherT**)&result = (otherT*)resource;
		resource->addRef();
		return result;
	}
	inline ThreadUnsafeSharedPtr() { }
	inline ThreadUnsafeSharedPtr(ThreadUnsafeSharedResource<T>* resource) : resource(resource) {
		if (resource) {
			resource->addRef();
		}
	}
	inline ~ThreadUnsafeSharedPtr() {
		if (!resource) return;
		unsigned int refCount = resource->release();
		if (refCount == 0) {
			delete resource;
		}
	}
	inline ThreadUnsafeSharedPtr(const ThreadUnsafeSharedPtr& other) {
		resource = other.resource;
		if (resource) {
			resource->addRef();
		}
	}
	inline ThreadUnsafeSharedPtr(ThreadUnsafeSharedPtr&& other) noexcept {
		resource = other.resource;
		other.resource = nullptr;
	}
	inline ThreadUnsafeSharedPtr& operator=(const ThreadUnsafeSharedPtr& other) {
		if (resource == other.resource) {
			return *this;
		}
		if (resource) {
			unsigned int newCount = resource->release();
			if (!newCount) {
				delete resource;
			}
		}
		resource = other.resource;
		if (resource) {
			resource->addRef();
		}
		return *this;
	}
	inline ThreadUnsafeSharedPtr& operator=(ThreadUnsafeSharedPtr&& other) noexcept {
		if (resource == other.resource) {
			other.resource = nullptr;
			if (resource) {
				resource->release();
			}
			return *this;
		}
		if (resource) {
			unsigned int newCount = resource->release();
			if (!newCount) {
				delete resource;
			}
		}
		resource = other.resource;
		other.resource = nullptr;
		return *this;
	}
	inline ThreadUnsafeSharedPtr& operator=(ThreadUnsafeSharedResource<T>* other) {
		if (resource == other) {
			return *this;
		}
		if (resource) {
			unsigned int newCount = resource->release();
			if (!newCount) delete resource;
		}
		resource = other;
		if (resource) {
			resource->addRef();
		}
		return *this;
	}
	inline T& operator*() {
		if (!resource) return *(T*)nullptr;  // this I feel you could still survive somehow
		return resource->get();
	}
	inline T* operator->() {
		#if _DEBUG
		if (!resource) throw std::logic_error("ThreadUnsafeSharedPtr: -> on a null pointer.");
		#else
		if (!resource) return nullptr;
		#endif
		return &(resource->get());
	}
	inline const T& operator*() const {
		if (!resource) return *(T*)nullptr;
		return resource->get();
	}
	inline const T* operator->() const {
		#if _DEBUG
		if (!resource) throw std::logic_error("ThreadUnsafeSharedPtr: -> on a null pointer.");
		#else
		if (!resource) return nullptr;
		#endif
		return &resource->get();
	}
	inline operator bool () const {
		return resource != nullptr;
	}
	inline bool operator!() const {
		return resource == nullptr;
	}
	inline bool operator==(const ThreadUnsafeSharedPtr& other) const {
		return resource == other.resource;
	}
	inline bool operator!=(const ThreadUnsafeSharedPtr& other) const {
		return resource != other.resource;
	}
	inline bool operator==(const ThreadUnsafeSharedResource<T>* other) const {
		return resource == other;
	}
	inline bool operator!=(const ThreadUnsafeSharedResource<T>* other) const {
		return resource != other;
	}
	inline bool operator==(const T* other) const {
		if (!resource) return !other;
		return &(resource->get()) == other;
	}
	inline bool operator!=(const T* other) const {
		if (!resource) return other;
		return &(resource->get()) != other;
	}
	inline unsigned int use_count() const {
		if (!resource) return 0;
		return resource->use_count();
	}
	inline T* get() const {
		if (!resource) return nullptr;
		return &(resource->get());
	}
private:
	ThreadUnsafeSharedResource<T>* resource = nullptr;
};
