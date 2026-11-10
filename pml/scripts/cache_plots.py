"""Summarize the supplied Lab 4 CSV format; called by plot_results.py."""
from collections import defaultdict
import csv
import math
from pathlib import Path
import statistics


def plot_cache(directory):
    import matplotlib.pyplot as plt

    groups = defaultdict(list)
    for path in sorted(directory.glob('run_*.csv')):
        with path.open() as stream:
            for row in csv.DictReader(stream):
                key = (path.name, row['workload'], row['method'])
                value = float(row['time_s'])
                if not math.isfinite(value) or value <= 0:
                    raise ValueError(f'Invalid elapsed time in {path}')
                groups[key].append(row)
    if not groups:
        raise ValueError('No Lab 4 measurements found')
    means = {}
    for key, data in groups.items():
        if len(data) != 5 or sorted(int(r['repetition']) for r in data) != [1, 2, 3, 4, 5]:
            raise ValueError(f'Expected five distinct repetitions for {key}')
        fields = ['n', 'workers', 'elements', 'element_bytes', 'line_size',
                  'counter_size', 'lock_free', 'shared_lines', 'checksum']
        if any(len({row[field] for row in data}) != 1 for field in fields):
            raise ValueError(f'Configuration changed within {key}')
        means[key] = statistics.mean(float(row['time_s']) for row in data)
    rows = []
    for (source, workload, method), data in groups.items():
        if workload not in ('locality', 'counters'):
            raise ValueError(f'Unexpected Lab 4 workload: {workload}')
        left, right = ('column', 'row') if workload == 'locality' else ('compact', 'padded')
        comparison = means[(source, workload, left)] / means[(source, workload, right)]
        first = data[0]
        row = {name: first[name] for name in ['n', 'workers', 'elements', 'element_bytes',
                                             'line_size', 'counter_size', 'lock_free', 'shared_lines']}
        mean = means[(source, workload, method)]
        values = [float(r['time_s']) for r in data]
        row.update(source=source, workload=workload, method=method, mean_s=mean,
                   min_s=min(values), max_s=max(values),
                   column_over_row=comparison if workload == 'locality' else '',
                   compact_over_padded=comparison if workload == 'counters' else '',
                   elements_per_s=int(first['elements']) / mean,
                   useful_bytes_per_s=int(first['elements']) * int(first['element_bytes']) / mean
                   if workload == 'locality' else '')
        rows.append(row)
    with (directory / 'summary.csv').open('w') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    for workload, methods, filename, xlabel in [
            ('locality', ('row', 'column'), 'locality.png', 'Matrix dimension N'),
            ('counters', ('compact', 'padded'), 'false_sharing.png', 'Threads (fixed total updates)')]:
        selected = [r for r in rows if r['workload'] == workload]
        if not selected:
            continue
        xfield = 'n' if workload == 'locality' else 'workers'
        if workload == 'counters' and len({int(r['elements']) for r in selected}) != 1:
            raise ValueError('Counter plots require fixed total work across thread counts')
        plt.figure()
        for method in methods:
            data = sorted((r for r in selected if r['method'] == method), key=lambda r: int(r[xfield]))
            if len({r[xfield] for r in data}) != len(data):
                raise ValueError('Keep different experiment series in separate directories')
            plt.errorbar([int(r[xfield]) for r in data], [r['mean_s'] for r in data],
                         yerr=[[r['mean_s'] - r['min_s'] for r in data],
                               [r['max_s'] - r['mean_s'] for r in data]],
                         marker='o', capsize=3, label=method)
        plt.xlabel(xlabel)
        plt.ylabel('Mean elapsed time (s)')
        plt.title('Five repetitions; error bars show min–max')
        plt.grid(alpha=0.25)
        plt.legend()
        plt.tight_layout()
        plt.savefig(directory / filename)
        plt.close()
    print(f'Saved summary.csv, locality.png and false_sharing.png in {directory}')
