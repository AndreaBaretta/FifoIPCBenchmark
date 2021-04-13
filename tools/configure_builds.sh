#!/bin/bash

# Copyright © 2021 Andrea Baretta

# This file is part of FifoIPCLatency.

# FifoIPCLatency is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# FifoIPCLatency is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with FifoIPCLatency.  If not, see <https://www.gnu.org/licenses/>.

set -e -o pipefail

mkdir -p builds/debug
mkdir -p builds/release

pushd builds/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
popd

pushd builds/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
popd
