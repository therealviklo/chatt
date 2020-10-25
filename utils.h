#pragma once
#include <stdexcept>

#define EXCEPT(name) \
	struct name final : public std::runtime_error \
	{ \
		name(const char* msg) : std::runtime_error(msg) {} \
	};

template <typename T, auto Closer, T InvalidHandle = nullptr>
class UHandle
{
private:
	T handle;
public:
	UHandle() noexcept : handle(InvalidHandle) {}
	explicit UHandle(T handle) noexcept : handle(handle) {}
	UHandle& operator=(T handle)
	{
		if (this->handle != InvalidHandle) Closer(this->handle);

		this->handle = handle;
		return *this;
	}
	UHandle(UHandle&& o) noexcept
	{
		handle = o.handle;
		o.handle = InvalidHandle;
	}
	UHandle& operator=(UHandle&& o)
	{
		if (&o != this)
		{
			if (handle != InvalidHandle) Closer(handle);

			handle = o.handle;
			o.handle = InvalidHandle;
		}
		return *this;
	}
	~UHandle()
	{
		if (handle != InvalidHandle) Closer(handle);
	}

	UHandle(const UHandle&) = delete;
	UHandle& operator=(const UHandle&) = delete;

	constexpr operator T() noexcept { return handle; }

	T* operator&()
	{
		if (handle != InvalidHandle) Closer(handle);
		handle = InvalidHandle;
		return &handle;
	}
	T* operator->() { return &handle; }
	
	constexpr operator bool() const noexcept { return handle != InvalidHandle; }
};