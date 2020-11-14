#pragma once
#include <utility>
#include <stdexcept>
#include <sstream>

// Diverse bra-att-ha-saker

/* Förlåt mig för att använda en så konstig makro men den här
   makron kan man använda för att snabbt definiera en exceptiontyp.
   Den har en konstruktor där man anger ett felmeddelande som visas. */
#define EXCEPT(name) \
	struct name final : public std::runtime_error \
	{ \
		name(const char* msg) : std::runtime_error(msg) {} \
	};

/* Template för en klass som hanterar ett handle, d.v.s. en typ som "representerar"
   någonting och som måste stängas/frias upp (t.ex. HWND för fönster och SOCKET för
   sockets). Klassens destruktor stänger automatiskt handlet om det är giltigt. Klassen
   heter UHandle (= unique handle) för att jag har gjort så att man inte kan kopiera
   den (för då när en av instanserna förstörs så stänger den handlet vilket betyder att
   den andra har ett ogiltigt handle). Man kan däremot "flytta" den (se "move semantics").
   Templaten behöver tre saker: en typ (d.v.s. handletypen), en funktion som ska användas
   för att stänga handlet och (om nödvändigt) vilket värde som används för att representera
   ett ogiltigt handle. Vanligtvis används 0 (= nullptr) för att representera ogiltiga
   handles men vissa saker (t.ex. SOCKET) har ett annat värde. (I SOCKET:s fall är det
   INVALID_SOCKET, vilket är ~0, vilket inte är noll.) */
template <typename T, auto Closer, T InvalidHandle = nullptr>
class UHandle
{
private:
	T handle;
public:
	/* Defaultkonstruktorn sätter handlet till det ogiltiga värdet istället för att bara
	   inte initiera det. (Annars skulle destruktorn försöka ta bort något random värde.) */
	UHandle() noexcept : handle(InvalidHandle) {}
	// Man kan konstruera UHandle med ett handle som man har fått från något.
	explicit UHandle(T handle) noexcept : handle(handle) {}
	// Man kan byta ut handlevärdet med ett nytt värde.
	UHandle& operator=(T handle)
	{
		// Fria upp ett eventuellt handle som redan finns.
		if (this->handle != InvalidHandle) Closer(this->handle);

		// Byt ut handlet.
		this->handle = handle;
		return *this;
	}
	// Movekonstruktor
	UHandle(UHandle&& o) noexcept
	{
		handle = o.handle;
		o.handle = InvalidHandle;
	}
	// Moveassignmentoperator
	UHandle& operator=(UHandle&& o)
	{
		if (&o != this)
		{
			// Fria upp ett eventuellt handle som redan finns.
			if (handle != InvalidHandle) Closer(handle);

			handle = o.handle;
			o.handle = InvalidHandle;
		}
		return *this;
	}
	~UHandle()
	{
		// Fria endast upp handlet om det är giltigt.
		if (handle != InvalidHandle) Closer(handle);
	}

	// De här två raderna tar bort kopiering.
	UHandle(const UHandle&) = delete;
	UHandle& operator=(const UHandle&) = delete;

	/* Sätter handle till det ogiltiga handlet utan att stänga handlet. (Använd
	   om du vet att handlet redan är stängt.) */
	void resetNoClose() noexcept
	{
		handle = InvalidHandle;
	}

	/* Det här gör så att man kan implicit konvertera klassen till själva handletypen.
	   (Alltså om en funktion vill ett HWND så kan man bara stoppa in en instans av
	   UHandle<HWND, ...> och så funkar det.) */
	constexpr operator T() noexcept { return handle; }

	/* Detta gör så att om man tar addressen av ett UHandle (t.ex. &uh där
	   uh är ett UHandle) så TAR DEN BORT *EVENTUELLA* HANDLES som finns i klassen
	   och returnerar addressen till själva handlet i klassen (och inte addressen
	   till UHandle:t). Detta är för att vissa funktioner vill ha en pekare till
	   en variabel som den kan lägga handlet i. */
	T* operator&()
	{
		if (handle != InvalidHandle) Closer(handle);
		handle = InvalidHandle;
		return &handle;
	}
	// Om handletypen är en pekare till något så kan man komma åt medlemmar med piloperatorn.
	T* operator->() { return &handle; }
	
	// Detta gör så att ett UHandle anses vara sant om och endast om det är giltigt.
	constexpr operator bool() const noexcept { return handle != InvalidHandle; }
};

/* Detta är en template för en klass som kör en lambdafunktion när den förstörs. 
   I nyare versioner av C++ autodetektas templatetypen L så man kan helt enkelt
   skriva: Defer df([](){ puts("Körs i slutet av scopet när df förstörs."); }); */
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

// Konvertera något till en std::string.
template <typename T>
std::string toString(T t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

// Konvertera något till en std::wstring.
template <typename T>
std::wstring toWString(T t)
{
	std::wstringstream ss;
	ss << t;
	return ss.str();
}

inline std::string wstringToString(const std::wstring& s)
{
	size_t len = WideCharToMultiByte(
		CP_UTF8,
		0,
		s.c_str(),
		s.length(),
		nullptr,
		0,
		nullptr,
		nullptr
	);
	std::string ret(len, '\0');
	WideCharToMultiByte(
		CP_UTF8,
		0,
		s.c_str(),
		s.length(),
		&ret[0],
		ret.length(),
		nullptr,
		nullptr
	);
	return ret;
}

inline std::wstring stringToWstring(const std::string& s)
{
	size_t len = MultiByteToWideChar(
		CP_UTF8,
		0,
		s.c_str(),
		s.length(),
		nullptr,
		0
	);
	std::wstring ret(len, L'\0');
	MultiByteToWideChar(
		CP_UTF8,
		0,
		s.c_str(),
		s.length(),
		&ret[0],
		ret.length()
	);
	return ret;
}