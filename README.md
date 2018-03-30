
# CWICLO

This work-in-progress is a C++ merge-rewrite of
[casycom](https://github.com/msharov/casycom) and
[uSTL](https://github.com/msharov/ustl). The result is an asynchronous COM
framework with some vaguely standard-library-like classes implemented
without rtti or exceptions, allowing the executables to not link
to libstdc++ or libgcc\_s. The intended purpose is writing low-level
system services in C++. Such services may have to run before /usr/lib and
libstdc++.so are available, and so must not be linked with anything other
than libc. They should, in fact, be linked statically, once you find a
libc that does not require a megabyte to implement printf and strcpy.

Compilation requires C++17 support, so use gcc 7.

> ./configure && make check && make install

Read the files in the [test/ directory](test) for usage examples.
Once the project is more complete, additional documentation will be written.

Report bugs on [project bugtracker](https://github.com/msharov/cwiclo/issues).
