#!/bin/bash

libtoolize && aclocal && autoconf && autoheader && cp src/config.h.in . && automake --add-missing
