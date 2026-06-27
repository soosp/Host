# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [0.6.1]

### Changed

- Fix type of `DEFAULT_DNS_CACHE_TTL_MS`

## [0.6.0]

### Added

- Add DNS resolver cache

### Changed

- Updated examples
- A lightweight IP validator and parser instead of sscanf
- Make getters const

## [0.5.0]

### Changed

- Use '\0' convention everywhere for empty zero terminated strings
- fromStr(): Empty input string results empty host object

## [0.4.0]

### Added

- \_LEN / \_SIZE constans for null terminated strings
- mDNS resolver example in `README.md`
- Add dependencies

### Changed

- Make the FQDN and FQDN label length configurable
- Update AVR example to demonstrate how to use less memory

## [0.3.0]

### Added

- Detailed API description

### Changed

- Use fromStr() in constructor to use not just FQDN but dotted decimal IP address string too.

## [0.2.1] - 2026-06-06

### Changed

- Minor fixes in `README.md`
- Remove redundant FQDN validity checks

## [0.2.0] - 2026-06-06

### Added

- Support AVR platform

### Changed

- Use pointer for `ResolverFn` instead of `std::function`

## [0.1.1] - 2026-06-06

### Changed

- Fix `library.json` for PaltformIO registry
- Fix Changelog

## [0.1.0] - 2026-06-05

### Added

- First public release

[unreleased]: https://github.com/soosp/Host/compare/0.6.1...HEAD
[0.6.1]: https://github.com/soosp/Host/compare/0.6.0...0.6.1
[0.6.0]: https://github.com/soosp/Host/compare/0.5.0...0.6.0
[0.5.0]: https://github.com/soosp/Host/compare/0.4.0...0.5.0
[0.4.0]: https://github.com/soosp/Host/compare/0.3.0...0.4.0
[0.3.0]: https://github.com/soosp/Host/compare/0.2.1...0.3.0
[0.2.1]: https://github.com/soosp/Host/compare/0.2.0...0.2.1
[0.2.0]: https://github.com/soosp/Host/compare/0.1.1...0.2.0
[0.1.1]: https://github.com/soosp/Host/compare/0.1.0...0.1.1
[0.1.0]: https://github.com/soosp/Host/releases/tag/0.1.0
