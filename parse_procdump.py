#!/usr/bin/env python3
"""
parse_procdump.py

Usage:
  python3 parse_procdump.py logfile1 [logfile2] [--plot]

Outputs:
  <logfile1>.csv
  <logfile2>.csv (if provided)
  comparison.csv (if two logs provided)
  optional: priorities_plot.png (when --plot and matplotlib installed)
"""
import re
import sys
import csv
import collections
import argparse
from pathlib import Path

def parse_log(path):
    text = Path(path).read_text(errors='ignore')
    # find all PS blocks
    # header marker like: --- PS at t=10 --- or --- PS at t=10 ---
    blocks = re.split(r'---\s*PS\s*(?:at\s*)?t=?\d+\s*---', text)
    # But we also want exact ticks; iterate by matching
    snapshots = collections.OrderedDict()
    # regex to find each header and following table
    header_re = re.compile(r'---\s*PS\s*(?:at\s*)?t=?(\d+)\s*---', re.IGNORECASE)
    # finditer through text
    pos = 0
    for m in header_re.finditer(text):
        tick = int(m.group(1))
        start = m.end()
        # extract following lines (up to next header or up to 200 lines)
        next_m = header_re.search(text, pos=start)
        end = next_m.start() if next_m else len(text)
        block = text[start:end].strip('\n')
        rows = {}
        lines = block.splitlines()
        # locate header row (contains PID STATE NAME PRIO ...)
        header_idx = None
        for i,l in enumerate(lines):
            if re.search(r'\bPID\b', l) and re.search(r'\bPRIO\b', l):
                header_idx = i
                break
        if header_idx is None:
            # try immediate lines if no header (skip)
            continue
        # the data rows follow header_idx+1 until a blank line or non-digit start
        for j in range(header_idx+1, len(lines)):
            line = lines[j].strip()
            if not line:
                break
            parts = re.split(r'\s+', line)
            # require first token digit (pid)
            if not parts or not parts[0].isdigit():
                # sometimes lines are garbled; skip
                continue
            # We expect at least 8 columns: PID STATE NAME PRIO RTIME WTIME NRUN STARVING
            # But NAME can be a single token; so we use positions from end for fixed numeric columns.
            # Get last 5 tokens as prio, rtime, wtime, nrun, starving OR last 4 if STARVING not present
            if len(parts) < 6:
                continue
            # take last 5 numeric columns if possible
            try:
                starving = int(parts[-1])
                nrun = int(parts[-2])
                wtime = int(parts[-3])
                rtime = int(parts[-4])
                prio = int(parts[-5])
                # name is tokens between parts[2]..parts[-6] (state at pos 1)
                pid = int(parts[0])
                state = parts[1]
                name_tokens = parts[2:-5]
                name = ' '.join(name_tokens) if name_tokens else ''
            except Exception:
                # fallback simpler parse: try fixed indices if line is well-formed
                try:
                    pid = int(parts[0])
                    state = parts[1] if len(parts) > 1 else ''
                    name = parts[2] if len(parts) > 2 else ''
                    prio = int(parts[3]) if len(parts) > 3 else ''
                    rtime = int(parts[4]) if len(parts) > 4 else ''
                    wtime = int(parts[5]) if len(parts) > 5 else ''
                    nrun = int(parts[6]) if len(parts) > 6 else ''
                    starving = int(parts[7]) if len(parts) > 7 else 0
                except:
                    continue
            rows[pid] = {'pid': pid, 'state': state, 'name': name, 'prio': prio,
                         'rtime': rtime, 'wtime': wtime, 'nrun': nrun, 'starving': starving,
                         'raw': line}
        snapshots[tick] = rows
    return snapshots

def write_csv(snapshots, outcsv):
    with open(outcsv, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['tick','pid','name','state','prio','rtime','wtime','nrun','starving'])
        for t,snap in snapshots.items():
            for pid,row in sorted(snap.items()):
                w.writerow([t,pid,row.get('name',''),row.get('state',''),
                            row.get('prio',''),row.get('rtime',''),
                            row.get('wtime',''),row.get('nrun',''),row.get('starving','')])

