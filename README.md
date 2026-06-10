# Host

An Arduino library for representing a network host identified by either
an IPv4 address or a fully qualified domain name (FQDN).

## Features

- Stores either an IPv4 address or an FQDN — but not both simultaneously
- RFC 1035 / RFC 3696 compliant FQDN validation
- IPv4 address validation with octet range checking
- Built-in DNS resolution on ESP32, ESP8266 and AVR platforms
- Custom resolver callback support for other platforms
- Thread-safe via `std::timed_mutex` with configurable timeout (non-AVR platforms)
- Lightweight: no dynamic memory allocation for the host data itself

## Supported Platforms

|Platform|DNS resolution|
|----------|---------------|
|ESP32 (Arduino Core 3.x)|`Network.hostByName`|
|ESP8266|`WiFi.hostByName`|
|AVR (Ethernet shield)*|`DNSClient::getHostByName`|
|Other|Custom resolver required|

*On the Arduino AVR platform, functionality is limited due to scarce resources.
For example, on an ATmega328-based Uno, a 254-byte FQDN can already take up too
much memory. The situation is better on the ATmega2560-based Mega.  
There are preprocessor macros available to help resolve this issue:
`HOST_FQDN_LABEL_LEN` and `HOST_FQDN_LEN`. See [Configuration](#configuration) for details.

## Dependencies

|Platform|Dependencies|
|--|--|
|ESP32|[ESP32 Arduino Core 3.x](https://github.com/espressif/arduino-esp32)|
|ESP8266|[ESP8266 Arduino Core 3.x](https://github.com/esp8266/arduino)|
|AVR|[Ethernet library](https://github.com/arduino-libraries/Ethernet)|

## Installation

### Arduino IDE

**Install via Library Manager:**

Search for **Host** in the Library Manager (Sketch → Include Library → Manage Libraries).

**Install manually:**

1. Download this repository as a ZIP file
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**

### PlatformIO

```ini
lib_deps =
    soosp/Host
```

### Manual

Copy the `Host.h` file into your project or library folder.

## Usage

### Basic usage

```cpp
#include <Host.h>

// Empty host
Host h;

// From IP address
Host h1(IPAddress(192, 168, 1, 1));

// From string — parsed as IP or FQDN automatically
Host h2("example.com");
Host h3("192.168.1.1");

// Set/get
h.setIP(IPAddress(10, 0, 0, 1));
h.setFqdn("mqtt.example.com");

// Resolve to IP (performs DNS lookup if FQDN is stored)
IPAddress ip = h2.getIP();

// Serialize
char buf[64];
h.toStr(buf, sizeof(buf));   // "192.168.1.1" or "example.com"

// Parse
h.fromStr("192.168.1.1");    // stored as IP
h.fromStr("example.com");    // stored as FQDN
```

### Validation

```cpp
if (Host::isValidFqdn("example.com")) { ... }
if (Host::isValidIp("192.168.1.1"))   { ... }
```

### Custom resolver

A custom resolver can be passed to any constructor as the last parameter.
The function must match the `ResolverFn` signature:

```cpp
bool myResolver(const char* fqdn, IPAddress& ip) {
    // your DNS implementation
    return true;
}

Host h("example.com", myResolver);
```

On ESP32, an mDNS resolver can be used for `.local` hostnames:

```cpp
#include <ESPmDNS.h>

bool mdnsResolver(const char* fqdn, IPAddress& ip) {
    ip = MDNS.queryHost(fqdn);
    return ip != IPAddress(0,0,0,0);
}

Host h("mydevice.local", mdnsResolver);
```

Or combined with regular DNS as a fallback:

```cpp
#include <ESPmDNS.h>

bool mdnsWithFallback(const char* fqdn, IPAddress& ip) {
    // Try mDNS first (fast, local network)
    ip = MDNS.queryHost(fqdn);
    if (ip != IPAddress(0,0,0,0)) return true;
    // Fall back to regular DNS
    return Network.hostByName(fqdn, ip) == 1;
}

Host h("mydevice.local", mdnsWithFallback);
```

## API

### Constants

|Constant|Value|Description|
|----------|-------|-------------|
|`MUTEX_TIMEOUT`|1000|Default mutex acquisition timeout in milliseconds.|
|`MAX_FQDN_LABEL_LEN`|63|Maximum length of a single FQDN label|
|`MAX_FQDN_LABEL_SIZE`|64|Buffer size for a single FQDN label including null terminator|
|`MAX_FQDN_LEN`|253|Maximum total FQDN length|
|`MAX_FQDN_SIZE`|254|Buffer size for FQDN including null terminator|

### Configuration

The default mutex timeout is overridable via the `HOST_MUTEX_TIMEOUT` preprocessor macro.

Although the RFC specifications define an FQDN as 253 characters and labels
as 63 characters, these values can consume excessive memory on
resource-constrained systems (e.g., Atmel AVR). Therefore, these lengths are
configurable via preprocessor macros `HOST_FQDN_LABEL_LEN` and `HOST_FQDN_LEN`
to allow smaller footprints.

These macros must be defined before including this library.

#### In PlatformIO (`platformio.ini`)

```ini
build_flags =
    -D HOST_MUTEX_TIMEOUT 500
    -D HOST_FQDN_LEN 63
```

#### In Arduino IDE

Define it before including `Host.h`:

```cpp
#define HOST_MUTEX_TIMEOUT 500
#define HOST_FQDN_LEN 63
#include <Host.h>
```

### Types

```cpp
using ResolverFn = bool(*)(const char* fqdn, IPAddress& ip);
```

Function pointer type for custom DNS resolver callbacks. The function receives a
null-terminated FQDN string and must write the resolved IP into `ip`.
Returns `true` on success, `false` on failure.

### Constructors

```cpp
Host(ResolverFn resolver = _defaultResolver)
```

Constructs an empty Host (IP `0.0.0.0`, no FQDN).

```cpp
explicit Host(const IPAddress ip, ResolverFn resolver = _defaultResolver)
```

Constructs a Host from an IPv4 address.

```cpp
explicit Host(const char* str, ResolverFn resolver = _defaultResolver)
```

Constructs a Host from a string. The string is first tried as a dotted-decimal
IPv4 address, then as an FQDN. If neither is valid, the Host is initialized empty.

All constructors accept an optional `resolver` parameter. On ESP32 the default resolver
uses `Network.hostByName`, on ESP8266 `WiFi.hostByName`, on AVR `DNSClient::getHostByName`.
On other platforms a stub resolver is used that always returns `false`; a custom resolver
must be provided for DNS resolution to work.

Copy construction and copy assignment are disabled.

### Methods

---

#### `bool isEmpty()`

Returns `true` if the Host holds neither an IP address nor an FQDN (i.e. it was
default-constructed, or parsing failed). Also returns `true` if the internal mutex
could not be acquired within `MUTEX_TIMEOUT` milliseconds.

---

#### `IPAddress getIP()`

Returns the IP address of the host. If an IP address is stored directly, it is
returned immediately. If only an FQDN is stored, a DNS lookup is performed via
the resolver function. The DNS lookup is intentionally done outside the mutex to
avoid blocking other threads during network I/O.

Returns `0.0.0.0` if the mutex could not be acquired or DNS resolution failed.

---

#### `bool setIP(IPAddress ip)`

Sets the host address to the given IPv4 address and clears any stored FQDN.
Returns `true` on success, `false` if the mutex could not be acquired.

---

#### `bool getFqdn(char* buf, size_t len)`

Copies the stored FQDN into `buf`. At most `len` bytes are written, including
the null terminator. Returns `true` if the FQDN fit entirely into the buffer,
`false` on truncation or mutex failure.

---

#### `bool setFqdn(const char* fqdn)`

Sets the host address to the given FQDN and clears the stored IP.
The FQDN is validated against RFC 1035 / RFC 3696 rules before storing.
Returns `true` on success, `false` if the FQDN is not RFC-conformant or
the mutex could not be acquired.

---

#### `bool toStr(char* buf, size_t len)`

Serializes the host to a human-readable string. Writes a dotted-decimal IP
address (e.g. `"192.168.1.1"`) if no FQDN is set, otherwise writes the FQDN.
At most `len` bytes are written, including the null terminator.
Returns `true` if the result fit entirely, `false` on truncation or mutex failure.

---

#### `bool fromStr(const char* str)`

Parses a string into a host address. The string is first tried as a dotted-decimal
IPv4 address with all octets in range [0, 255]; if that fails, it is tried as an
RFC-conformant FQDN. The previously stored address is replaced only on success.
Returns `true` on success, `false` if the string is neither a valid IP address
nor a valid FQDN, or if the mutex could not be acquired.

---

#### `static bool isValidFqdn(const char* str)`

Validates an FQDN string against RFC 1035 / RFC 3696 rules:

- Total length must not exceed 253 characters (excluding any trailing dot)
- Each dot-separated label must be 1–63 characters long
- Labels may only contain ASCII letters, digits, and hyphens
- Labels must not start or end with a hyphen
- The last label (TLD) must not be all-numeric

Returns `true` if the string is a valid FQDN, `false` otherwise.

---

#### `static bool isValidIp(const char* str)`

Validates a string as a dotted-decimal IPv4 address. All four octets must be
present and in the range [0, 255]. Extra characters after the fourth octet
(e.g. `"1.2.3.4.5"`) cause the validation to fail.

Returns `true` if the string is a valid IPv4 address, `false` otherwise.

---

All public methods except `isValidFqdn` and `isValidIp` are thread-safe on
non-AVR platforms. The bool-returning methods return `false` on mutex acquisition
timeout.

## License

MIT — see [LICENSE](LICENSE) for details.
