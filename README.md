C++ DBC Parser and CAN Transcoder
=================================
[![License](https://img.shields.io/badge/license-BSD-blue.svg)](

Introduction
------------

This repository is motivated by a lack of suitable open-source DBC parsers in C++.

They are often plagued by issues such as being slow, bloated, or supporting only a fraction of the full DBC standard.

We provide a fast, lightweight, and fully-featured DBC parser.

The project also contains `v2c_transcoder.cpp`, showing how such a parser can be used for [IOT telemetry](https://iotatlas.net/en/patterns/telemetry/).

Contains
--------
* [DBC parser](dbc/README.md)
    * A complete and customizable DBC parser written in C++. Full DBC syntax support for all keywords.
* [Vehicle-To-Cloud Transcoder](v2c/README.md)
    * Edge-computing telemetric component that groups, filters, and aggregates CAN signals. Can drastically reduce the amount of data sent from the device over the network.
Uses the DBC parser to read and define the CAN network.

How to Build
------------

#### 1. Fetch Boost

* Download [Boost](https://www.boost.org/users/download/) and move it to your include path

The project requires only headers from Boost, so no libraries need to be built.

#### 2. Build

You can compile the example as follows.

```sh
$ g++ -std=c++20 example\example.cpp dbc\dbc_parser.cpp v2c\v2c_transcoder.cpp -I . -o can_example
```

`can-utils` has been tested with Clang, GCC and MSVC on Windows and Linux. It requires C++20.

#### 3. Run Example
```sh
$ ./can_example
```

The example program parses [example.dbc](example/example.dbc), generates random frames, aggregates them with `v2c_transcoder`, and prints the decoded raw signals to the console.

Usage
-----

- [Example](example/README.md)
- [DBC Parser](dbc/README.md)
- [CAN Transcoder](v2c/README.md)

License
-------

Copyright (c) 2001-2023 Mireo, EU

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Contact us
---------- 

<p align="center">
<a href="https://www.mireo.com/spacetime"><img height="200" alt="Mireo" src="https://www.mireo.com/img/assets/mireo-logo.svg"></img></a>
</p>