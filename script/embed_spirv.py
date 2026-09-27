"""Embed glslang's SPIR-V as aligned uint32 arrays (no runtime shader files)."""
import pathlib
import struct
import sys

output = pathlib.Path(sys.argv[1])
lines = ['#pragma once', '#include <cstdint>', '#include <cstddef>', '#include <string>', '#include <utility>', '#include <stdexcept>', 'namespace VulkanShaders {']
entries = []
for filename in sys.argv[2:]:
    path = pathlib.Path(filename)
    name = path.name[:-4]
    symbol = name.replace('.', '_')
    data = path.read_bytes()
    words = struct.unpack('<' + 'I' * (len(data) // 4), data)
    lines.append('inline constexpr uint32_t ' + symbol + '[] = {')
    for start in range(0, len(words), 8):
        lines.append(','.join(hex(w) for w in words[start:start + 8]) + ',')
    lines.append('};')
    entries.append(f'if(name=="{name}") return {{{symbol},sizeof({symbol})}};')
lines += ['inline std::pair<const uint32_t*,size_t> get(const std::string& name) {', *entries,
          'throw std::runtime_error("Unknown Vulkan shader: "+name);', '}', '}']
output.write_text('\n'.join(lines) + '\n')
