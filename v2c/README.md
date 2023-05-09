# V2C Transcoder 

V2CTranscoder is a middleware tool that aggregates and groups CAN data frames. 
It is designed to run on a device connected to a CAN bus that sends aggregated frames to a remote server or stores it locally.

The transcoder takes CAN frames as input, aggregates their values, and encodes them back into CAN frames. 
The resulting frames are more suitable for sending over the network due to their reduced frequency.
Frames received by the server will appear identical to frames sent over CAN, making the tool transparent from the server's perspective.

The tool can be configured to send different messages at different frequencies, or to send the average value of a signal over a time window.

# Usage

To use V2CTranscoder, initialize `v2c_transcoder` and call its `transcode(t, frame)` member method with frames read from the CAN socket. 

```cpp
can_frame frame = read_frame();

auto fp = transcoder.transcode(std::chrono::system_clock::now(), frame);
```

The resulting `fp` is of the type can::frame_packet, defined in [frame_packet.h](can/frame_packet.h), and is a serialized list of CAN frames with timestamp information.

The frame packet will be empty except when more than `V2CTxTime` milliseconds have passed since the last transmission.

## frame_packet interface

A `frame_packet` can be iterated on to get raw frames with their timestamps:

```cpp
for (const auto& [ts, frame] : fp) {
	// use frame.can_id, frame.data
}
```

`frame` is of the type `can_frame`, defined in [can_kernel.h](can/can_kernel.h), and `ts` is a `std::chrono::system_clock::timepoint`.

The `frame_packet` class offers the following methods to access its raw data, in order to send the bytes over the network:

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

All V2C configuration is done in the input DBC file, in DBC syntax, using attributes and environemntal variables.

## Transmission

```py
EV_ V2CTxTime: 0 [0|60000] "ms" 2000 1 DUMMY_NODE_VECTOR1 V2C;
```

The `V2CTxTime` environmental variable is used to set the time between two transmissions. Each transmission contains all aggregated frames since the last transmission, in one `frame_packet`.

## Grouping

Groups are defined by a set of message ids and a frequency.

The messages in a group are treated as a single unit: if a single message in a group does not arrive over the CAN, the whole group is not sent.

This guarantees that there is no outdated data being sent. 

To define a new group, add a new environmental variable with the `GroupTxFreq` suffix.

The values of these variables define the sending period of that group, in milliseconds. (600ms, 500ms)

The sending frequency (2000ms) doesn't have to match messages aggregation frequencies.

```py
EV_ GPSGroupTxFreq: 0 [0|60000] "ms" 600 11 DUMMY_NODE_VECTOR1 V2C;
EV_ EnergyGroupTxFreq: 0 [0|60000] "ms" 500 13 DUMMY_NODE_VECTOR1 V2C;
```

For signals that change slowly over time, such as from temperature sensors, it is useful to send them less frequently.

To add a message to a group, set its "TxGroupFreq" to the groupname:

```py
BA_ "TxGroupFreq" BO_ 2 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 3 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 5 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 7 "GPSGroupTxFreq";

BA_ "TxGroupFreq" BO_ 4 "EnergyGroupTxFreq";
BA_ "TxGroupFreq" BO_ 6 "EnergyGroupTxFreq";
```

The attributes above assign messages `GPSLatLong`, `GPSAltitude`, `GPSSpeed` and `PowerState` to the `GPSGroupTxFreq` group, 
and messages `SOC` and `BatteryCurrent` to the `EnergyGroupTxFreq` group.

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

For `example.dbc` described above, `transcoder.transcode(ts, frame)` will return a new `frame_packet` every 2000ms, 
which will contain up to four `GPS` groups and up to four `Energy` groups, ordered chronologically.
Each group will contain an aggregated `can_frame` for each message in that group.

See [example README](example/README.md) for a detailed look at output timestamps.

## Filtering

To filter out a signal, simply do not include it in any group. Only signals that are a part of a group are aggregated and appended to the resulting `frame_packet`.
