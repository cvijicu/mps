#!/usr/bin/env python3
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys

parser = argparse.ArgumentParser(description='Build and collect experiments for Labs 1–3.')
parser.add_argument('lab', choices=['1', '2', '3'])
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--max-workers', type=int, default=4)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
if args.max_workers < 1 or args.max_workers > 128:
    parser.error('--max-workers must be in [1, 128]')
available = sorted(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else list(range(os.cpu_count() or 1))
limit = min(args.max_workers, len(available))
counts = [p for p in [1, 2, 4] if p <= limit]
target = {'1': 'moving_average', '2': 'thread_average', '3': 'omp_average'}[args.lab]
if args.out.exists():
    parser.error('Output directory exists; choose a new --out to preserve earlier runs.')
args.out.mkdir(parents=True)
compiler = shlex.split(os.environ.get('CXX', 'g++'))
flags = shlex.split(os.environ.get('CXXFLAGS', '-O3 -Wall -Wextra'))
standard = shlex.split(os.environ.get('CXXSTD', ''))
sources = ['src/moving_average.cpp', 'src/average/sequential.cpp']
if args.lab == '2':
    flags += ['-pthread']
    sources = ['src/average/thread_experiment.cpp', 'src/average/sequential.cpp', 'src/average/threads.cpp']
elif args.lab == '3':
    flags += ['-fopenmp']
    sources = ['src/average/openmp_experiment.cpp', 'src/average/sequential.cpp', 'src/average/openmp.cpp']
build_command = compiler + flags + standard + sources + ['-o', 'src/' + target]
build = subprocess.run(build_command, cwd=root, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
(args.out / 'build.txt').write_text(shlex.join(build_command) + '\n' + build.stdout)
if build.returncode:
    sys.exit('Build failed; see build.txt')
env = dict(os.environ, OMP_DYNAMIC='FALSE')
metadata = {
    'platform': platform.platform(), 'available_cpus': available,
    'worker_counts': counts, 'python': platform.python_version(),
    'environment': {key: value for key, value in env.items()
                    if key.startswith('OMP_') or key in ('CXX', 'CXXFLAGS', 'CXXSTD')},
    'build_command': build_command,
    'commands': [],
    'note': 'Record VM/container status and competing load separately.'
}
if Path('/proc/cpuinfo').exists():
    metadata['cpuinfo'] = Path('/proc/cpuinfo').read_text()

def save_metadata():
    (args.out / 'machine.json').write_text(json.dumps(metadata, indent=2) + '\n')

save_metadata()
checked = subprocess.run([str(root / 'src' / target), '--check'], env=env,
                         capture_output=True, text=True)
(args.out / 'checks.txt').write_text(checked.stdout)
(args.out / 'checks.err').write_text(checked.stderr)
if checked.returncode:
    sys.exit('Checks failed; complete the student kernels first. See checks.err')
if args.lab == '1':
    commands = [[str(n), str(w)] for n in [100000, 1000000, 5000000] for w in [8, 64]]
elif args.lab == '2':
    commands = [['1000000', '64', str(p)] for p in counts]
else:
    commands = [['regular', '1000000', '64', str(p), 'static'] for p in counts]
    commands.append(['irregular', '100000', '256', str(counts[-1]), 'all'])
for i, arguments in enumerate(commands, 1):
    stem = args.out / f'run_{i:02d}'
    csv_path = stem.with_suffix('.csv').resolve()
    command = [str(root / 'src' / target), *arguments, str(csv_path)]
    metadata['commands'].append(command)
    save_metadata()
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    stem.with_suffix('.txt').write_text(result.stdout)
    stem.with_suffix('.err').write_text(result.stderr)
    if result.returncode:
        sys.exit(f'Experiment failed: {command}; see {stem}.err')
    if not csv_path.is_file():
        sys.exit(f'Missing measurement file: {csv_path}')
    print(f'Saved {stem}.csv')
