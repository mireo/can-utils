#pragma once

#include <optional>
#include <string_view>
#include <string>
#include <vector>
#include <variant>
#include <utility>

#include "any/any.h"

namespace can {

template <size_t N>
struct method_name {
	constexpr method_name(const char(&d)[N]) {}
};

template <method_name method, typename ...Args>
struct cpo {
	using type_erased_signature_t = void (mireo::this_&, Args...);

	template <typename T>
	requires mireo::tag_invocable<cpo, T&, Args...>
	void operator()(T& x, Args ...args) const {
		return mireo::tag_invoke(*this, x, std::forward<Args>(args)...);
	}

	template <typename T>
	friend void tag_invoke(cpo<method, Args...>, std::reference_wrapper<T>& t, Args... args) {
		mireo::tag_invoke(cpo<method, Args...>{}, t.get(), std::forward<Args>(args)...);
	}

	template <typename T>
	friend void tag_invoke(cpo<method, Args...>, T&, Args...) {}
};

using def_version_cpo = cpo<"version", std::string>;
inline constexpr def_version_cpo def_version;

using def_bu_cpo = cpo<"bu", std::vector<std::string>>;
inline constexpr def_bu_cpo def_bu;

using def_bo_cpo = cpo<"bo", uint32_t, std::string, size_t, size_t>;
inline constexpr def_bo_cpo def_bo;

using def_sg_cpo = cpo<
	"sg", uint32_t, std::optional<unsigned>, std::string,
	unsigned, unsigned, char, char, double, double, double, double,
	std::string, std::vector<size_t>
>;
inline constexpr def_sg_cpo def_sg;

using def_sg_mux_cpo = cpo<
	"sg_mux", uint32_t, std::string,
	unsigned, unsigned, char, char,
	std::string, std::vector<size_t>
>;
inline constexpr def_sg_mux_cpo def_sg_mux;

using def_ev_cpo = cpo<
	"ev", std::string, unsigned, double, double,
	std::string, double, unsigned, std::string, std::vector<size_t>
>;
inline constexpr def_ev_cpo def_ev;

using def_envvar_data_cpo = cpo<"envvar_data", std::string, unsigned>;
inline constexpr def_envvar_data_cpo def_envvar_data;

using def_sgtype_ref_cpo = cpo<"sgtype_ref", unsigned, std::string, std::string>;
inline constexpr def_sgtype_ref_cpo def_sgtype_ref;

using def_sgtype_cpo = cpo<
	"sgtype", std::string, unsigned, char, char,
	double, double, double, double, std::string, double, size_t
>;
inline constexpr def_sgtype_cpo def_sgtype;

using def_sig_group_cpo = cpo<"sig_group", unsigned, std::string, unsigned, std::vector<std::string>>;
inline constexpr def_sig_group_cpo def_sig_group;

using def_cm_glob_cpo = cpo<"cm", std::string>;
inline constexpr def_cm_glob_cpo def_cm_glob;

using def_cm_bu_cpo = cpo<"cm_msg", unsigned, std::string>;
inline constexpr def_cm_bu_cpo def_cm_bu;

using def_cm_bo_cpo = cpo<"cm_bo", uint32_t, std::string>;
inline constexpr def_cm_bo_cpo def_cm_bo;

using def_cm_sg_cpo = cpo<"cm_sg", uint32_t, std::string, std::string>;
inline constexpr def_cm_sg_cpo def_cm_sg;

using def_cm_ev_cpo = cpo<"cm_ev", std::string, std::string>;
inline constexpr def_cm_ev_cpo def_cm_ev;

using def_ba_def_enum_cpo = cpo<"ba_def_enum", std::string, std::string, std::vector<std::string>>;
inline constexpr def_ba_def_enum_cpo def_ba_def_enum;

using def_ba_def_int_cpo = cpo<"ba_def_int", std::string, std::string, int32_t, int32_t>;
inline constexpr def_ba_def_int_cpo def_ba_def_int;

using def_ba_def_float_cpo = cpo<"ba_def_float", std::string, std::string, double, double>;
inline constexpr def_ba_def_float_cpo def_ba_def_float;

using def_ba_def_string_cpo = cpo<"ba_def_string", std::string, std::string>;
inline constexpr def_ba_def_string_cpo def_ba_def_string;

using def_ba_def_def_cpo = cpo<"ba_def_def", std::string, std::variant<int32_t, double, std::string>>;
inline constexpr def_ba_def_def_cpo def_ba_def_def;

using def_ba_cpo = cpo<
	"ba", std::string, std::string, std::string, size_t, unsigned,
	std::variant<int32_t, double, std::string>
>;
inline constexpr def_ba_cpo def_ba;

using def_val_env_cpo = cpo<
	"val_env", std::string,
	std::vector<std::pair<unsigned, std::string>>
>;
inline constexpr def_val_env_cpo def_val_env;

using def_val_sg_cpo = cpo<
	"val_sg", uint32_t, std::string,
	std::vector<std::pair<unsigned, std::string>>
>;
inline constexpr def_val_sg_cpo def_val_sg;

using def_val_table_cpo = cpo<"val_table", std::string, std::vector<std::pair<unsigned, std::string>>>;
inline constexpr def_val_table_cpo def_val_table;

using def_sig_valtype_cpo = cpo<"sig_valtype", unsigned, std::string, unsigned>;
inline constexpr def_sig_valtype_cpo def_sig_valtype;

using def_bo_tx_bu_cpo = cpo<"bo_tx_bu", unsigned, std::vector<std::string>>;
inline constexpr def_bo_tx_bu_cpo def_bo_tx_bu;

using def_sg_mul_val_cpo = cpo<"sg_mul_val", unsigned, std::string, std::string, std::vector<std::pair<unsigned, unsigned>>>;
inline constexpr def_sg_mul_val_cpo def_sg_mul_val;

using interpreter = mireo::any<
	def_version,
	def_bu,
	def_bo,
	def_ev,
	def_envvar_data,
	def_sgtype,
	def_sgtype_ref,
	def_sig_group,
	def_cm_glob,
	def_cm_bu,
	def_cm_bo,
	def_cm_sg,
	def_cm_ev,
	def_ba_def_enum,
	def_ba_def_int,
	def_ba_def_float,
	def_ba_def_string,
	def_ba_def_def,
	def_ba,
	def_sg,
	def_sg_mux,
	def_val_env,
	def_val_sg,
	def_val_table,
	def_sig_valtype,
	def_bo_tx_bu,
	def_sg_mul_val
>;

bool parse_dbc(std::string_view dbc_src, interpreter ipt);

} // end namespace can
