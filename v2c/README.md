# V2C Transcoder 

V2CTranscoder is a middleware tool that aggregates and groups CAN data frames. 
It is designed to run on a device connected to a CAN bus that sends aggregated frames to a remote server or stores them locally.

The transcoder takes CAN frames as input, aggregates their values, and encodes them back into CAN frames. 
The resulting frames are more suitable for sending over the network because of their reduced frequency.
Frames received by the server will appear identical to frames sent over CAN, making the tool transparent from the server's perspective.

The tool can be configured to send different messages at different frequencies or to send the average value of a signal over a time window.

# Usage

To use V2CTranscoder, initialize `v2c_transcoder` and call its `transcode(t, frame)` method with frames read from the CAN socket. 

```cpp
can_frame frame = read_frame();

auto fp = transcoder.transcode(std::chrono::system_clock::now(), frame);
```

The result `fp` is of the type `can::frame_packet`, defined in [frame_packet.h](/can/frame_packet.h), and is a serialized list of CAN frames with timestamp information.

The frame packet is not sent unless more than `V2CTxTime` milliseconds have passed since the last transmission.

## frame_packet interface

A `frame_packet` can be iterated to get raw frames with their timestamps:

```cpp
for (const auto& [ts, frame] : fp) {
	// use frame.can_id, frame.data
}
```

`frame` is of type `can_frame`, defined in [can_kernel.h](/can/can_kernel.h), and `ts` is a `std::chrono::system_clock::timepoint`.

The class `frame_packet` provides the following methods for accessing its raw data, to send the bytes over the network:

```cpp
const uint8_t* data_begin() const {
	return _buff.data();
}

const uint8_t* data_end() const {
	return _buff.data() + _buff.size();
}

std::vector<uint8_t> release() {
	return std::move(_buff);
}
```

On the server side, the same `frame_packet` can be recreated directly from a byte buffer as such:

```cpp
std::vector<uint8_t> buffer(buffer_size);

read_socket.read_some(boost::asio::buffer(buffer));

can::frame_packet fp(std::move(buffer));
```

# Configuration

All V2C configuration is in the DBC input file, in DBC syntax, using attributes and environment variables.

## Transmission

```py
EV_ V2CTxTime: 0 [0|60000] "ms" 2000 1 DUMMY_NODE_VECTOR1 V2C;
```

The environment variable `V2CTxTime` is used to specify the time between two transmissions. 
Each transmission contains all aggregated frames since the last transmission in a `frame_packet`.

## Grouping

Groups are defined by a set of message IDs and a frequency.

The messages of a group are treated as a single unit: If a single message of a group does not arrive over the CAN, the entire group is not sent.

This ensures that no outdated data is sent. 

To define a new group, add a new environment variable with the suffix `GroupTxFreq`.

The value of this variable determines the sending period of this group in milliseconds. (600ms, 500ms)

The sending frequency (2000ms) does not have to match message aggregation frequencies.

```py
EV_ GPSGroupTxFreq: 0 [0|60000] "ms" 600 11 DUMMY_NODE_VECTOR1 V2C;
EV_ EnergyGroupTxFreq: 0 [0|60000] "ms" 500 13 DUMMY_NODE_VECTOR1 V2C;
```

For signals that change slowly over time, such as from temperature sensors, it makes sense to send them less frequently.

To add a message to a group, set its "TxGroupFreq" to the group name:

```py
BA_ "TxGroupFreq" BO_ 2 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 3 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 5 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 7 "GPSGroupTxFreq";

BA_ "TxGroupFreq" BO_ 4 "EnergyGroupTxFreq";
BA_ "TxGroupFreq" BO_ 6 "EnergyGroupTxFreq";
```

The above attributes assign messages `GPSLatLong`, `GPSAltitude`, `GPSSpeed`, and `PowerState` to the `GPSGroupTxFreq` group, 
and the messages `SOC` and `BatteryCurrent` to the `EnergyGroupTxFreq` group.

## Aggregating

Two aggregators are supported: 

* `LAST` is used to send the most recent value of the signal. This is the **default**.
* `AVG` is used to send the average value of the signal.

To set a signal's aggregation type, set its "AggType" attribute to either `LAST` or `AVG`:

```py
BA_ "AggType" SG_  2 GPSAccuracy "LAST";
BA_ "AggType" SG_  2 GPSLongitude "LAST";
BA_ "AggType" SG_  2 GPSLatitude "LAST";
BA_ "AggType" SG_  3 GPSAltitude "LAST";
BA_ "AggType" SG_  5 GPSSpeed "LAST";
BA_ "AggType" SG_  7 PowerState "LAST";
BA_ "AggType" SG_  4 SOCavg "LAST";
BA_ "AggType" SG_  6 RawBattCurrent "AVG";
BA_ "AggType" SG_  6 SmoothBattCurrent "AVG";
BA_ "AggType" SG_  6 BattVoltage "AVG";
```

The aggregation is done in time windows that match the group's sending period. No single CAN measurement is sent or aggregated more than once.

For `example.dbc` described above, `transcoder.transcode(ts, frame)` returns a new `frame_packet` every 2000 ms
containing up to four `GPS` groups and up to four `Energy` groups, in chronological order.
Each group contains an aggregated `can_frame` for each message in that group.

## Filtering

To filter out a signal, simply do not include it in any group. Only signals that are part of a group are aggregated and appended to the resulting `frame_packet`.

## Multiplexing

Messages are often multiplexed by a switch that determines which signal is currently active:

```py
BO_ 10 MuxedMessage: 4 DUMMY_NODE_VECTOR1
 SG_ MuxSwitch M : 0|8@1+ (1,0) [0|255] "" Receiver
 SG_ Signal1 m1 : 8|8@1+ (2,0) [0|510] "" Receiver
 SG_ Signal2 m2 : 8|8@1+ (0.5,0) [0|127.5] "" Receiver
 SG_ Signal3 m3 : 8|8@1+ (1,0) [0|255] "" Receiver
 Sg_ Signal4 : 16|16@1+ (0.05,0) [-1000|1000] "" Receiver
```

Signals 1 to 3 all occupy the same bit range, and are multiplexed by `MuxSwitch`.

This means that if we want to calculate the average of all three signals separately, their average values would not fit into a single `can_frame`.

To solve this problem, the final `frame_packet` contains a separate `can_frame` for each multiplexed signal value.

For the above message, three `can_frames` are generated, each encoded with one of the multiplexed signals (and the corresponding mux switch value).

`Signal4` will be aggregated as usual, from every input `can_frame` regardless of its mux switch value.

To avoid duplication on the server side, `Signal4` is encoded only in the first `can_frame`, 
and the other frames are marked as duplicates by setting their `frame.__res0` byte to `1`.
This is a reserved padding byte not used for anything else.

## DBC Parser Integration

The transcoder only has to use six different `tag_invokes` for DBC parsing and data structure initialization.

The short parsing functions can be found at the end of [v2c_transcoder.h](v2c_transcoder.h).
