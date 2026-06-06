# Host

An Arduino library for representing a network host identified by either
an IPv4 address or a fully qualified domain name (FQDN).

## Features

- Stores either an IPv4 address or an FQDN — but not both simultaneously
- RFC 1035 / RFC 3696 compliant FQDN validation
- IPv4 address validation with octet range checking
- Built-in DNS resolution on ESP32 and ESP8266
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
For example, on an ATmega328-based Uno, a 254-byte FQDN can already take up too much memory.
The situation is better on the ATmega2560-based Mega.

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/soosp/Host.git
```

### Arduino IDE

Download the repository as a ZIP and install via **Sketch → Include Library → Add .ZIP Library**.

## Usage

### Basic usage

```cpp
#include <Host.h>

// Empty host
Host h;

// From IP address
Host h1(IPAddress(192, 168, 1, 1));

// From FQDN
Host h2("example.com");

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

```cpp
bool myResolver(const char* fqdn, IPAddress& ip) {
    // your DNS implementation
    return true;
}

Host h("example.com", myResolver);
```

## API

### Constants

|Constant|Value|Description|
|----------|-------|-------------|
|`MUTEX_TIMEOUT`|1000|Default mutex acquisition timeout in milliseconds.|
|`MAX_FQDN_LABEL_LEN`|63|Maximum length of a single FQDN label|
|`MAX_FQDN_LEN`|253|Maximum total FQDN length|

### Configuration

The default mutex timeout overridable via `HOST_MUTEX_TIMEOUT` preprocessor macro.
It should be defined before including this library:

```cpp
#define HOST_MUTEX_TIMEOUT 500
#include <Host.h>
```

### Constructors

```cpp
Host(ResolverFn resolver = _defaultResolver)
explicit Host(const IPAddress ip, ResolverFn resolver = _defaultResolver)
explicit Host(const char* fqdn, ResolverFn resolver = _defaultResolver)
```

Copy construction and copy assignment are disabled.

### Methods

|Method|Description|
|--------|-------------|
|`bool isEmpty()`|Returns true if neither IP address nor FQDN is stored|
|`IPAddress getIP()`|Returns the IP; performs DNS lookup if only FQDN is stored|
|`bool setIP(IPAddress ip)`|Sets the IP, clears FQDN|
|`bool getFqdn(char* buf, size_t len)`|Copies the stored FQDN into buf|
|`bool setFqdn(const char* fqdn)`|Sets the FQDN if RFC-conformant, clears IP|
|`bool toStr(char* buf, size_t len)`|Serializes to dotted-decimal IP or FQDN string|
|`bool fromStr(const char* str)`|Parses an IP or FQDN string|
|`static bool isValidFqdn(const char* str)`|RFC 1035/3696 FQDN validation|
|`static bool isValidIp(const char* str)`|IPv4 dotted-decimal validation|

All public methods are thread-safe on non-AVR platforms and return `false` on mutex acquisition timeout.

## License

MIT — see [LICENSE](LICENSE) for details.
