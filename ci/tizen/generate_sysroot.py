#!/usr/bin/env python3
# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import shutil
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path

base_packages = [
    'gcc',
    'glibc',
    'glibc-devel',
    'libgcc',
    'linux-glibc-devel',
    'zlib-devel',
]

unified_packages = [
    'fontconfig',
    'fontconfig-devel',
    'freetype2-devel',
    'libpng-devel',
]


def generate_sysroot(sysroot: Path, api_version: float, arch: str, quiet=False):
  if arch == 'arm':
    tizen_arch = 'armv7l'
  elif arch == 'arm64':
    tizen_arch = 'aarch64'
  elif arch == 'x86':
    tizen_arch = 'i686'
  else:
    sys.exit('Unknown arch: ' + arch)

  base_repo = 'http://download.tizen.org/snapshots/TIZEN/Tizen-{}/Tizen-{}-Base/latest/repos/standard/packages'.format(
      api_version, api_version
  )
  unified_repo = 'http://download.tizen.org/snapshots/TIZEN/Tizen-{}/Tizen-{}-Unified/latest/repos/standard/packages'.format(
      api_version, api_version
  )

  # Retrieve html documents.
  documents = {}
  for url in ['{}/{}'.format(base_repo, tizen_arch),
              '{}/{}'.format(base_repo, 'noarch'), '{}/{}'.format(unified_repo,
                                                                  tizen_arch),
              '{}/{}'.format(unified_repo, 'noarch')]:
    request = urllib.request.Request(url)
    with urllib.request.urlopen(request) as response:
      documents[url] = response.read().decode('utf-8')

  # Download packages.
  download_path = sysroot / '.rpms'
  download_path.mkdir(exist_ok=True)
  existing_rpms = [f for f in download_path.iterdir() if f.suffix == '.rpm']

  for package in base_packages + unified_packages:
    quoted = urllib.parse.quote(package)
    pattern = re.escape(quoted) + '-\\d+\\.[\\d_\\.]+-[\\d\\.]+\\..+\\.rpm'

    if any([re.match(pattern, rpm.name) for rpm in existing_rpms]):
      continue

    for parent, doc in documents.items():
      match = re.search('<a href="({})">.+?</a>'.format(pattern), doc)
      if match:
        rpm_url = '{}/{}'.format(parent, match.group(1))
        break

    if match:
      if not quiet:
        print('Downloading ' + rpm_url)
      urllib.request.urlretrieve(rpm_url, download_path / match.group(1))
    else:
      sys.exit('Could not find a package named ' + package)

  # Extract files.
  for rpm in [f for f in download_path.iterdir() if f.suffix == '.rpm']:
    command = 'rpm2cpio {} | cpio -idum --quiet'.format(rpm)
    subprocess.run(command, shell=True, cwd=sysroot, check=True)

  # Create symbolic links.
  asm = sysroot / 'usr' / 'include' / 'asm'
  if not asm.exists():
    os.symlink('asm-' + arch, asm)
  pkgconfig = sysroot / 'usr' / 'lib' / 'pkgconfig'
  if arch == 'arm64' and not pkgconfig.exists():
    os.symlink('../lib64/pkgconfig', pkgconfig)

  # Copy objects required by the linker, such as crtbeginS.o and libgcc.a.
  if arch == 'arm64':
    libpath = sysroot / 'usr' / 'lib64'
  else:
    libpath = sysroot / 'usr' / 'lib'
  subprocess.run(
      'cp gcc/*/*/*.o gcc/*/*/*.a .', shell=True, cwd=libpath, check=True
  )

  # Apply a patch if applicable.
  patch = Path(__file__).parent / '{}.patch'.format(arch)
  if patch.is_file():
    command = 'patch -p1 -s -d {} < {}'.format(sysroot, patch)
    subprocess.run(command, shell=True, check=True)


def main():
  # Check dependencies.
  for dep in ['rpm2cpio', 'cpio', 'git']:
    if not shutil.which(dep):
      sys.exit(
          '{} is not installed. To install, run:\n'
          '  sudo apt install {}'.format(dep, dep)
      )

  # Parse arguments.
  parser = argparse.ArgumentParser(description='Tizen sysroot generator')
  parser.add_argument(
      '-o',
      '--out',
      metavar='PATH',
      type=str,
      help='Path to the output directory'
  )
  parser.add_argument(
      '-f',
      '--force',
      action='store_true',
      help='Force re-downloading of packages'
  )
  parser.add_argument(
      '-q', '--quiet', action='store_true', help='Suppress log output'
  )
  parser.add_argument(
      '--api-version',
      metavar='VER',
      default=5.5,
      type=float,
      help='Target API version (defaults to 5.5)'
  )
  args = parser.parse_args()

  if args.out:
    outpath = Path(args.out)
  else:
    outpath = Path(__file__).parent / 'sysroot'
  outpath.mkdir(exist_ok=True)

  for arch in ['arm', 'arm64', 'x86']:
    sysroot = outpath / arch
    if args.force and sysroot.is_dir():
      shutil.rmtree(sysroot)
    sysroot.mkdir(exist_ok=True)

    if not args.quiet:
      print('Generating sysroot for {}...'.format(arch))
    generate_sysroot(sysroot.resolve(), args.api_version, arch, args.quiet)


# Execute only if run as a script.
if __name__ == '__main__':
  main()
