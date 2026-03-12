# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

No changes yet.

## [1.0.0] - 2020-12-07

### Added
- K-d tree container adaptor with idiomatic C++ STL interface
- In-place tree construction via std::nth_element
- Exact search, nearest neighbor queries, range queries, and radius queries
- Templated point type as compositional facade over std::array
- Support for custom coordinate types and dynamically-sized containers
- Memory-efficient design with O(log n) stack overhead
