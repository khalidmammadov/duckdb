//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/planner/expression/bound_parameter_data.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/types/value.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/field_writer.hpp"

namespace duckdb {

struct BoundParameterData {
	BoundParameterData() {
	}
	BoundParameterData(Value val) : value(move(val)), return_type(value.type()) {
	}

	Value value;
	LogicalType return_type;

public:
	void Serialize(Serializer &serializer) const {
		FieldWriter writer(serializer);
		value.Serialize(writer.GetSerializer());
		writer.WriteSerializable(return_type);
		writer.Finalize();
	}

	static unique_ptr<BoundParameterData> Deserialize(Deserializer &source) {
		FieldReader reader(source);
		auto value = Value::Deserialize(reader.GetSource());
		auto result = make_unique<BoundParameterData>(value);
		result->return_type = reader.ReadRequiredSerializable<LogicalType, LogicalType>();
		reader.Finalize();
		return result;
	}
};

using bound_parameter_map_t = unordered_map<idx_t, shared_ptr<BoundParameterData>>;

struct BoundParameterMap {
	BoundParameterMap(vector<BoundParameterData> &parameter_data) : parameter_data(parameter_data) {
	}

	bound_parameter_map_t parameters;
	vector<BoundParameterData> &parameter_data;

	LogicalType GetReturnType(idx_t index) {
		if (index >= parameter_data.size()) {
			return LogicalTypeId::UNKNOWN;
		}
		return parameter_data[index].return_type;
	}
};

} // namespace duckdb
