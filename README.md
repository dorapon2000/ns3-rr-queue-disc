# ns3-rr-queue-disc
Round Robin Queue Disc for ns-3.29

## Environment
- Ubuntu 16.04
- ns3.29

## Usage
```sh
$ git clone git@github.com:dorapon2000/ns3-rr-queue-disc.git
```

Copy src/ and scratch/ to your ns-3.29/ directory.

If your ns-3.29/ directory is placed in home directory, use following script.
Note that script overwrites files.

```sh
$ cd ns3-rr-queue-disc
$ chmod +x update.sh
$ ./update.sh
```

cd to your ns-3.29/ and build.

```sh
$ ./waf build
```

Here is sample senario.

```sh
$ ./waf --run "my-pfifo-vs-red-vs-rr --queueDiscType=RR"
```

## Detail
RRQueueDisc has multiple internal queue.
The number of internal queue is limited by Attribute "QueueNum" (defualt 10).
For instance, assume 11 flows flow and QueueNum is 10, 11th flow is dropped by "Too many flows".
You can change QueueNum by Config::SetDefault().

```cpp
Config::SetDefault ("ns3::RRQueueDisc::QueueNum", UintegerValue (11));
```

RRQueueDisc classifies flows by hash (src_ip, src_port, dst_ip, dst_port, protocol).
However, if the number of flows is less than QueueNum, the hashes do not collided.

This module does not consider deleting flow.

## References
- ns3によるネットワークシミュレーション
