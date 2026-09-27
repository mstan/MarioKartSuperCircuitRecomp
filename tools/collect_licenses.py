"""Stage dependency notices next to a desktop release (no private build data)."""
import argparse
from pathlib import Path
import shutil

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('engine', type=Path)
p.add_argument('ui', type=Path)
p.add_argument('output', type=Path)
p.add_argument('--mingw', type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
sources = {
    'gbarecomp': a.engine / 'LICENSE',
    'arm-recomp-core': a.engine / 'external/arm-recomp-core/LICENSE',
    'recomp-net': a.engine / 'external/recomp-net/LICENSE',
    'rbengine': a.engine / 'external/rbengine/LICENSE',
    'recomp-ui': a.ui / 'LICENSE',
    'Dear-ImGui': a.ui / 'src/third_party/imgui/LICENSE.txt',
}
for name, source in sources.items():
    shutil.copyfile(source, a.output / f'{name}.txt')
if a.mingw:
    for package in ('gcc-libs', 'libwinpthread', 'SDL2'):
        shutil.copytree(a.mingw / 'share/licenses' / package,
                        a.output / package, dirs_exist_ok=True)
else:
    shutil.copyfile(a.engine / 'platform/android/third_party/SDL/LICENSE.txt',
                    a.output / 'SDL2.txt')
    for package in ('libfreetype6', 'libharfbuzz0b', 'libpng16-16',
                    'libbrotli1', 'libgraphite2-3'):
        shutil.copyfile(Path('/usr/share/doc') / package / 'copyright',
                        a.output / f'{package}.txt')
