# Example

Instead of reading CAN frames from a CAN driver, the example generates them randomly.

Once a frame_packet is ready, the example just prints it to the console.

### Timestamp explanation

CAN messages with IDs 4 and 6 (part of the `Energy` group) are packaged together, once every 500ms. 

Can messages with IDs 2, 3, 5, and 7 (part of the `GPS` group) are packaged together, once every 600ms.

A frame packet is produced every 2000ms, ready to be sent over the network, or stored locally. These frequencies were set in `example.dbc`.

The signal values printed below are raw decoded bytes, and not scaled by the signal's factor or offset.

# Sample output:

```cmd
Parsed DBC in 1ms
New frame packet:
 t: 1683294790.320000s, can_id: 4
  SOCavg: 142
 t: 1683294790.320000s, can_id: 6
  RawBattCurrent: -10644
  SmoothBattCurrent: -10657
  BattVoltage: 32810
 t: 1683294790.420000s, can_id: 2
  GPSAccuracy: 81
  GPSLongitude: -66834125
  GPSLatitude: 132564085
 t: 1683294790.420000s, can_id: 3
  GPSAltitude: 4365
 t: 1683294790.420000s, can_id: 5
  GPSSpeed: 2392
 t: 1683294790.420000s, can_id: 7
  PowerState: 3
 t: 1683294790.820000s, can_id: 4
  SOCavg: 748
 t: 1683294790.820000s, can_id: 6
  RawBattCurrent: 11
  SmoothBattCurrent: 26666
  BattVoltage: 32744
New frame packet:
 t: 1683294791.020000s, can_id: 2
  GPSAccuracy: 69
  GPSLongitude: -68809371
  GPSLatitude: 9817485
 t: 1683294791.020000s, can_id: 3
  GPSAltitude: 1344
 t: 1683294791.020000s, can_id: 5
  GPSSpeed: 813
 t: 1683294791.020000s, can_id: 7
  PowerState: 2
 t: 1683294791.320000s, can_id: 4
  SOCavg: 445
 t: 1683294791.320000s, can_id: 6
  RawBattCurrent: 8640
  SmoothBattCurrent: 27
  BattVoltage: 32711
 t: 1683294791.620000s, can_id: 2
  GPSAccuracy: 100
  GPSLongitude: -128160893
  GPSLatitude: 89975602
 t: 1683294791.620000s, can_id: 3
  GPSAltitude: -2789
 t: 1683294791.620000s, can_id: 5
  GPSSpeed: 2751
 t: 1683294791.620000s, can_id: 7
  PowerState: 2
 t: 1683294791.820000s, can_id: 4
  SOCavg: 302
 t: 1683294791.820000s, can_id: 6
  RawBattCurrent: 33
  SmoothBattCurrent: 14
  BattVoltage: 32796
 t: 1683294792.220000s, can_id: 2
  GPSAccuracy: 105
  GPSLongitude: -95475031
  GPSLatitude: 98353453
 t: 1683294792.220000s, can_id: 3
  GPSAltitude: -2969
 t: 1683294792.220000s, can_id: 5
  GPSSpeed: 2302
 t: 1683294792.220000s, can_id: 7
  PowerState: 2
 t: 1683294792.320000s, can_id: 4
  SOCavg: 74
 t: 1683294792.320000s, can_id: 6
  RawBattCurrent: 29149
  SmoothBattCurrent: 29224
  BattVoltage: 32727
 t: 1683294792.820000s, can_id: 2
  GPSAccuracy: 122
  GPSLongitude: -109359055
  GPSLatitude: -8610630
 t: 1683294792.820000s, can_id: 3
  GPSAltitude: -3678
 t: 1683294792.820000s, can_id: 5
  GPSSpeed: 634
 t: 1683294792.820000s, can_id: 7
  PowerState: 3
 t: 1683294792.820000s, can_id: 4
  SOCavg: 619
 t: 1683294792.820000s, can_id: 6
  RawBattCurrent: -29289
  SmoothBattCurrent: 52
  BattVoltage: 32839
New frame packet:
 t: 1683294793.320000s, can_id: 4
  SOCavg: 443
 t: 1683294793.320000s, can_id: 6
  RawBattCurrent: -9264
  SmoothBattCurrent: -9271
  BattVoltage: 32820
 t: 1683294793.420000s, can_id: 2
  GPSAccuracy: 5
  GPSLongitude: -124656872
  GPSLatitude: 1599866
 t: 1683294793.420000s, can_id: 3
  GPSAltitude: -3864
 t: 1683294793.420000s, can_id: 5
  GPSSpeed: 2868
 t: 1683294793.420000s, can_id: 7
  PowerState: 1
 t: 1683294793.820000s, can_id: 4
  SOCavg: 919
 t: 1683294793.820000s, can_id: 6
  RawBattCurrent: -21935
  SmoothBattCurrent: 27
  BattVoltage: 32764
 t: 1683294794.020000s, can_id: 2
  GPSAccuracy: 79
  GPSLongitude: -3247526
  GPSLatitude: -110910094
 t: 1683294794.020000s, can_id: 3
  GPSAltitude: 7631
 t: 1683294794.020000s, can_id: 5
  GPSSpeed: 3969
 t: 1683294794.020000s, can_id: 7
  PowerState: 3
 t: 1683294794.320000s, can_id: 4
  SOCavg: 151
 t: 1683294794.320000s, can_id: 6
  RawBattCurrent: 28645
  SmoothBattCurrent: 28645
  BattVoltage: 32902
 t: 1683294794.620000s, can_id: 2
  GPSAccuracy: 11
  GPSLongitude: 11465855
  GPSLatitude: 127756354
 t: 1683294794.620000s, can_id: 3
  GPSAltitude: -7487
 t: 1683294794.620000s, can_id: 5
  GPSSpeed: 1947
 t: 1683294794.620000s, can_id: 7
  PowerState: 3
 t: 1683294794.820000s, can_id: 4
  SOCavg: 566
 t: 1683294794.820000s, can_id: 6
  RawBattCurrent: 83
  SmoothBattCurrent: -17953
  BattVoltage: 32803
```