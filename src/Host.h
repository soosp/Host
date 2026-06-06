#pragma once
#include <Arduino.h>
#ifdef ARDUINO_ARCH_ESP32
#include <Network.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_AVR)
#include <Ethernet.h>
#include <Dns.h>
#endif
#ifndef ARDUINO_ARCH_AVR
#include <mutex>
#include <chrono>
#endif

#ifndef HOST_MUTEX_TIMEOUT
#define HOST_MUTEX_TIMEOUT 1000
#endif

/**
 * @brief Represents a network host identified by either an IP address or an FQDN.
 *
 * Stores either an IPv4 address or a fully qualified domain name (FQDN), but not both
 * simultaneously. All public methods are thread-safe via a std::timed_mutex.
 *
 * @note Copy construction and copy assignment are disabled.
 * @note If an FQDN is set, getIP() will perform a DNS lookup on every call.
 */
class Host {
public:
    /** @brief Default timeout in milliseconds when acquiring the internal mutex. */
    static constexpr uint32_t MUTEX_TIMEOUT = HOST_MUTEX_TIMEOUT;

    /** @brief Maximum FQDN label length. */
    static constexpr size_t MAX_FQDN_LABEL_LEN = 63;

    /** @brief Maximum FQDN length. */
    static constexpr size_t MAX_FQDN_LEN = 253;

    /**
     * @brief Function signature for DNS resolution callbacks.
     *
     * @param fqdn Null-terminated FQDN string to resolve.
     * @param ip   Output parameter receiving the resolved IP address.
     * @return true if resolution succeeded, false otherwise.
     */
    using ResolverFn = bool(*)(const char*, IPAddress&);

    /**
     * @brief Constructs an empty Host (IP 0.0.0.0, no FQDN).
     * @param resolver DNS resolver function. Defaults to Network.hostByName on ESP32,
     *                 WiFi.hostByName on ESP8266, returns false on other platforms.
     */
    Host(ResolverFn resolver = _defaultResolver)
        : _ip(IPAddress(0,0,0,0))
        , _resolver(resolver)
    {
        _fqdn[0] = 0;
    }

    /**
     * @brief Constructs a Host from an IP address.
     * @param ip       The IPv4 address to assign.
     * @param resolver DNS resolver function. Defaults to Network.hostByName on ESP32,
     *                 WiFi.hostByName on ESP8266, returns false on other platforms.
     */
    explicit Host(const IPAddress ip, ResolverFn resolver = _defaultResolver)
        : _ip(ip)
        , _resolver(resolver)
    {
        _fqdn[0] = 0;
    }

    /**
     * @brief Constructs a Host from an FQDN string.
     * @param fqdn     Null-terminated FQDN string. Truncated to MAX_FQDN_LEN characters if longer.
     * @param resolver DNS resolver function. Defaults to Network.hostByName on ESP32,
     *                 WiFi.hostByName on ESP8266, returns false on other platforms.
     */
    explicit Host(const char* fqdn, ResolverFn resolver = _defaultResolver)
        : _ip(IPAddress(0,0,0,0))
        , _resolver(resolver)
    {
        if (isValidFqdn(fqdn)) {
            snprintf(_fqdn, sizeof(_fqdn), "%s", fqdn);
        } else {
            _fqdn[0] = 0;
        }
    }

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    /**
     * @brief Destroys the Host.
     */
    ~Host() = default;

    /**
     * @brief Checks whether the Host holds no address (IP is 0.0.0.0 and FQDN is empty).
     * @return true if empty or mutex acquisition failed, false otherwise.
     */
    bool isEmpty() {
        if (!_lock()) return true;
        bool empty = ((_fqdn[0] == 0) && (_ip == IPAddress(0,0,0,0)));
        _unlock();
        return empty;
    }

    /**
     * @brief Returns the IP address of the host.
     *
     * If only an FQDN is stored, performs a DNS lookup via _resolver().
     * The DNS lookup is done outside the mutex to avoid blocking other threads.
     *
     * @return Resolved or stored IP address, or 0.0.0.0 on failure.
     */
    IPAddress getIP() {
        if (!_lock()) return IPAddress(0,0,0,0);
        IPAddress ip = _ip;
        char fqdn[MAX_FQDN_LEN + 1];
        snprintf(fqdn, sizeof(fqdn), "%s", _fqdn);
        _unlock();
        if (fqdn[0] != 0) {
            if (!_resolver(fqdn, ip)) ip = IPAddress(0,0,0,0);
        }
        return ip;
    }

