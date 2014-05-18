# Clang Key Path Validator

This is a clang plug-in that performs checking on literal key paths passed to `valueForKeyPath:` and others.

More info in [dev etc: Safe and Sane Key Paths](http://localhost:4000/code/2014/05/17/safe-and-sane-key-paths.html)

To download and try it out, go to the [releases page](https://github.com/jmah/Clang-KeyPathValidator/releases).

<a href="http://devetc.org/assets/2014-05-17-safe-and-sane-key-paths/warnings.png"><img alt="Key path warnings in Xcode" src="http://devetc.org/assets/2014-05-17-safe-and-sane-key-paths/warnings.png" width="640"></a>

## Development

The current version of the plug-in works with release_34 of LLVM and clang.
Clone the repository to `llvm/tools/clang/examples/Clang-KeyPathValidator` and run `make` to compile the plugin, and `make run` to do a diagnostic pass over the files in the `tests` directory.

## TODO

There are a bunch of tasks tracked in GitHub Issues.

## License

The KeyPathValidator plug-in is released under the same license as LLVM, the [University of Illinois/NCSA Open Source License](http://llvm.org/releases/3.4/LICENSE.TXT).

    Copyright (c) 2014 Jonathon Mah.
    All rights reserved.
    
    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal with
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
        * Redistributions of source code must retain the above copyright notice,
          this list of conditions and the following disclaimers.
    
        * Redistributions in binary form must reproduce the above copyright notice,
          this list of conditions and the following disclaimers in the
          documentation and/or other materials provided with the distribution.
    
        * Neither the names of the LLVM Team, University of Illinois at
          Urbana-Champaign, nor the names of its contributors may be used to
          endorse or promote products derived from this Software without specific
          prior written permission.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
    SOFTWARE.
