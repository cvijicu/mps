#!/usr/bin/env python3
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys

parser = argparse.ArgumentParser(description='Build and collect experiments for Labs 1–4.')
parser.add_argument('lab', choices=['1', '2', '3', '4'])
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--max-workers', type=int, default=4)
parser.add_argument('--matrix-sizes', type=int, nargs='+', default=[128, 512, 2048],
                    help='Lab 4 matrix dimensions (default: 128 512 2048)')
parser.add_argument('--updates', type=int, default=4000000,
                    help='Lab 4 fixed total atomic updates (default: 4000000)')
parser.add_argument('--line-bytes', type=int, help='Lab 4 verified coherence-line size')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
if args.max_workers < 1 or args.max_workers > 128:
    parser.error('--max-workers must be in [1, 128]')
available = sorted(os.sched_getaffinity(0)) if hasattr(os, 'sched_getaffinity') else list(range(os.cpu_count() or 1))
limit = min(args.max_workers, len(available))
counts = [p for p in [1, 2, 4] if p <= limit]
cache_entries = []
for entry in sorted(Path(f'/sys/devices/system/cpu/cpu{available[0]}/cache').glob('index*')):
    cache_entries.append({name: (entry / name).read_text().strip()
                          for name in ['level', 'type', 'size', 'coherency_line_size', 'shared_cpu_list']
                          if (entry / name).is_file()})
line_bytes = args.line_bytes
line_source = 'command line'
if line_bytes is None:
    detected = next((c['coherency_line_size'] for c in cache_entries
                     if c.get('level') == '1' and c.get('type') in ('Data', 'Unified')), None)
    line_bytes = int(detected) if detected else 64
    line_source = 'sysfs L1 data cache' if detected else '64 B assumption; verify on the working computer'
if args.lab == '4':
    if not all(1 <= n <= 8192 for n in args.matrix_sizes) or len(set(args.matrix_sizes)) != len(args.matrix_sizes):
        parser.error('--matrix-sizes must be distinct integers in [1, 8192]')
    if not 1 <= args.updates <= 100000000 or any(args.updates % p for p in counts):
        parser.error('--updates must be in [1, 100000000] and divisible by each selected worker count')
    if not 16 <= line_bytes <= 4096 or line_bytes & (line_bytes - 1):
        parser.error('--line-bytes must be a power of two in [16, 4096]')
if args.out.exists():
    parser.error('Output directory exists; choose a new --out to preserve earlier runs.')
args.out.mkdir(parents=True)
compiler = shlex.split(os.environ.get('CXX', 'g++'))
flags = shlex.split(os.environ.get('CXXFLAGS', '-O3 -Wall -Wextra'))
standard = shlex.split(os.environ.get('CXXSTD', ''))
target = {'1': 'moving_average', '2': 'thread_average', '3': 'omp_average', '4': 'locality'}[args.lab]
sources = ['src/moving_average.cpp', 'src/average/sequential.cpp']
if args.lab == '2':
    flags += ['-pthread']
    sources = ['src/average/thread_experiment.cpp', 'src/average/sequential.cpp', 'src/average/threads.cpp']
elif args.lab == '3':
    flags += ['-fopenmp']
    sources = ['src/average/openmp_experiment.cpp', 'src/average/sequential.cpp', 'src/average/openmp.cpp']
elif args.lab == '4':
    sources = ['src/arch/locality.cpp', 'src/arch/cache_kernels.cpp']
build_command = compiler + flags + standard + sources + ['-o', 'src/' + target]
build_commands = [build_command]
if args.lab == '4':
    build_commands.append(compiler + flags + standard + ['-pthread', f'-DPML_LINE_BYTES={line_bytes}',
                          'src/arch/false_sharing.cpp', '-o', 'src/false_sharing'])
with (args.out / 'build.txt').open('w') as log:
    for command in build_commands:
        build = subprocess.run(command, cwd=root, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log.write(shlex.join(command) + '\n' + build.stdout)
        if build.returncode:
            sys.exit('Build failed; see build.txt')
env = dict(os.environ, OMP_DYNAMIC='FALSE')
metadata = {
    'lab': args.lab, 'platform': platform.platform(), 'available_cpus': available,
    'worker_counts': counts, 'python': platform.python_version(), 'caches': cache_entries,
    'environment': {key: value for key, value in env.items()
                    if key.startswith('OMP_') or key in ('CXX', 'CXXFLAGS', 'CXXSTD')},
    'build_command': build_command, 'build_commands': build_commands, 'commands': [],
    'repetitions': 5, 'warmups_per_method': 1,
    'note': 'Record VM/container status and competing load separately. No affinity is set.'
}
if args.lab == '4':
    metadata.update(line_bytes=line_bytes, line_size_source=line_source, total_updates=args.updates,
                    matrix_sizes=args.matrix_sizes,
                    timing_scope={'locality': 'kernel call only; result checked outside timer',
                                  'counters': 'thread creation, increments and joins; reset/check/allocation outside timer'})
if Path('/proc/cpuinfo').exists():
    metadata['cpuinfo'] = Path('/proc/cpuinfo').read_text()
for name, command in [('compiler_version', compiler + ['--version']), ('lscpu', ['lscpu'])]:
    try:
        result = subprocess.run(command, capture_output=True, text=True)
        metadata[name] = result.stdout if result.returncode == 0 else result.stderr
    except OSError as error:
        metadata[name] = str(error)

def save_metadata():
    (args.out / 'machine.json').write_text(json.dumps(metadata, indent=2) + '\n')

save_metadata()
check_targets = [target, 'false_sharing'] if args.lab == '4' else [target]
with (args.out / 'checks.txt').open('w') as output, (args.out / 'checks.err').open('w') as errors:
    for checked_target in check_targets:
        checked = subprocess.run([str(root / 'src' / checked_target), '--check'], env=env,
                                 capture_output=True, text=True)
        output.write(checked.stdout)
        errors.write(checked.stderr)
        if checked.returncode:
            sys.exit('Checks failed; complete the student kernels first. See checks.err')
if args.lab == '1':
    commands = [(target, [str(n), str(w)]) for n in [100000, 1000000, 5000000] for w in [8, 64]]
elif args.lab == '2':
    commands = [(target, ['1000000', '64', str(p)]) for p in counts]
elif args.lab == '3':
    commands = [(target, ['regular', '1000000', '64', str(p), 'static']) for p in counts]
    commands.append((target, ['irregular', '100000', '256', str(counts[-1]), 'all']))
else:
    commands = [(target, [str(n), 'row-first']) for n in args.matrix_sizes]
    commands += [('false_sharing', [str(p), str(args.updates // p), 'compact-first']) for p in counts]
for i, (binary, arguments) in enumerate(commands, 1):
    stem = args.out / f'run_{i:02d}'
    csv_path = stem.with_suffix('.csv').resolve()
    command = [str(root / 'src' / binary), *arguments, str(csv_path)]
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
