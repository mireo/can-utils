#pragma once

#include "dbc/dbc_parser.h"

class your_class {
	// The parser invokes callbacks that have access to an instance of this class.
	// It can implement custom logic and use only the parts of the .dbc file that are needed.
};

namespace can {

inline void tag_invoke(
	def_version_cpo, your_class& this_,
	std::string version
) {
}

inline void tag_invoke(
	def_bu_cpo, your_class& this_,
	std::vector<std::string> nodes
) {
}

inline void tag_invoke(
	def_bo_cpo, your_class& this_,
	uint32_t msg_id, std::string msg_name, size_t msg_size, size_t transmitter_ord
) {
}

inline void tag_invoke(
	def_sg_cpo, your_class& this_,
	uint32_t msg_id, std::optional<unsigned> sg_mux_switch_val, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	double sg_factor, double sg_offset, double sg_min, double sg_max,
	std::string sg_unit, std::vector<size_t> receiver_ords
) {
}

inline void tag_invoke(
	def_sg_mux_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	std::string sg_unit, std::vector<size_t> receiver_ords
) {
}

inline void tag_invoke(
	def_ev_cpo, your_class& this_,
	std::string name, unsigned type, double ev_min, double ev_max,
	std::string unit, double initial, unsigned ev_id,
	std::string access_type, std::vector<size_t> access_nodes_ords
) {
}

inline void tag_invoke(
	def_envvar_data_cpo, your_class& this_,
	std::string ev_name, unsigned data_size
) {
}

inline void tag_invoke(
	def_sgtype_cpo, your_class& this_,
	unsigned msg_id, std::string sg_name, std::string sg_type_name
) {
}

inline void tag_invoke(
	def_sgtype_ref_cpo, your_class& this_,
	std::string sg_type_name, unsigned sg_size, char sg_byte_order, char sg_sign,
	double sg_factor, double sg_offset, double sg_min, double sg_max,
	std::string sg_unit, double sg_default_val, size_t val_table_ord
) {
}

inline void tag_invoke(
	def_sig_group_cpo, your_class& this_,
	uint32_t msg_id, std::string sig_group_name, unsigned repetitions, std::vector<std::string> sig_names
) {
}

inline void tag_invoke(
	def_cm_glob_cpo, your_class& this_,
	std::string comment_text
) {
}

inline void tag_invoke(
	def_cm_bu_cpo, your_class& this_,
	unsigned bu_ord, std::string comment_text
) {
}

inline void tag_invoke(
	def_cm_bo_cpo, your_class& this_,
	uint32_t msg_id, std::string comment_text
) {
}

inline void tag_invoke(
	def_cm_sg_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name, std::string comment_text
) {
}

inline void tag_invoke(
	def_cm_ev_cpo, your_class& this_,
	std::string ev_name, std::string comment_text
) {
}

inline void tag_invoke(
	def_ba_def_enum_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, std::vector<std::string> enum_vals
) {
}

inline void tag_invoke(
	def_ba_def_int_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, int32_t min, int32_t max
) {
}

inline void tag_invoke(
	def_ba_def_float_cpo, your_class& this_,
	std::string ba_name, std::string ba_type, double min, double max
) {
}

inline void tag_invoke(
	def_ba_def_string_cpo, your_class& this_,
	std::string ba_name, std::string ba_type
) {
}

inline void tag_invoke(
	def_ba_def_def_cpo, your_class& this_,
	std::string ba_name, std::variant<int32_t, double, std::string> default_val
) {
}

inline void tag_invoke(
	def_ba_cpo, your_class& this_,
	std::string ba_name, std::string object_type, std::string object_name,
	unsigned bu_ord, uint32_t msg_id, std::variant<int32_t, double, std::string> ba_val
) {
}

inline void tag_invoke(
	def_val_env_cpo, your_class& this_,
	std::string env_var_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}

inline void tag_invoke(
	def_val_sg_cpo, your_class& this_,
	uint32_t msg_id, std::string sg_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}

inline void tag_invoke(
	def_val_table_cpo, your_class& this_,
	std::string table_name, std::vector<std::pair<unsigned, std::string>> val_descs
) {
}

inline void tag_invoke(
	def_sig_valtype_cpo, your_class& this_,
	unsigned msg_id, std::string sg_name, unsigned sg_ext_val_type
) {
}

inline void tag_invoke(
	def_bo_tx_bu_cpo, your_class& this_,
	unsigned msg_id, std::vector<std::string> transmitters
) {
}

inline void tag_invoke(
	def_sg_mul_val_cpo, your_class& this_,
	unsigned msg_id, std::string mux_sg_name, std::string mux_switch_name,
	std::vector<std::pair<unsigned, unsigned>> val_ranges
) {
}

}; // end namespace can
