# ImmutableString

Yet another immutable C++ string implementation.
================================================

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
