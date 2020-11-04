#pragma once
#include <stdexcept>
#include <sstream>

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

template <typename L>
class Defer
{
private:
	L l;
public:
	Defer(L&& l) : l(std::move(l)) {}
	~Defer()
	{
		l();
	}

	Defer(const Defer&) = delete;
	Defer& operator=(const Defer&) = delete;
};

template <typename T>
std::string toString(T t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

template <typename T>
std::wstring toWString(T t)
{
	std::wstringstream ss;
	ss << t;
	return ss.str();
}