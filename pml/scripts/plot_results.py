#!/usr/bin/env python3
import argparse
from collections import defaultdict
import csv
from pathlib import Path
import math
import statistics
import sys

parser = argparse.ArgumentParser(description='Summarize and plot measured lab data.')
parser.add_argument('directory', type=Path)
args = parser.parse_args()
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

groups = defaultdict(list)
for path in sorted(args.directory.glob('run_*.csv')):
    with path.open() as stream:
        for row in csv.DictReader(stream):
            key = (path.name, row['method'], row.get('workload', 'regular'),
                   int(row['n']), int(row['w']), int(row.get('workers', 1)),
                   row.get('schedule', 'none'))
            value = float(row['time_s'])
            if not math.isfinite(value) or value <= 0:
                sys.exit(f'Invalid elapsed time in {path}')
            groups[key].append(value)
if not groups:
    sys.exit('No run_*.csv measurement files found.')
if any(len(values) != 5 for values in groups.values()):
    sys.exit('Expected five measured repetitions for every configuration.')
means = {key: statistics.mean(values) for key, values in groups.items()}
rows = []
for key, values in groups.items():
    source, method, workload, n, w, num_workers, schedule = key
    row = dict(source=source, method=method, workload=workload, n=n, w=w,
               workers=num_workers, schedule=schedule, mean_s=means[key],
               min_s=min(values), max_s=max(values), speedup='', efficiency='',
               ratio_vs_sliding='')
    if method in ('threads', 'openmp'):
        baseline = means[(source, 'direct', workload, n, w, 1, 'none')]
        row['speedup'] = baseline / means[key]
        row['efficiency'] = row['speedup'] / num_workers
        if workload == 'regular':
            row['ratio_vs_sliding'] = means[(source, 'sliding', workload, n, w, 1, 'none')] / means[key]
    rows.append(row)
with (args.directory / 'summary.csv').open('w') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)

def save(name, ylabel, xlabel):
    plt.ylabel(ylabel)
    plt.xlabel(xlabel)
    plt.grid(alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig(args.directory / name)
    plt.close()

parallel = [r for r in rows if r['method'] in ('threads', 'openmp')]
if not parallel:
    for w in sorted({r['w'] for r in rows}):
        plt.figure()
        for method in ('direct', 'sliding'):
            data = sorted((r for r in rows if r['w'] == w and r['method'] == method), key=lambda r: r['n'])
            plt.errorbar([r['n'] for r in data], [r['mean_s'] for r in data],
                         yerr=[[r['mean_s']-r['min_s'] for r in data],
                               [r['max_s']-r['mean_s'] for r in data]],
                         marker='o', capsize=3, label=method)
        plt.title(f'Window W={w}; bars show min–max')
        save(f'time_w{w}.png', 'Mean computation time (s)', 'Input samples N')
else:
    regular = [r for r in parallel if r['workload'] == 'regular']
    for field, label in [('speedup', 'Speedup relative to sequential direct'),
                         ('ratio_vs_sliding', 'Sequential sliding time / parallel time')]:
        plt.figure()
        for method, schedule in sorted({(r['method'], r['schedule']) for r in regular}):
            data = sorted((r for r in regular if r['method'] == method and r['schedule'] == schedule), key=lambda r: r['workers'])
            plt.plot([r['workers'] for r in data], [r[field] for r in data], marker='o', label=f'{method}: {schedule}')
        plt.axhline(1, color='gray', linestyle='--')
        save(f'{field}.png', label, 'Actual workers')
    irregular = [r for r in parallel if r['workload'] == 'irregular']
    if irregular:
        plt.figure()
        plt.bar([r['schedule'] for r in irregular], [r['mean_s'] for r in irregular],
                yerr=[[r['mean_s']-r['min_s'] for r in irregular],
                      [r['max_s']-r['mean_s'] for r in irregular]], capsize=4,
                label=f"{irregular[0]['workers']} workers; min–max")
        save('scheduling.png', 'Mean computation time (s)', 'Schedule')
print(f'Saved summary.csv and plots in {args.directory}')