    /**
     * @brief Sets the host address to the given IP and clears any stored FQDN.
     * @param ip The IPv4 address to assign.
     * @return true on success, false if mutex acquisition failed.
     */
    bool setIP(IPAddress ip) {
        if (!_lock()) return false;
        _fqdn[0] = 0;
        _ip = ip;
        _unlock();
        return true;
    }

    /**
     * @brief Copies the stored FQDN into the provided buffer.
     * @param fqdn Destination buffer.
     * @param len  Size of the destination buffer in bytes.
     * @return true if the FQDN fit entirely, false on truncation or mutex failure.
     */
    bool getFqdn(char* fqdn, size_t len) {
        if (!_lock()) return false;
        int written = snprintf(fqdn, len, "%s", _fqdn);
        bool ok = (written >= 0 && (size_t)written < len);
        _unlock();
        return ok;
    }

    /**
     * @brief Sets the host address to the given FQDN and clears the stored IP.
     * @param fqdn Null-terminated FQDN string. Truncated to MAX_FQDN_LEN if longer.
     * @return true if the FQDN fit entirely, false on truncation, mutex failure or
     *         if FQDN is not RFC conform.
     */
    bool setFqdn(const char* fqdn) {
        if (!isValidFqdn(fqdn)) return false;
        if (!_lock()) return false;
        int written = snprintf(_fqdn, sizeof(_fqdn), "%s", fqdn);
        bool ok = (written >= 0 && (size_t)written < sizeof(_fqdn));
        _ip = IPAddress(0,0,0,0);
        _unlock();
        return ok;
    }

