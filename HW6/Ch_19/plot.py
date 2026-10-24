#!/usr/bin/env python3
"""Plot TLB measurements (OSTEP ch.19 Q4).

Reads every results/*.csv written by run.sh (one file per machine), and for
each machine plots the median ns-per-access against the number of pages,
with the min..max spread of the repetitions shown as error bars.

Usage: python3 plot.py [results_dir]   -> writes tlb_all.png and tlb_<label>.png
"""
import csv
import glob
import os
import statistics
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")            # draw to files, no display needed (works over ssh)
import matplotlib.pyplot as plt

results_dir = sys.argv[1] if len(sys.argv) > 1 else "results"

# data[label][pages] -> list of ns_per_access from the repetitions
data = defaultdict(lambda: defaultdict(list))
for path in sorted(glob.glob(os.path.join(results_dir, "*.csv"))):
    with open(path) as f:
        for row in csv.DictReader(f):
            data[row["label"]][int(row["pages"])].append(float(row["ns_per_access"]))

if not data:
    sys.exit(f"no CSV files found in {results_dir}/")


def draw(ax, label, by_pages):
    pages = sorted(by_pages)
    med = [statistics.median(by_pages[p]) for p in pages]
    lo = [m - min(by_pages[p]) for m, p in zip(med, pages)]
    hi = [max(by_pages[p]) - m for m, p in zip(med, pages)]
    ax.errorbar(pages, med, yerr=[lo, hi], marker="o", capsize=3, label=label)


def finish(ax, title):
    ax.set_xscale("log", base=2)          # pages double each step -> evenly spaced
    ax.set_xlabel("Number of pages")
    ax.set_ylabel("Time per access (ns)")
    ax.set_title(title)
    ax.set_ylim(bottom=0)
    ax.grid(True, which="both", alpha=0.3)
    ax.legend()


# one figure with every machine for comparison
fig, ax = plt.subplots(figsize=(8, 5))
for label, by_pages in sorted(data.items()):
    draw(ax, label, by_pages)
finish(ax, "TLB size measurement (all machines)")
fig.tight_layout()
fig.savefig("tlb_all.png", dpi=150)

# one figure per machine
for label, by_pages in sorted(data.items()):
    fig, ax = plt.subplots(figsize=(8, 5))
    draw(ax, label, by_pages)
    finish(ax, f"TLB size measurement: {label}")
    fig.tight_layout()
    fig.savefig(f"tlb_{label}.png", dpi=150)

print("wrote tlb_all.png and", ", ".join(f"tlb_{l}.png" for l in sorted(data)))
