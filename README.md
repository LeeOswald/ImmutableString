# ImmutableString

Yet another immutable C++ string implementation.
================================================

1. basic_immutable_string object can hold a reference to a string literal, no memory allocations involved in this case.
```
immutable_string str("I hold only a pointer and size", immutable_string::FromStringLiteral);
```

2. reference-counted data
```
immutable_string str1("I share a reference to actual data");
immutable_string str2(str1); // share the same data
```

3. *almost* zero-cost copying. Copying a basic_immutable_string instance costs as much as one atomic increment and two pointer-size member copyings.

4. short string optimization (SSO). Strings up to 16 bytes (on x64) long (including null terminator) are stored inside basic_immutable_string object, no additional allocations.


Cloning
--------
```bash
git clone --recurse-submodules https://github.com/LeeOswald/ImmutableString.git

```

Building
--------
This is a header-only library; however, tests require building.
On Windows, open Visual Studio Command Propmpt (or just find and run vcvars64.bat from VS installation folder).

```bash
cd ImmutableString
mkdir build
cd build
cmake ..
cmake . --build --target tests
```
