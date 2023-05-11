# DBC Parser

A simple, lightweight, and *complete* C++ DBC parser that follows the [DBC format](http://mcu.so/Microcontroller/Automotive/dbc-file-format-documentation_compress.pdf) specification.

## Usage Example

```cpp
#include "dbc/dbc_parser.h"

...

custom_dbc dbc_impl; // custom class that implements your logic 

bool success = can::parse_dbc(dbc_content, std::ref(dbc_impl)); // parses the DBC and invokes callbacks on dbc_impl for every line

// dbc_impl is now populated by the parser and can be used
```

This parser does not generate any classes or structs from the DBC file. Instead, it calls user-defined callbacks for each line of the input DBC file. Callbacks receive the values parsed from the current line as arguments.

Users should use values provided in callback arguments to create their own data structures or directly perform any other required action.

In most cases, a CAN DBC file is used to configure the behavior of an edge node participating in CAN. In that sense, a DBC file is a configuration file that describes the semantics of CAN messages and the overall structure and roles of nodes in CAN.

With DBC being a configuration file, it seems more natural to parse a DBC file and configure a CAN node without using a bunch of intermediate structures like AST or C++ structures that restate DBC information in a C++ manner just to extract pieces of information from them once again to configure CAN node. Therefore, direct parsing and "interpreting" values could be a better option for most CAN DBC use cases.

A parser callback has the form of:

```cpp
inline void tag_invoke(
	def_<callback_name>_cpo, your_class& this_,
	(callback_arguments ...)
) {
	// your logic here
}
```

It can either be declared as a friend method of `your_class`, or as a free inline function inside the `can` namespace.

## Customization Example

The following example shows how to create a custom class that stores the names of signals for each message id using the parsed DBC data.

To do this, we need to implement only one callback: `def_sg_cpo`. It has access to `custom_dbc` through the `this_` argument.

```cpp

struct custom_dbc {
	std::map<uint32_t, std::vector<std::string>> sigs; // map of message id to vector of signal names

	friend void tag_invoke(
		can::def_sg_cpo, custom_dbc& this_,
		uint32_t msg_id, std::optional<unsigned> sg_mux_switch_val, std::string sg_name,
		unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
		double sg_factor, double sg_offset, double sg_min, double sg_max,
		std::string sg_unit, std::vector<size_t> receiver_ords
	) {
		this_.sigs[msg_id].push_back(sg_name); // add the signal name to the vector for the message id
	}
}
```

If you don't need to use some of the callbacks, you can omit them as shown in the example above.

### Performance:

The parser is highly performant. It can parse a 1 MB DBC file in less than 10ms on a 2.6GHz CPU.

### See Also:

For a complete example of a custom class that implements these callbacks, see [v2c_transcoder.h](../../include/v2c/v2c_transcoder.h). It supports CAN frame encoding and decoding.

For a header file that contains all the callback signatures that you can copy and paste, see [parser_template.h](parser_template.h).

For an in-depth explanation of how the parser uses [tag_invokes](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1895r0.pdf) to call the customization methods, see this link. 

### Keyword Links:
Below is a list of all the callbacks you can implement to customize how the parser works. 

Each entry has the callback signature and a sample DBC line that would trigger the callback,
along with the values that the parser would pass as arguments in that case.

Some keywords have more than one tag_invoke method with different signatures, depending on the context.

- [VERSION](#version) - Version
- [BU](#bu) - Nodes
- [VAL_TABLE](#val_table) - Value Tables
- [BO](#bo) - Messages
- [BO_TX_BU](#bo_tx_bu) - Message Transmitters
- [SG](#sg) - Signals
- [EV](#ev) - Environment Variables
- [ENVVAR_DATA](#envvar_data) - Environment Variables Data
- [SIG_VALTYPE](#sig_valtype) - Signal Types
- [CM](#cm-global) - Comments
- [BA_DEF](#ba_def-int) - Attribute Definitions
- [BA_DEF_DEF](#ba_def_def) - Attribute Defaults
- [BA](#ba) - Attribute Values
- [VAL](#val-env) - Value Descriptions
- [SG_MUL_VAL](#sg_mul_val) - Extended Multiplexing

---

## VERSION
``` cpp
inline void tag_invoke(
	def_version_cpo, your_class& this_,
	std::string version
) {
}
```

#### DBC example:
```VERSION "1.0"```
<details><summary>Parsed as:</summary>
 
   - version = "1.0"
</details>
<br/>

---

## BU
``` cpp
inline void tag_invoke(
	def_bu_cpo, your_class& this_,
	std::vector<std::string> nodes
) {
}
```

#### DBC example:
```BU_: MASTER SLAVE01 SLAVE02 Receiver```
<details><summary>Parsed as:</summary>
 
   - nodes = { "MASTER", "SLAVE01", "SLAVE02", "Receiver" }
</details>
<br/>

---

## VAL_TABLE

```cpp
inline void tag_invoke(
	def_val_table_cpo, your_class& this_,
	std::string table_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}
```

#### DBC example:
```VAL_TABLE_ DriverMode 0 "Off" 1 "On" 2 "Guest";```

<details><summary>Parsed as:</summary>

   - table_name = "DriverMode"
   - val_descs = { { 0, "Off" }, { 1, "On" }, { 2, "Guest" } }
</details>
<br/>

---

## BO

``` cpp
inline void tag_invoke(
	def_bo_cpo, your_class& this_,
	uint32_t msg_id, std::string msg_name, size_t msg_size, size_t transmitter_ord
) {
}
```

#### DBC example:
```BO_ 100 Temperature: 8 Receiver```
<details><summary>Parsed as:</summary>
 
   - msg_id = 100
   - msg_name = "Temperature"
   - msg_size = 8
   - transmitter_ord = 3 
     - index in BU vector, nodes[3] = "Receiver"
</details>
<br/>


---
## SG
```cpp
inline void tag_invoke(
	def_sg_cpo, your_class& this_,
	uint32_t msg_id, std::optional<unsigned> sg_mux_switch_val, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	double sg_factor, double sg_offset, double sg_min, double sg_max,
	std::string sg_unit, std::vector<size_t> receiver_ords
) {
}
```

#### Notes:
This callback won't be invoked if the signal is the multiplexer switch. See also: [SG (MUX)](#sg-mux).

#### DBC examples:
```SG_ OuterTemperature1 : 50|10@1+ (0.5,-10) [-30|200] "C" SLAVE01 Receiver```
 <details><summary>Parsed as:</summary>
 
   - msg_id = 100
     - id of message that contains this signal
   - sg_mux_switch_val = { }
     - empty because this signal is not multiplexed
   - sg_name = "OuterTemperature1"
   - sg_start_bit = 50
   - sg_size = 10
   - sg_byte_order = '1'
   - sg_sign = '+'
   - sg_factor = 0.5
   - sg_offset = -10.0
   - sg_min = -30.0
   - sg_max = 200.0
   - sg_unit = "C"
   - receiver_ords = { 1,3 }
     - indices in BU vector
 </details>

---
 
 ```SG_ VIN_1 m12 : 30|8@0+ (1,0) [0|255] "" Receiver```
 <details><summary>Parsed as:</summary>
 
   - msg_id = 1020 (id of message that contains this signal)
   - sg_mux_switch_val = 12
   - sg_name = "VIN_1"
   - sg_start_bit = 30
   - sg_size = 8
   - sg_byte_order = '0'
   - sg_sign = '+'
   - sg_factor = 1.0
   - sg_offset = 0.0
   - sg_min = 0.0
   - sg_max = 255.0
   - sg_unit = ""
   - receiver_ords = { 3 }
      - indices in BU vector
  </details>
<br/>

---
## SG (MUX)

```cpp
inline void tag_invoke(
	def_sg_mux_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	std::string sg_unit, std::vector<size_t> receiver_ords
) {
}
```

#### Notes:
This callback will be invoked only for the multiplexer switch signal. See also: [SG](#sg).

#### DBC example: ```SG_ mux M : 0|8@1+ (1,0) [0|255] "" Receiver```
 <details><summary>Parsed as:</summary>
 
   - msg_id = 1020 
     - id of message that contains this signal
   - sg_name = "mux"
   - sg_start_bit = 0
   - sg_size = 8
   - sg_byte_order = '1'
   - sg_sign = '+'
   - sg_unit = ""
   - receiver_ords = { 3 }
     - indices in BU vector
 </details>
<br>

---

## BO_TX_BU
```cpp
inline void tag_invoke(
	def_bo_tx_bu_cpo, your_class& this_,
	unsigned msg_id, std::vector<std::string> transmitters
) {
}
```

#### DBC example:
```BO_TX_BU_ 101 "MASTER" "SLAVE";```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - transmitters = { "MASTER", "SLAVE" }
</details>
<br/>

---

## EV

``` cpp
inline void tag_invoke(
	def_ev_cpo, your_class& this_,
	std::string name, unsigned type, double ev_min, double ev_max,
	std::string unit, double initial, unsigned ev_id,
	std::string access_type, std::vector<size_t> access_nodes_ords
) {
}
```

#### DBC example:
```EV_ EngineSpeed: 0 [0|8031] "rpm" 0 7 DUMMY_NODE_VECTOR1 Receiver;```

 <details><summary>Parsed as:</summary>

   - name = "EngineSpeed"
   - type = 0
     - 0 = integer, 1 = float, 2 = string
   - ev_min = 0.0
   - ev_max = 8031.0
   - unit = "rpm"
   - initial = 0.0
   - ev_id = 7
   - access_type = "DUMMY_NODE_VECTOR1"
   - access_nodes_ords = { 3 }
     - indices in BU vector
</details>
<br/>

---

## ENVVAR_DATA
``` cpp
inline void tag_invoke(
	def_envvar_data_cpo, your_class& this_,
	std::string ev_name, unsigned data_size
) {
}
```

#### DBC example:
```ENVVAR_DATA_ EngineSpeed: 4;```

<details><summary>Parsed as:</summary>

   - ev_name = "EngineSpeed"
   - data_size = 4
</details>
<br/>

---


## SGTYPE
``` cpp
inline void tag_invoke(
	def_sgtype_cpo, your_class& this_,
	std::string sg_type_name, unsigned sg_size, char sg_byte_order, char sg_sign,
	double sg_factor, double sg_offset, double sg_min, double sg_max,
	std::string sg_unit, double sg_default_val, size_t val_table_ord
) {
}
```

#### Notes:
This callback will only be invoked for signal type declarations. See also: [SGTYPE (REF)](#sgtype-ref).

#### DBC example:
```SGTYPE_ TemperatureType : 16@0+ (0.125,-20) [-50,100] "C" 0.0 TempTable;```

<details><summary>Parsed as:</summary>

   - sg_type_name = "TemperatureType"
   - sg_size = 16
   - sg_byte_order = '0'
   - sg_sign = '+'
   - sg_factor = 0.125
   - sg_offset = -20.0
   - sg_min = -50.0
   - sg_max = 100.0
   - sg_unit = "C"
   - sg_default_val = 0.0
   - val_table_ord = 2
     - index in VAL_TABLE_ vector, val_tables[2] = "TempTable"
</details>
<br/>

---

## SGTYPE (REF)
``` cpp
inline void tag_invoke(
	def_sgtype_ref_cpo, your_class& this_,
	unsigned msg_id, std::string sg_name, std::string sg_type_name
) {
}
```

#### Notes:
This callback will only be invoked for signals declarations that use a previously defined signal type. See also: [SGTYPE](#sgtype).

#### DBC example:
```SGTYPE_ 101 TemperatureOuter TemperatureType;```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - sg_name = "TemperatureOuter"
   - sg_type_name = "TemperatureType"
</details>
<br/>

---

## SIG_GROUP
``` cpp
inline void tag_invoke(
	def_sig_group_cpo, your_class& this_,
	uint32_t msg_id, std::string sig_group_name, unsigned repetitions, std::vector<std::string> sig_names
) {
}
```

#### DBC example:
```SIG_GROUP_ 57 TempGroup2 4 : Temp01, Temp02, Temp03, Temp04;```

<details><summary>Parsed as:</summary>

   - msg_id = 57
   - sig_group_name = "TempGroup2"
   - repetitions = 4
   - sig_names = { "Temp01", "Temp02", "Temp03", "Temp04" }
</details>
</br>

---

## CM (GLOBAL)
```cpp
inline void tag_invoke(
	def_cm_glob_cpo, your_class& this_,
	std::string comment_text
) {
}
```

#### DBC example:
```CM_ "This is a global comment";```

<details><summary>Parsed as:</summary>

   - comment_text = "This is a global comment"
</details>
<br/>

---

## CM (BU)
```cpp
inline void tag_invoke(
	def_cm_bu_cpo, your_class& this_,
	unsigned bu_ord, std::string comment_text
) {
}
```

#### DBC example:
```CM_ BU_ Receiver "This is a comment for node Receiver";```

<details><summary>Parsed as:</summary>

   - bu_ord = 3
   - comment_text = "This is a comment for node Receiver"
</details>
<br/>

---

## CM (BO)
```cpp
inline void tag_invoke(
	def_cm_bo_cpo, your_class& this_,
	uint32_t msg_id, std::string comment_text
) {
}
```

#### DBC example:
```CM_ BO_ 101 "This is a comment for message 101";```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - comment_text = "This is a comment for message 101"
</details>
<br/>

---

## CM (SG)
```cpp
inline void tag_invoke(
	def_cm_sg_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name, std::string comment_text
) {
}
```

#### DBC example:
```CM_ SG_ 101 TemperatureOuter "This is a comment for signal TemperatureOuter in message 101";```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - sg_name = "TemperatureOuter"
   - comment_text = "This is a comment for signal TemperatureOuter in message 101"
</details>
<br/>

---

## CM (EV)
```cpp
inline void tag_invoke(
	def_cm_ev_cpo, your_class& this_,
	std::string ev_name, std::string comment_text
) {
}
```

#### DBC example:
```CM_ EV_ EngineSpeed "This is a comment for EV EngineSpeed";```

<details><summary>Parsed as:</summary>

   - ev_name = "EngineSpeed"
   - comment_text = "This is a comment for EV EngineSpeed"
</details>
<br/>

---

## BA_DEF (INT)
```cpp
inline void tag_invoke(
	def_ba_def_int_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, int32_t min, int32_t max
) {
}
```

#### DBC example:
```BA_DEF_ BO_ "SignalCount" INT 1 100;```

<details><summary>Parsed as:</summary>

   - ba_name = "SignalCount"
   - ba_type = "BO_"
   - min = 1
   - max = 100
</details>
<br/>

---

## BA_DEF (FLOAT)
```cpp
inline void tag_invoke(
	def_ba_def_float_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, double min, double max
) {
}
```

#### DBC example:
```BA_DEF_ "Speed" FLOAT -200 200;```

<details><summary>Parsed as:</summary>

   - ba_name = "Speed"
   - ba_type = ""
     - empty ba_types are allowed
   - min = -200.0
   - max = 200.0
</details>
<br/>

---

## BA_DEF (STRING)
```cpp
inline void tag_invoke(
	def_ba_def_string_cpo, your_class& this_,
	std::string ba_name, std::string ba_type
) {
}
```

#### DBC example:
```BA_DEF_ EV_ "Date added" STRING;```

<details><summary>Parsed as:</summary>

   - ba_name = "Date added"
   - ba_type = "EV_"
</details>
<br/>

---

## BA_DEF (ENUM)
```cpp
inline void tag_invoke(
	def_ba_def_enum_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, std::vector<std::string> enum_vals
) {
}
```

#### DBC example:
```BA_DEF_ SG_ "EngineType" ENUM "Diesel", "Gasoline", "Electric";```

<details><summary>Parsed as:</summary>

   - ba_name = "EngineType"
   - ba_type = "SG_"
   - enum_vals = { "Diesel", "Gasoline", "Electric" }
</details>
<br/>

---

## BA_DEF_DEF
```cpp
inline void tag_invoke(
	def_ba_def_def_cpo, your_class& this_,
	std::string ba_name, std::variant<int32_t, double, std::string>> default_val
) {
}
```

#### DBC example:
```BA_DEF_DEF_ "EngineType" "Diesel";```

<details><summary>Parsed as:</summary>

   - ba_name = "EngineType"
   - default_val = "Diesel"
</details>
<br/>

---

## BA

```cpp
inline void tag_invoke(
	def_ba_cpo, your_class& this_,
	std::string ba_name, std::string object_type, std::string object_name,
	unsigned bu_ord, uint32_t msg_id, std::variant<int32_t, double, std::string>> ba_val
) {
}
```

#### DBC examples:
```BA_ "SignalCount" BO_ 101 10;```

<details><summary>Parsed as:</summary>

   - ba_name = "SignalCount"
   - object_type = "BO_"
   - object_name = ""
     - object_name is not specified in this example
   - bu_ord = 0
     - bu_ord is not specified in this example
   - msg_id = 101
   - ba_val = 10
</details>

---

```BA_ "EngineType" SG_ 101 "TemperatureOuter" "Gasolene";```

<details><summary>Parsed as:</summary>

   - ba_name = "EngineType"
   - object_type = "SG_"
   - object_name = "TemperatureOuter"
   - bu_ord = 0
	 - bu_ord is not specified in this example
   - msg_id = 101
   - ba_val = "Gasolene"
</details>

---

```BA_ "NodeFrequency" BU_ "MASTER" 30;```

<details><summary>Parsed as:</summary>

   - ba_name = "NodeFrequency"
   - object_type = "BU_"
   - object_name = "MASTER"
   - bu_ord = 0
     - index in the list of nodes, nodes[0] = "MASTER"
   - msg_id = 0
     - msg_id is not specified in this example
   - ba_val = 30
</details>
<br/>

---

## VAL (ENV)

```cpp
inline void tag_invoke(
	def_val_env_cpo, your_class& this_,
	std::string env_var_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}
```

#### Notes: 
This callback is only called if the keyword VAL_ is followed by an environment variable. See also: [VAL (SG)](#val-sg).

#### DBC example:
```VAL_ PowerState 0 "OFF" 1 "STANDBY" 2 "DRIVE";```

<details><summary>Parsed as:</summary>

   - env_var_name = "PowerState"
   - val_descs = { { 0, "OFF" }, { 1, "STANDBY" }, { 2, "DRIVE" } }
</details>
<br/>

---

## VAL (SG)
```cpp
inline void tag_invoke(
	def_val_sg_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}
```

#### Notes: 
This callback is only called if the keyword VAL_ is followed by a signal. See also: [VAL (ENV)](#val-env).

#### DBC example:
```VAL_ 101 PowerStateSignal 0 "OFF" 1 "STANDBY" 2 "DRIVE";```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - sg_name = "PowerStateSignal"
   - val_descs = { { 0, "OFF" }, { 1, "STANDBY" }, { 2, "DRIVE" } }
</details>
<br/>

---

## SIG_VALTYPE

```cpp
inline void tag_invoke(
	def_sig_valtype_cpo, your_class& this_,
	unsigned msg_id, std::string sg_name, unsigned sg_ext_val_type
) {
}
```

#### DBC example:
```SIG_VALTYPE_ 101 PowerStateSignal 1;```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - sg_name = "PowerStateSignal"
   - sg_ext_val_type = 1
     - 0 = integer, 1 = 32-bit IEEE float, 2 = 64-bit IEEE double
</details>
<br/>

---

## SG_MUL_VAL
```cpp
inline void tag_invoke(
	def_sg_mul_val_cpo, your_class& this_,
	unsigned msg_id, std::string mux_sg_name, std::string mux_switch_name,
	std::vector<std::pair<unsigned, unsigned>> val_ranges
) {
}
```

#### DBC example:
```SG_MUL_VAL_ 101 TemperatureOuter Mux_2 1-3, 5-7;```

<details><summary>Parsed as:</summary>

   - msg_id = 101
   - mux_sg_name = "TemperatureOuter"
   - mux_switch_name = "Mux_2"
   - val_ranges = { { 1, 3 }, { 5, 7 } }
</details>
<br/>

---


