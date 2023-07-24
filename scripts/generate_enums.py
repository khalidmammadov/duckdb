import os
import json
import re

targets = [
    {
        'source': 'extension/json/include/',
        'target': 'extension/json'
    }
]

file_list = []
for target in targets:
    source_base = os.path.sep.join(target['source'].split('/'))
    target_base = os.path.sep.join(target['target'].split('/'))
    for fname in os.listdir(source_base):
        if '_enums.json' not in fname:
            continue
        file_list.append(
            {
                'source': os.path.join(source_base, fname),
                'include_path': fname.replace('.json', '.hpp'),
                'target_hpp': os.path.join(source_base, fname.replace('.json', '.hpp')),
                'target_cpp': os.path.join(target_base, fname.replace('.json', '.cpp'))
            }
        )

header = '''//===----------------------------------------------------------------------===//
// This file is automatically generated by scripts/generate_enums.py
// Do not edit this file manually, your changes will be overwritten
//===----------------------------------------------------------------------===//

${INCLUDE_LIST}
namespace duckdb {
'''

footer = '''
} // namespace duckdb
'''

include_base = '#include "${FILENAME}"\n'

enum_header = '\nenum class ${ENUM_NAME} : ${ENUM_TYPE} {\n'

enum_footer = '};'

enum_value = '\t${ENUM_MEMBER} = ${ENUM_VALUE},\n'

enum_util_header = '''
template<>
const char* EnumUtil::ToChars<${ENUM_NAME}>(${ENUM_NAME} value);

template<>
${ENUM_NAME} EnumUtil::FromString<${ENUM_NAME}>(const char *value);
'''

enum_util_conversion_begin = '''
template<>
const char* EnumUtil::ToChars<${ENUM_NAME}>(${ENUM_NAME} value) {
	switch(value) {
'''

enum_util_switch = '\tcase ${ENUM_NAME}::${ENUM_MEMBER}:\n\t\treturn "${ENUM_MEMBER}";\n'

enum_util_conversion_end = '''	default:
		throw NotImplementedException(StringUtil::Format("Enum value of type ${ENUM_NAME}: '%d' not implemented", value));
	}
}
'''

from_string_begin = '''
template<>
${ENUM_NAME} EnumUtil::FromString<${ENUM_NAME}>(const char *value) {
'''

from_string_comparison = '''    if (StringUtil::Equals(value, "${ENUM_MEMBER}")) {
		return ${ENUM_NAME}::${ENUM_MEMBER};
	}
'''

from_string_end = '''   throw NotImplementedException(StringUtil::Format("Enum value of type ${ENUM_NAME}: '%s' not implemented", value));
}
'''

class EnumMember:
    def __init__(self, entry, index):
        self.comment = None
        self.index = index
        if type(entry) == str:
            self.name = entry
        else:
            self.name = entry['name']
            if 'comment' in entry:
                self.comment = entry['comment']
            if 'index' in entry:
                self.index = int(entry['index'])

class EnumClass:
    def __init__(self, entry):
        self.name = entry['name']
        self.type = 'uint8_t'
        self.values = []
        index = 0
        for value_entry in entry['values']:
            self.values.append(EnumMember(value_entry, index))
            index += 1

for entry in file_list:
    source_path = entry['source']
    target_header = entry['target_hpp']
    target_source = entry['target_cpp']
    include_path = entry['include_path']
    with open(source_path, 'r') as f:
        json_data = json.load(f)

    include_list = ['duckdb/common/constants.hpp', 'duckdb/common/enum_util.hpp']
    enums = []

    for entry in json_data:
        if 'includes' in entry:
            include_list += entry['includes']
        enums.append(EnumClass(entry))

    with open(target_header, 'w+') as f:
        include_text = '#pragma once\n\n'
        include_text += ''.join([include_base.replace('${FILENAME}', x) for x in include_list])
        f.write(header.replace('${INCLUDE_LIST}', include_text))

        for enum in enums:
            f.write(enum_header.replace('${ENUM_NAME}', enum.name).replace('${ENUM_TYPE}', enum.type))
            for value in enum.values:
                if value.comment is not None:
                    f.write('\t//! ' + value.comment + '\n')
                f.write(enum_value.replace('${ENUM_MEMBER}', value.name).replace('${ENUM_VALUE}', str(value.index)))

            f.write(enum_footer)
            f.write('\n')

        for enum in enums:
            f.write(enum_util_header.replace('${ENUM_NAME}', enum.name))

        f.write(footer)

    with open(target_source, 'w+') as f:
        source_include_list = [include_path, 'duckdb/common/string_util.hpp']
        f.write(header.replace('${INCLUDE_LIST}', ''.join([include_base.replace('${FILENAME}', x) for x in source_include_list])))

        for enum in enums:
            f.write(enum_util_conversion_begin.replace('${ENUM_NAME}', enum.name))
            for value in enum.values:
                f.write(enum_util_switch.replace('${ENUM_MEMBER}', value.name).replace('${ENUM_NAME}', enum.name))

            f.write(enum_util_conversion_end.replace('${ENUM_NAME}', enum.name))
            f.write(from_string_begin.replace('${ENUM_NAME}', enum.name))
            for value in enum.values:
                f.write(from_string_comparison.replace('${ENUM_MEMBER}', value.name).replace('${ENUM_NAME}', enum.name))

            f.write(from_string_end.replace('${ENUM_NAME}', enum.name))
        f.write(footer)
