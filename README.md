# ImmutableString

Yet another immutable C++ string implementation.
================================================

* ims::basic_immutable_string object can hold a reference to a string literal, no memory allocations involved in this case.
```
ims::immutable_string str("I hold only a pointer and size", immutable_string::FromStringLiteral);
```

* reference-counted data
```
ims::immutable_string str1("I share a reference to actual data");
ims::immutable_string str2(str1); // share the same data
```

* *almost* zero-cost copying. Copying a basic_immutable_string instance costs as much as one atomic increment and two pointer-size member copyings.

* short string optimization (SSO). Strings up to 22 bytes long on x64 (including null terminator) are stored inside basic_immutable_string object, no additional allocations.

* allocation-free substr() method. Substring only hold a strong reference to the original string.

* STL-compatible

* header-only


Benchmarks
--------

Splitting 33,495,558 bytes long string into 1,000,000 words.

* std::string
```
Time:                  63 ms
Allocations:           765618
Additional memory:     35.523 Mb
```

* ims::immutable_string
```
Time:                  23 ms
Allocations:           0
Additional memory:     0 Mb
```


Cloning
--------
```bash
git clone --recurse-submodules https://github.com/LeeOswald/ImmutableString.git

```

Building & running tests
------------------------
This is a header-only library; only tests require building.
On Windows, open Visual Studio Command Propmpt (or just find and run vcvars64.bat from VS installation folder).

```bash
cd ImmutableString
mkdir build
cd build
cmake ..
cmake . --build
./string_tests
```

Benchmarking
-------------

First of all, generate a dataset:
```bash
./string_tests --generate my_words.txt --size 1000000
```

Then run the benchmarks:
```bash
./string_tests --benchmark --load my_words.txt --runs 5
```