def make_comparison(snaps_left, snaps_right, outcsv='comparison.csv'):
    # union of ticks
    ticks = sorted(set(list(snaps_left.keys()) + list(snaps_right.keys())))
    pids = set()
    for s in snaps_left.values():
        pids.update(s.keys())
    for s in snaps_right.values():
        pids.update(s.keys())
    pids = sorted(pids)
    with open(outcsv, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['tick','pid','prio_left','prio_right','delta'])
        for t in ticks:
            left = snaps_left.get(t,{})
            right = snaps_right.get(t,{})
            for pid in pids:
                pr1 = left.get(pid, {}).get('prio','')
                pr2 = right.get(pid, {}).get('prio','')
                delta = ''
                try:
                    if pr1 != '' and pr2 != '':
                        delta = int(pr1) - int(pr2)
                except:
                    delta = ''
                w.writerow([t,pid,pr1,pr2,delta])

def print_summary(snaps_left, snaps_right):
    # choose max tick for each
    if not snaps_left or not snaps_right:
        print("Need two logs for summary")
        return
    t_left = max(snaps_left.keys())
    t_right = max(snaps_right.keys())
    pids = sorted(set(list(snaps_left[t_left].keys()) + list(snaps_right[t_right].keys())))
    print("Summary at end of runs: left_tick=%s right_tick=%s" % (t_left, t_right))
    print("pid\tprio_left\tprio_right\tdelta (left-right)")
    for pid in pids:
        pr1 = snaps_left.get(t_left, {}).get(pid,{}).get('prio','')
        pr2 = snaps_right.get(t_right, {}).get(pid,{}).get('prio','')
        delta = ''
        try:
            if pr1 != '' and pr2 != '':
                delta = int(pr1) - int(pr2)
        except:
            delta = ''
        print(f"{pid}\t{pr1}\t{pr2}\t{delta}")

def plot_priorities(snaps_left, snaps_right, out='priorities_plot.png'):
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not available; skipping plot.")
        return
    # build per-pid timeseries
    def build_series(snaps):
        data = {}
        for t,snap in snaps.items():
            for pid,row in snap.items():
                data.setdefault(pid,[]).append((t, row.get('prio')))
        # sort times
        for pid in data:
            data[pid] = sorted(data[pid], key=lambda x: x[0])
        return data
    left_series = build_series(snaps_left)
    right_series = build_series(snaps_right)
    plt.figure(figsize=(12,6))
    # for each pid plot left as solid, right as dashed
    for pid,ser in left_series.items():
        xs = [x for x,y in ser]
        ys = [y for x,y in ser]
        plt.plot(xs, ys, label=f'pid{pid} left', linewidth=1)
    for pid,ser in right_series.items():
        xs = [x for x,y in ser]
        ys = [y for x,y in ser]
        plt.plot(xs, ys, linestyle='--', label=f'pid{pid} right', linewidth=1)
    plt.xlabel('tick')
    plt.ylabel('prio (numerical)')
    plt.title('Priority over time (left=log1, right=log2)')
    plt.legend(loc='best', fontsize='small', ncol=2)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(out)
    print("Wrote plot to", out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('log1', help='first logfile (left)')
    ap.add_argument('log2', nargs='?', help='second logfile (right)')
    ap.add_argument('--plot', action='store_true', help='generate priorities_plot.png (requires matplotlib)')
    args = ap.parse_args()
    snaps1 = parse_log(args.log1)
    out1 = args.log1 + '.csv'
    write_csv(snaps1, out1)
    print("Wrote", out1)
    snaps2 = None
    if args.log2:
        snaps2 = parse_log(args.log2)
        out2 = args.log2 + '.csv'
        write_csv(snaps2, out2)
        print("Wrote", out2)
        make_comparison(snaps1, snaps2, outcsv='comparison.csv')
        print("Wrote comparison.csv")
        print_summary(snaps1, snaps2)
        if args.plot:
            plot_priorities(snaps1, snaps2)
    else:
        if args.plot:
            print("Plot requires two logs for side-by-side; skipping plot.")
    print("Done.")

if __name__ == '__main__':
    main()