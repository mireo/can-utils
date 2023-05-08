# V2C Transcoder 

V2CTranscoder is a transparent middleware tool for aggregating and grouping CAN data frames. 

It takes CAN frames as input and outputs aggregated CAN frames, which are more suitable for sending over the network.

It's meant to be run on a device connected to a CAN bus, that can send the aggregated data to a remote server, or store it locally.

## Motivation

- filtering and aggregating CAN data is a common problem
- vehicles produce too much CAN data too quickly to send over an unreliable network
- some data is more important than other data
- some messages are similiar in meaning and should be sent together
- some messages change slowly and can be sent less frequently
- some messages change quickly and should be sent more frequently
- v2c-transcoder serves to transparently aggregate and group CAN data frames 

# Usage

Initialize a `v2c_transcoder` and call its `transcode(t, frame)` member method with frames read from the CAN socket. 

```cpp
	can_frame frame = read_frame();

	auto fp = transcoder.transcode(std::chrono::system_clock::now(), frame);
```

The resulting `fp` is of the type can::frame_packet, defined in [frame_packet.h](can/frame_packet.h), and is a serialized list of CAN frames with timestamp information.

The frame packet will be empty except when more than `V2CTxTime` milliseconds have passed since the last transmission.

A `frame_packet` can be iterated on to get the frames and their timestamps:
```cpp
for (const auto& [ts, frame] : fp) {
	// use frame.can_id, frame.data
}
```

`frame` is of the type `can_frame`, defined in [can_kernel.h](can/can_kernel.h), and `ts` is a `std::chrono::system_clock::timepoint`.

## frame_packet


```cpp
const uint8_t* data_begin() const {
	return _buff.data();
}

const uint8_t* data_end() const {
	return _buff.data() + _buff.size();
}
```

On the server side, the frame packet can be constructed directly from a serialized buffer of bytes:

```cpp
std::vector<uint8_t> buffer(buffer_size);

read_socket.read_some(boost::asio::buffer(buffer));

can::frame_packet fp(std::move(buffer));
```

# Configuration

All V2C configuration is done in the input DBC file, in DBC syntax, using attributes and environemntal variables.

## Transmission

```
EV_ V2CTxTime: 0 [0|60000] "ms" 2000 1 DUMMY_NODE_VECTOR1 V2C;
```

The 'V2CTxTime' environmental variable is used to set the time between two transmissions. Each transmission contains all aggregated frames since the last transmission.

## Grouping

Groups are defined by a set of message ids and a frequency.

The messages in a group are treated as a single unit: if a single message in a group does not arrive over the CAN, the whole group is not sent.

For example, it 

This guarantees that there is no outdated data being sent. 

To define a new group, add a new environmental variable with the following format: (TODO: bad sentence)

A groupname has `GroupTxFreq' as its suffix and has a sending period in milliseconds:

```py
EV_ GPSGroupTxFreq: 0 [0|60000] "ms" 600 11 DUMMY_NODE_VECTOR1 V2C;
EV_ EnergyGroupTxFreq: 0 [0|60000] "ms" 500 13 DUMMY_NODE_VECTOR1 V2C;
```

To add a message to a group, set its "TxGroupFreq" to the groupname:

```py
BA_ "TxGroupFreq" BO_ 2 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 3 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 5 "GPSGroupTxFreq";
BA_ "TxGroupFreq" BO_ 7 "GPSGroupTxFreq";

BA_ "TxGroupFreq" BO_ 4 "EnergyGroupTxFreq";
BA_ "TxGroupFreq" BO_ 6 "EnergyGroupTxFreq";
```

This excerpt assigns messages `GPSLatLong`, `GPSAltitude`, `GPSSpeed` and `PowerState` to the `GPSGroupTxFreq` group, and messages `SOC`, `BatteryCurrent` to the `EnergyGroupTxFreq` group.

## Aggregating

Two aggregators are supported: 

* `LAST` is used to send the most recent value of a signal within a group. This is the **default**.
* `AVG` is used to send the average value of a signal within a group.

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


