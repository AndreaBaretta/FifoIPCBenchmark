# FifoIPCBenchmark

A test to specifically benchmark the latency of interprocess communication using a shared-memory FIFO object. 

## Installation

To install the project, you can start by cloning the repo and running the cmake configuration bash script in `tools`.

```shell
git clone git@github.com:AndreaBaretta/FifoIPCBenchmark.git
cd FifoIPCBenchmark
./tools/configure_builds.sh
```

You can now move into either `builds/release` or `builds/debug` and compile the benchmark in either release mode or test mode. Also, make sure that `cmake` is installed.

```shell
cd ./builds/release
cmake --build .
```

You can now run the executables as sudo.

```shell
sudo ./fifo-ipc-latency
sudo ./fifo-ipc-throughput --intra-ccx
```

## Usage

### Latency measurements

`clang-13` is recommended for compilation, and it is the compiler used to obtain the sample data:
```shell
sudo apt-get install clang-13
```

The c++ standard used for compilation is c++20.

Once you have compiled the benchmark as previously described and have the executable, it can take several command-line arguments:

* `-n, --numtries arg`: Number of iterations of the test (default: 1000000)
* `-i, --individual-message`: Save latency data on individual messages
* `-d, --data-dir arg`: Directory for gathered data. If used with -i, it will save latency data on individual messages. If not specified, uses present working directory
* `--cores arg`: If not specified, tests all cores communicating with each other. If one value is specified, then that is fixed as the reference core. If two values are specified, only those two cores are tested
* `-b, --buffer-size arg`: Size of the buffer in the shared FIFO in number of messages (default: 8)
* `-m, --message-size arg`: Size of the message sent to FIFO in number of cache lines (default: 1)
* `--no-save`: If used, gathered data is not stored and does not override any previously gathered data. This is largely for development purposes so you don't kill your SSD

##### Note: All of this information is displayed when running the program with `-h` or `--help`.

For example, you can measure the average latency of sending 100,000 messages, each one spanning 3 cache lines, across cores 1 and 18 through the following command:

```shell
sudo ./fifo-ipc-latency -n 100000 -m 3 --cores 1,18
```

Or, if you wish to test the benchmark by measuring the latency of individual messages sent between all cores but you do not wish to overwrite your current csv files, you can run:

```shell
sudo ./fifo-ipc-latency -i --no-save
```

Please note that to make full use of the benchmark, it is recommended to run the executable as sudo, set the CPU governor to 'performance', and disable CPU C-States in your BIOS.

Lastly, to visualize the data, you can move into the `python` folder.

```shell
cd ../../python
```

Here, you will find a python script which generates a heatmap of the average latency. To see this, run:

```shell
python3 heatmap_avg.py <file-path>
```

If you specify a file path, it will use that particular file. Otherwise, by default, it will use the measurements in `/builds/release/data`.

You can also visualize the distribution of the data gathered from saving the latency of individual measurements by using the following python script:

```shell
python3 dist_individual.py <file-path>
```

Once again, you can specify a file path. If you do not, it will look at `builds/release/data/FifoIpcLatency_0_1.csv`.

Pyhton 3 is necessary to run these scripts. Also, please make sure you have the following packages installed:

```shell
pip3 install numpy pandas matplotlib seaborn
```

### Throughput measurements

`clang-13` is once again recommended for compilation, and it is the compiler used to obtain the sample data. The c++ standard used for compilation is stil  c++20.

```shell
sudo apt-get install clang-13
```

The executable `fifo-ipc-throughput` can take multiple command-line arguments:

* `--ccx arg`: Number of CCXs per socket (default: 1)
* `--sockets arg`: Number of sockets in the system
* `--intra-core`: Tests the throughput of message across two threads in the same physical core
* `--intra-ccx`: Tests the throughput of messages across pairs of cores in the same CCX
* `--intra-ccx-all`: Tests the throughput of messages across all pairs of cores in one CCX on all CCXs at the same time
* `--inter-ccx`: Tests communication across all pairs of CCXs in one socket
* `--inter-socket`: Tests the throughput of messages across all pairs of cores between sockets
* `--mt`: Perform this test with simultaneous multithreading
* `-b, --buffer-size arg`: Size of the buffer in the shared FIFO in number of messages (default: 8)
* `-m, --message-size arg`: Size of the message sent to FIFO in number of cache lines (default: 1)
* `-n, --numtries arg`: Number of iterations of the test (default: 1000000)
* `-d, --data-dir arg`: Directory for gathered data. If not specified, uses present working directory

##### Note: All of this information is displayed when running the program with `-h` or `--help`.

For example, you can measure the throughput of sending 1,0000,000 messages, with a buffer of 100 cache lines, across CCXs with the following command:

```shell
sudo ./fifo-ipc-throughput -n 10000000 -b 100 --inter-ccx
```

Or, if you wish to test the throughput of intra-CCX communication across all CCXs simultaneously with a custom data directory, you can run:

```shell
sudo ./fifo-ipc-throughput -d /home/andrea/My/Data/Directory --intra-ccx-all 
```

Once again, to make full use of the benchmark, it is recommended to run the executable as sudo, set the CPU governor to 'performance', and disable CPU C-States in your BIOS.

Lastly, to visualize the data, you can move into the `python` folder.

```shell
cd ../../python
```

Here, you will find a python script which generates a box and whisker plot of the distribution of throughpuyt measurements for all tests. To see this, run:

```shell
python3 throughput_plot.py <data-dir>
```

If you specify a file path, it will use that particular file. Otherwise, by default, it will use the measurements in `/builds/release/data`.

## Contributions

Contributions are appreciated, but please keep in mind that this benchmark specifically measures the latency and throughput of communication using shared memory FIFOs.

Feel free to open an issue or send a pull request.

<!-- ## [License](http://goldsborough.mit-license.org) -->
## License

This project is released under the [GNU General Public License](https://www.gnu.org/licenses/gpl-3.0.en.html). For more information, see the COPYING file.

    This file is part of FifoIPCBenchmark.

    FifoIPCBenchmark is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FifoIPCBenchmark is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FifoIPCBenchmark.  If not, see <https://www.gnu.org/licenses/>.

## Authors

[Andrea Baretta](https://github.com/AndreaBaretta)

Copyright © 2021 Andrea Baretta