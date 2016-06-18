#pragma once

#define VBEAT_CONFIG_ORG "ssc"
#define VBEAT_CONFIG_APP "varibeat"
#ifdef SSC_FUTURES
#	define VBEAT_CONFIG_VERSION 20170510L
#endif

#ifdef VBEAT_WINDOWS
#	define VBEAT_EXPORT __declspec(dllexport)
#else
#	define VBEAT_EXPORT
#endif

// http://stackoverflow.com/a/33298731
#if !defined(VBEAT_NOEXCEPT)
#	if defined(__clang__)
#		if __has_feature(cxx_noexcept)
#			define HAS_NOEXCEPT
#		endif
#	else
#		 if defined(__GXX_EXPERIMENTAL_CXX0X__) && \
			__GNUC__ * 10 + __GNUC_MINOR__ >= 46 || \
			defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026
#			define HAS_NOEXCEPT
#		endif
#	endif
#	ifdef HAS_NOEXCEPT
#		undef HAS_NOEXCEPT
#		define VBEAT_NOEXCEPT noexcept
#	else
#		define VBEAT_NOEXCEPT
#	endif
#endif

#include <cstddef> // size_t, ...
#include <memory>  // shared_ptr, ...

void* operator new(size_t sz);
void operator delete(void* ptr) VBEAT_NOEXCEPT;

namespace bx {
	struct AllocatorI;
}

namespace vbeat {
	bx::AllocatorI *get_allocator();
	void *v_malloc(size_t bytes);
	void *v_realloc(void *ptr, size_t new_size);
	void  v_free(void *ptr);

	// STL allocator, for things that need it.
	// I'm not sure we need this if we're overriding global operators new & delete.
	template <class T>
	class v_allocator
	{
	public:
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;

		v_allocator() {}
		v_allocator(const v_allocator&) {}

		pointer allocate(size_type n, const void * = 0) {
			T* t = (T*)vbeat::v_malloc(n * sizeof(T));
			return t;
		}

		void deallocate(void* p, size_type) {
			if (p) {
				vbeat::v_free(p);
			}
		}

		pointer address(reference x) const {
			return &x;
		}

		const_pointer address(const_reference x) const {
			return &x;
		}

		v_allocator<T>& operator=(const v_allocator&) {
			return *this;
		}

		void construct(pointer p, const T& val) {
			new ((T*) p) T(val);
		}
		void destroy(pointer p) {
			p->~T();
		}

		size_type max_size() const {
			return size_t(-1);
		}

		template <class U>
		struct rebind {
			typedef v_allocator<U> other;
		};

		template <class U>
		v_allocator(const v_allocator<U>&) {}

		template <class U>
		v_allocator& operator=(const v_allocator<U>&) {
			return *this;
		}
	};
}
