#pragma once
#include <stdexcept>

#define EXCEPT(name) \
	struct name final : public std::runtime_error \
	{ \
		name(const char* msg) : std::runtime_error(msg) {} \
	};

template <typename T, auto Closer>
class UHandle
{
private:
	T handle;
public:
	UHandle() noexcept : handle(nullptr) {}
	explicit UHandle(T handle) noexcept : handle(handle) {}
	UHandle& operator=(T handle)
	{
		if (this->handle) Closer(this->handle);

		this->handle = handle;
		return *this;
	}
	UHandle(UHandle&& o) noexcept
	{
		handle = o.handle;
		o.handle = nullptr;
	}
	UHandle& operator=(UHandle&& o)
	{
		if (&o != this)
		{
			if (handle) Closer(handle);

			handle = o.handle;
			o.handle = nullptr;
		}
		return *this;
	}
	~UHandle()
	{
		if (handle) Closer(handle);
	}

	UHandle(const UHandle&) = delete;
	UHandle& operator=(const UHandle&) = delete;

	constexpr operator T() noexcept { return handle; }

	T* operator&()
	{
		if (handle) Closer(handle);
		handle = nullptr;
		return &handle;
	}
	T* operator->() { return &handle; }
};