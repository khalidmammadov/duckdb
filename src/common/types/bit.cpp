#include "duckdb/common/operator/cast_operators.hpp"
#include "duckdb/common/types/bit.hpp"

namespace duckdb {

idx_t Bit::BitLength(string_t bits) {
	return ((bits.GetSize() - 1) * 8) - GetPadding(bits);
}

idx_t Bit::OctetLength(string_t bits) {
	return bits.GetSize() - 1;
}

idx_t Bit::BitCount(string_t bits) {
	idx_t count = 0;
	const char *buf = bits.GetDataUnsafe();
	for (idx_t byte_idx = 1; byte_idx < OctetLength(bits) + 1; byte_idx++) {
		for (idx_t bit_idx = 0; bit_idx < 8; bit_idx++) {
			count +=
			    (buf[byte_idx] & (1 << bit_idx)) ? 1 : 0; // buf holds the byte index and j holds the index of the bit
		}
	}
	return count;
}

idx_t Bit::BitPosition(string_t substring, string_t bits) {
	const char *buf = bits.GetDataUnsafe();
	auto len = bits.GetSize();
	auto substr_len = BitLength(substring);
	idx_t substr_idx = 0;

	for (idx_t bit_idx = GetPadding(bits); bit_idx < 8; bit_idx++) {
		idx_t bit = buf[1] & (1 << (7 - bit_idx)) ? 1 : 0;
		if (bit == GetBit(substring, substr_idx)) {
			substr_idx++;
			if (substr_idx == substr_len) {
				return (bit_idx - GetPadding(bits)) - substr_len + 2;
			}
		} else {
			substr_idx = 0;
		}
	}

	for (idx_t byte_idx = 2; byte_idx < len; byte_idx++) {
		for (idx_t bit_idx = 0; bit_idx < 8; bit_idx++) {
			idx_t bit = buf[byte_idx] & (1 << (7 - bit_idx)) ? 1 : 0;
			if (bit == GetBit(substring, substr_idx)) {
				substr_idx++;
				if (substr_idx == substr_len) {
					return (((byte_idx - 1) * 8) + bit_idx - GetPadding(bits)) - substr_len + 2;
				}
			} else {
				substr_idx = 0;
			}
		}
	}
	return 0;
}

void Bit::ToString(string_t bits, char *output) {
	auto data = (const_data_ptr_t)bits.GetDataUnsafe();
	auto len = bits.GetSize();

	idx_t padding = GetPadding(bits);
	idx_t output_idx = 0;
	for (idx_t bit_idx = padding; bit_idx < 8; bit_idx++) {
		output[output_idx++] = data[1] & (1 << (7 - bit_idx)) ? '1' : '0';
	}
	for (idx_t byte_idx = 2; byte_idx < len; byte_idx++) {
		for (idx_t bit_idx = 0; bit_idx < 8; bit_idx++) {
			output[output_idx++] = data[byte_idx] & (1 << (7 - bit_idx)) ? '1' : '0';
		}
	}
}

bool Bit::TryGetBitStringSize(string_t str, idx_t &str_len, string *error_message) {
	auto data = (const_data_ptr_t)str.GetDataUnsafe();
	auto len = str.GetSize();
	str_len = 0;
	for (idx_t i = 0; i < len; i++) {
		if (data[i] == '0' || data[i] == '1') {
			str_len++;
		} else {
			string error = StringUtil::Format("Invalid character encountered in string -> bit conversion: '%s'",
			                                  string((char *)data + i, 1));
			HandleCastError::AssignError(error, error_message);
			return false;
		}
	}
	str_len = str_len % 8 ? (str_len / 8) + 1 : str_len / 8;
	str_len++; // additional first byte to store info on zero padding
	return true;
}

void Bit::ToBit(string_t str, data_ptr_t output) {
	auto data = (const_data_ptr_t)str.GetDataUnsafe();
	auto len = str.GetSize();

	char byte = 0;
	idx_t padded_byte = len % 8;
	for (idx_t i = 0; i < padded_byte; i++) {
		byte <<= 1;
		if (data[i] == '1') {
			byte |= 1;
		}
	}
	if (padded_byte != 0) {
		*(output++) = (8 - padded_byte); // the first byte contains the number of padded zeroes
	}
	*(output++) = byte;

	for (idx_t byte_idx = padded_byte; byte_idx < len; byte_idx += 8) {
		byte = 0;
		for (idx_t bit_idx = 0; bit_idx < 8; bit_idx++) {
			byte <<= 1;
			if (data[byte_idx + bit_idx] == '1') {
				byte |= 1;
			}
		}
		*(output++) = byte;
	}
}

idx_t Bit::GetBit(string_t bit_string, idx_t n) {
	const char *buf = bit_string.GetDataUnsafe();
	n += GetPadding(bit_string);

	char byte = buf[(n / 8) + 1] >> (7 - (n % 8));
	return (byte & 1 ? 1 : 0);
}

void Bit::SetBit(string_t &bit_string, idx_t n, idx_t new_value) {
	char *buf = bit_string.GetDataWriteable();
	n += GetPadding(bit_string);

	char shift_byte = 1 << (7 - (n % 8));
	if (new_value == 0) {
		shift_byte = ~shift_byte;
		buf[(n / 8) + 1] &= shift_byte;
	} else {
		buf[(n / 8) + 1] |= shift_byte;
	}
}

inline idx_t Bit::GetPadding(string_t &bit_string) {
	auto data = (const_data_ptr_t)bit_string.GetDataUnsafe();
	return data[0];
}
} // namespace duckdb
