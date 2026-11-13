"""Stage dependency notices next to a desktop release (no private build data)."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request

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

# The shipped overlay compiler is the fixed TinyCC 0.9.27 binary. Include its
# matching license and complete upstream source alongside it.
tcc_url = 'https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27.tar.bz2'
tcc_sha256 = 'de23af78fca90ce32dff2dd45b3432b2334740bb9bb7b05bf60fdbfc396ceb9c'
tcc_cache = a.engine / 'tools/_toolchain_cache/tcc-0.9.27.tar.bz2'
if not tcc_cache.exists():
    tcc_cache.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(tcc_url, timeout=60) as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != tcc_sha256:
        raise RuntimeError('TinyCC source archive checksum differs')
    tcc_cache.write_bytes(data)
if hashlib.sha256(tcc_cache.read_bytes()).hexdigest() != tcc_sha256:
    raise RuntimeError('Cached TinyCC source archive checksum differs')
tcc_output = a.output / 'TinyCC'
tcc_output.mkdir(exist_ok=True)
shutil.copyfile(tcc_cache, tcc_output / tcc_cache.name)
with tarfile.open(tcc_cache) as archive:
    (tcc_output / 'COPYING').write_bytes(archive.extractfile('tcc-0.9.27/COPYING').read())

git = 'C:/Program Files/Git/cmd/git.exe' if Path('C:/Program Files/Git/cmd/git.exe').exists() else 'git'
title = Path(__file__).resolve().parents[1]
source_lines = ['# Source identities', '',
                'The title and pinned dependencies include their build scripts. Supply your own ROM/BIOS to regenerate the game corpus.', '']
for label, root, url in (
    ('Title', title, None),
    ('Engine', a.engine, 'https://github.com/mstan/gbarecomp'),
    ('Launcher', a.ui, 'https://github.com/RetroPortingToolKit/recomp-ui'),
):
    head = subprocess.check_output([git, '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    if url is None:
        for remote in ('origin', 'mstan'):
            candidate = subprocess.run([git, '-C', str(root), 'config', '--get', f'remote.{remote}.url'], text=True, capture_output=True)
            if candidate.returncode == 0:
                url = candidate.stdout.strip().removesuffix('.git').replace('git@github.com:', 'https://github.com/')
                break
        if url is None:
            raise RuntimeError('Title source remote is missing')
    source_lines.append(f'- {label}: {url}/tree/{head}')
source_lines += ['', 'Additional engine dependencies are pinned by the engine source tree above.',
                 f'TinyCC 0.9.27 source is included in TinyCC/{tcc_cache.name}; upstream: {tcc_url}',
                 f'TinyCC source SHA256: {tcc_sha256}', '']
(a.output / 'SOURCES.md').write_text('\n'.join(source_lines), encoding='utf-8')
