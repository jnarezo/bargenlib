# bargenlib

A barcode image generator and exporter for C++11.

## Documentation

**bargenlib** supports bitmap (.bmp) and png (.png), grayscale and alpha image encoding.

The library can be accessed using the `bargenlib` namespace
which contains 2 enumerators and 1 function:

* The `Encoding` enum which specifies your code's encoding as a barcode. The library
currently provides and supports `UPC_A`, `EAN_8`, and `EAN_13`.
* The `FileType` enum which specifies your image's encoding. The library currently only supports
bitmap (.bmp), png (.png) png with alpha/transparency (.png). These file types are accessible
through `BMP`, `PNG`, and `PNG_A`, respectively.
* The `save()` function which takes a `std::vector<int>` code that will be used to generate
your barcode image, a string to your file path like `../../../my_barcode.bmp`,
an `Encoding` enum to specify your code's encoding format, and a `FileType` enum to
specify your image's encoding format:

    `void save(const std::vector<int> &code, const std::string &path, Encoding codeType, FileType fileType)`

Slight additional documentation can be found in `bargenlib.h`.

## Building

The following files are needed to utilize the library:

* `bargenlib.h`
* `bargenlib.cpp`
* `lodepng.h`
* `lodepng.cpp`

The files inside the include folder may be placed in your projects include
directory as is (`lodepng.h` must be in root). No other library is necessary.

## Compiling

If you have some `my_program.cpp` that uses **bargenlib**, simply build the source files
and make sure the contents of the `include` directory is placed in a specified include directory,
like so:

`g++ my_program.cpp lodepng.cpp bargenlib.cpp` (the `include` contents, located in the project root)

or:

`clang++ -Iinclude my_program.cpp lodepng.cpp bargenlib.cpp` (the `include` directory is specified)

You may use whatever compiler flags you need for your specific
compiler or build system for your project instead of these commands.

## Credits

Credit to [lodepng](https://github.com/lvandeve/lodepng) for supplying the code for encoding png images.

You may want to take a look at `lodepng.h` or the [lodepng](https://github.com/lvandeve/lodepng)
repository for additional compiler flags to disable compilation of unused sections of **lodepng**
*at your own discretion*, like the decoder.
