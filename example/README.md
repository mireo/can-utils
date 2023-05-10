# Example

Instead of reading CAN frames from a CAN driver, the example generates them randomly.

Once a frame_packet is ready, it is decoded and printed to the console frame by frame, signal by signal.

### Timestamp explanation

CAN messages with IDs 4 and 6 (part of the `Energy` group) are packaged one after another, once every 500ms. 

Can messages with IDs 2, 3, 5, and 7 (part of the `GPS` group) are packaged one after another, once every 600ms.

A frame packet is produced every 2000ms, ready to be sent over the network, or stored locally. These frequencies are set in `example.dbc`.

The signal values printed below are raw decoded bytes, and not scaled by the signal's factor or offset.

# Example output:

```cmd
Parsed DBC in 0ms
New frame_packet (from 2051379 frames):
 can_frame at t: 1683709840.616000s, can_id: 4
  SOCavg: 743
 can_frame at t: 1683709840.616000s, can_id: 6
  RawBattCurrent: 10913
  SmoothBattCurrent: 10902
  BattVoltage: 32815
 can_frame at t: 1683709840.716000s, can_id: 2
  GPSAccuracy: 80
  GPSLongitude: -49263357
  GPSLatitude: -69043685
 can_frame at t: 1683709840.716000s, can_id: 3
  GPSAltitude: 668
 can_frame at t: 1683709840.716000s, can_id: 5
  GPSSpeed: 455
 can_frame at t: 1683709840.716000s, can_id: 7
  PowerState: 2
 can_frame at t: 1683709841.116000s, can_id: 4
  SOCavg: 85
 can_frame at t: 1683709841.116000s, can_id: 6
  RawBattCurrent: 20
  SmoothBattCurrent: 30162
  BattVoltage: 32730
 can_frame at t: 1683709841.316000s, can_id: 2
  GPSAccuracy: 2
  GPSLongitude: 28618090
  GPSLatitude: 120699021
 can_frame at t: 1683709841.316000s, can_id: 3
  GPSAltitude: -2233
 can_frame at t: 1683709841.316000s, can_id: 5
  GPSSpeed: 3911
 can_frame at t: 1683709841.316000s, can_id: 7
  PowerState: 1
 can_frame at t: 1683709841.616000s, can_id: 4
  SOCavg: 268
 can_frame at t: 1683709841.616000s, can_id: 6
  RawBattCurrent: 3
  SmoothBattCurrent: 13
  BattVoltage: 32756
 can_frame at t: 1683709841.916000s, can_id: 2
  GPSAccuracy: 72
  GPSLongitude: -36024324
  GPSLatitude: -75430669
 can_frame at t: 1683709841.916000s, can_id: 3
  GPSAltitude: 7540
 can_frame at t: 1683709841.916000s, can_id: 5
  GPSSpeed: 2965
 can_frame at t: 1683709841.916000s, can_id: 7
  PowerState: 2
New frame_packet (from 2121812 frames):
 can_frame at t: 1683709842.116000s, can_id: 4
  SOCavg: 574
 can_frame at t: 1683709842.116000s, can_id: 6
  RawBattCurrent: 10914
  SmoothBattCurrent: 10921
  BattVoltage: 32760
 can_frame at t: 1683709842.516000s, can_id: 2
  GPSAccuracy: 118
  GPSLongitude: -106019721
  GPSLatitude: 26758102
 can_frame at t: 1683709842.516000s, can_id: 3
  GPSAltitude: -8084
 can_frame at t: 1683709842.516000s, can_id: 5
  GPSSpeed: 2160
 can_frame at t: 1683709842.516000s, can_id: 7
  PowerState: 2
 can_frame at t: 1683709842.616000s, can_id: 4
  SOCavg: 163
 can_frame at t: 1683709842.616000s, can_id: 6
  RawBattCurrent: -27877
  SmoothBattCurrent: -27827
  BattVoltage: 32731
 can_frame at t: 1683709843.116000s, can_id: 2
  GPSAccuracy: 59
  GPSLongitude: -28059766
  GPSLatitude: -54755491
 can_frame at t: 1683709843.116000s, can_id: 3
  GPSAltitude: -1195
 can_frame at t: 1683709843.116000s, can_id: 5
  GPSSpeed: 2671
 can_frame at t: 1683709843.116000s, can_id: 7
  PowerState: 3
 can_frame at t: 1683709843.116000s, can_id: 4
  SOCavg: 946
 can_frame at t: 1683709843.116000s, can_id: 6
  RawBattCurrent: -7116
  SmoothBattCurrent: 68
  BattVoltage: 32819
 can_frame at t: 1683709843.616000s, can_id: 4
  SOCavg: 937
 can_frame at t: 1683709843.616000s, can_id: 6
  RawBattCurrent: -3167
  SmoothBattCurrent: 5
  BattVoltage: 32877
 can_frame at t: 1683709843.716000s, can_id: 2
  GPSAccuracy: 4
  GPSLongitude: -27846777
  GPSLatitude: -77214133
 can_frame at t: 1683709843.716000s, can_id: 3
  GPSAltitude: -7707
 can_frame at t: 1683709843.716000s, can_id: 5
  GPSSpeed: 465
 can_frame at t: 1683709843.716000s, can_id: 7
  PowerState: 1
New frame_packet (from 2124536 frames):
 can_frame at t: 1683709844.116000s, can_id: 4
  SOCavg: 123
 can_frame at t: 1683709844.116000s, can_id: 6
  RawBattCurrent: -15248
  SmoothBattCurrent: 25
  BattVoltage: 32719
 can_frame at t: 1683709844.316000s, can_id: 2
  GPSAccuracy: 18
  GPSLongitude: 78674066
  GPSLatitude: -57802922
 can_frame at t: 1683709844.316000s, can_id: 3
  GPSAltitude: 3343
 can_frame at t: 1683709844.316000s, can_id: 5
  GPSSpeed: 171
 can_frame at t: 1683709844.316000s, can_id: 7
  PowerState: 1
 can_frame at t: 1683709844.616000s, can_id: 4
  SOCavg: 841
 can_frame at t: 1683709844.616000s, can_id: 6
  RawBattCurrent: 55
  SmoothBattCurrent: -1994
  BattVoltage: 32913
 can_frame at t: 1683709844.916000s, can_id: 2
  GPSAccuracy: 12
  GPSLongitude: 4703531
  GPSLatitude: -51589001
 can_frame at t: 1683709844.916000s, can_id: 3
  GPSAltitude: -694
 can_frame at t: 1683709844.916000s, can_id: 5
  GPSSpeed: 2184
 can_frame at t: 1683709844.916000s, can_id: 7
  PowerState: 1
 can_frame at t: 1683709845.116000s, can_id: 4
  SOCavg: 468
 can_frame at t: 1683709845.116000s, can_id: 6
  RawBattCurrent: -31401
  SmoothBattCurrent: -31395
  BattVoltage: 32717
 can_frame at t: 1683709845.516000s, can_id: 2
  GPSAccuracy: 107
  GPSLongitude: -8775822
  GPSLatitude: 87438303
 can_frame at t: 1683709845.516000s, can_id: 3
  GPSAltitude: 4875
 can_frame at t: 1683709845.516000s, can_id: 5
  GPSSpeed: 2672
 can_frame at t: 1683709845.516000s, can_id: 7
  PowerState: 3
 can_frame at t: 1683709845.616000s, can_id: 4
  SOCavg: 903
 can_frame at t: 1683709845.616000s, can_id: 6
  RawBattCurrent: -13414
  SmoothBattCurrent: 39
  BattVoltage: 32818
```