    /**
     * @brief Serializes the host to a human-readable string.
     *
     * Writes a dotted-decimal IP address (e.g. "192.168.1.1") if no FQDN is set,
     * otherwise writes the FQDN.
     *
     * @param str Destination buffer.
     * @param len Size of the destination buffer in bytes.
     * @return true if the result fit entirely, false on truncation or mutex failure.
     */
    bool toStr(char* str, size_t len) {
        if (!_lock()) return false;
        int written;
        if (_fqdn[0] == 0) {
            written = snprintf(str, len, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3]);
        } else {
            written = snprintf(str, len, "%s", _fqdn);
        }
        _unlock();
        return (written >= 0 && (size_t)written < len);
    }

    /**
     * @brief Parses a string into a host identifier.
     *
     * If the string matches a dotted-decimal format (e.g. "192.168.1.1") valid
     * IP address, it is stored as an IP address. If the sting a valid FQDN it is
     * stored and the IP is cleared. Otherwise returns false,
     *
     * @param str Null-terminated input string.
     * @return true on success, false on neither valid IP addres nor valid FQDN found,
     *         the FQDN was truncated (means also not valid), or mutex acquisition
     *         failed.
     */
    bool fromStr(const char *str) {
        unsigned int ip[4] = {0, 0, 0, 0};
        bool isIp = isValidIp(str);

        if (isIp) sscanf(str, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
        else if (!isValidFqdn(str)) return false;

        if (!_lock()) return false;
        bool ok = true;
        if (isIp) {
            _ip = IPAddress(ip[0], ip[1], ip[2], ip[3]);
            _fqdn[0] = 0;
        } else {
            int written = snprintf(_fqdn, sizeof(_fqdn), "%s", str);
            ok = (written >= 0 && (size_t)written < sizeof(_fqdn));
            _ip = IPAddress(0,0,0,0);
        }
        _unlock();
        return ok;
    }

    /**
     * @brief Validates an FQDN string against RFC 1035 / RFC 3696 rules.
     *
     * Rules enforced:
     * - Total length must not exceed 253 characters (excluding any trailing dot).
     * - Each dot-separated label must be 1–63 characters long.
     * - Labels may only contain ASCII letters, digits, and hyphens.
     * - Labels must not start or end with a hyphen.
     * - The last label (TLD) must not be all-numeric.
     *
     * @param str Null-terminated string to validate.
     * @return true if valid, false otherwise.
     */
    static bool isValidFqdn(const char* str) {
        if (!str || str[0] == 0) return false;

        size_t totalLen = strlen(str);

        // Strip optional trailing dot ("example.com." is legal)
        if (str[totalLen - 1] == '.') totalLen--;
        if (totalLen == 0 || totalLen > MAX_FQDN_LEN) return false;

        const char* p = str;
        const char* lastLabel = p;
        int labelLen = 0;

        while ((size_t)(p - str) < totalLen) {
            if (*p == '.') {
                if (labelLen == 0)               return false; // empty label
                if (*(p - 1) == '-')             return false; // label ends with hyphen
                lastLabel = p + 1;
                labelLen = 0;
            } else {
                if (labelLen == 0 && *p == '-')  return false; // label starts with hyphen
                if (!isalnum((unsigned char)*p) && *p != '-') return false; // invalid char
                if (++labelLen > (int)MAX_FQDN_LABEL_LEN) return false; // label too long
            }
            p++;
        }

        // Check the last label
        if (labelLen == 0)
            return false; // trailing dot already stripped, so this means empty
        if (lastLabel[labelLen - 1] == '-')
            return false; // last label ends with hyphen

        // TLD must not be all-numeric
        bool allNumeric = true;
        for (const char* t = lastLabel; *t && *t != '.'; t++) {
            if (!isdigit((unsigned char)*t)) { allNumeric = false; break; }
        }
        if (allNumeric) return false;

        return true;
    }

    /**
     * @brief Validates a string as IP address.
     *
     * Checks if the string matches the dotted-decimal format (e.g. "192.168.1.1")
     * with all octets in range [0, 255].
     *
     * @param str Null-terminated string to validate.
     * @return true if valid, false otherwise.
     */
    static bool isValidIp(const char* str) {
        unsigned int ip[4] = {0, 0, 0, 0};
        char tail[2] = {0};
        if (sscanf(str, "%u.%u.%u.%u%1s", &ip[0], &ip[1], &ip[2], &ip[3], tail) != 4) return false;
        if (ip[0] > 255 || ip[1] > 255 || ip[2] > 255 || ip[3] > 255) return false;
        return true;
    }

private:
#ifndef ARDUINO_ARCH_AVR
    mutable std::timed_mutex _mutex; ///< mutex protecting _ip and _fqdn.
#endif
    IPAddress _ip;                   ///< Stored IPv4 address. Valid when _fqdn[0] == 0.
    char _fqdn[MAX_FQDN_LEN + 1];    ///< Stored FQDN. Empty string when IP is used instead.
    ResolverFn _resolver;            ///< DNS resolver function used by getIP().

#ifdef ARDUINO_ARCH_AVR
    /** @brief AVR is single-threaded; lock always succeeds. Timeout parameter is ignored. */
    bool _lock(uint32_t /*timeoutMs*/ = MUTEX_TIMEOUT) const { return true; }
    /** @brief AVR is single-threaded; no-op. */
    void _unlock() const {}
#else
    /** @brief Tries to acquire the mutex. @return true on success, false on timeout. */
    bool _lock(uint32_t timeoutMs = MUTEX_TIMEOUT) const {
        return _mutex.try_lock_for(std::chrono::milliseconds(timeoutMs));
    }

    /** @brief Releases the mutex. */
    void _unlock() const {
        _mutex.unlock();
    }
#endif

#ifdef ARDUINO_ARCH_ESP32
    /**
     * @brief Default DNS resolver using the ESP32 Arduino Core Network.hostByName().
     * @param fqdn Null-terminated FQDN string to resolve.
     * @param ip   Output parameter receiving the resolved IP address.
     * @return true if resolution succeeded, false otherwise.
     */
    static bool _defaultResolver(const char* fqdn, IPAddress& ip) {
        return Network.hostByName(fqdn, ip) == 1;
    }
#elif defined(ARDUINO_ARCH_ESP8266)
    /**
     * @brief Default DNS resolver using the ESP8266 Arduino Core WiFi.hostByName().
     * @param fqdn Null-terminated FQDN string to resolve.
     * @param ip   Output parameter receiving the resolved IP address.
     * @return true if resolution succeeded, false otherwise.
     */
    static bool _defaultResolver(const char* fqdn, IPAddress& ip) {
        return WiFi.hostByName(fqdn, ip) == 1;
    }
#elif defined(ARDUINO_ARCH_AVR)
    /**
     * @brief Default DNS resolver using the DNSClient::getHostByName()
              from Arduino Ethernet library.
     * @param fqdn Null-terminated FQDN string to resolve.
     * @param ip   Output parameter receiving the resolved IP address.
     * @return true if resolution succeeded, false otherwise.
     */
    static bool _defaultResolver(const char* fqdn, IPAddress& ip) {
        DNSClient dns;
        dns.begin(Ethernet.dnsServerIP());
        return dns.getHostByName(fqdn, ip) == 1;
    }
#else
    /**
     * @brief Stub resolver for non-ESP32 platforms. Always returns false.
     *        Override by passing a custom ResolverFn to the constructor.
     * @param fqdn Null-terminated FQDN string (unused).
     * @param ip   Output parameter (unused).
     * @return false always.
     */
    static bool _defaultResolver(const char* /*fqdn*/, IPAddress& /*ip*/) {
        return false;
    }
#endif    
};