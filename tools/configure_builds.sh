#!/bin/bash

# Copyright © 2021 Andrea Baretta

# This file is part of FifoIPCBenchmark.

# FifoIPCBenchmark is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# FifoIPCBenchmark is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with FifoIPCBenchmark. If not, see <https://www.gnu.org/licenses/>.

set -e -o pipefail

function echodo () {
        set -e -o pipefail
        echo "[BEGIN] $@" >&2
        "$@"
        echo "[END  ] $@" >&2
}


export CC=clang-13
export CXX=clang++-13
echodo mkdir -p builds/debug
echodo mkdir -p builds/release

echodo pushd builds/debug
echodo cmake -DCMAKE_BUILD_TYPE=Debug ../..
echodo popd

echodo pushd builds/release
echodo cmake -DCMAKE_BUILD_TYPE=Release ../..
echodo popd
