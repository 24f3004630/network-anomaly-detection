import os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

STAT = os.environ.get("STAT", "median")   # "median" or "min"
KEYS = ["experiment", "method", "n", "p", "threads"]

df = pd.read_csv("results/results.csv")
agg = df.groupby(KEYS)[["t_load", "t_compute"]].agg(STAT).reset_index()
spread = df.groupby(KEYS)["t_compute"].agg(["min", "median", "max"]).reset_index()

# ---- strong scaling (n fixed, MPI ranks only, 1 thread each)
s = agg[(agg.experiment == "strong") & (agg.method == "kmeans_mpi") & (agg.threads == 1)]
s = s.sort_values("p").copy()
t1 = s[s.p == 1].t_compute.iloc[0]
s["speedup"] = t1 / s.t_compute
s["efficiency"] = s.speedup / s.p
seq = agg[(agg.experiment == "strong") & (agg.method == "kmeans_seq")].t_compute.iloc[0]

fig, ax = plt.subplots(1, 2, figsize=(10, 4))
ax[0].plot(s.p, s.speedup, "o-", label="measured")
ax[0].plot(s.p, s.p, "--", label="ideal")
ax[0].set_xlabel("MPI processes"); ax[0].set_ylabel("speedup vs MPI p=1")
ax[0].legend(); ax[0].grid(True)
ax[1].plot(s.p, s.efficiency * 100, "o-")
ax[1].set_ylim(0, 110); ax[1].grid(True)
ax[1].set_xlabel("MPI processes"); ax[1].set_ylabel("parallel efficiency (%)")
fig.suptitle(f"Strong scaling, n = 4M ({STAT} of 5 runs)")
fig.tight_layout(); fig.savefig("results/strong_scaling.png", dpi=150)

# ---- data volume
v = agg[agg.experiment == "volume"]
plt.figure(figsize=(6, 4))
for (m, p), g in v.groupby(["method", "p"]):
    g = g.sort_values("n")
    label = "sequential" if m == "kmeans_seq" else f"MPI p={p}"
    plt.plot(g.n / 1e6, g.t_compute, "o-", label=label)
plt.xlabel("rows (millions)"); plt.ylabel("compute time (s)")
plt.title(f"Runtime vs data volume ({STAT})")
plt.legend(); plt.grid(True); plt.tight_layout()
plt.savefig("results/data_volume.png", dpi=150)

# ---- hybrid
h = agg[agg.experiment == "hybrid"].copy()
h["cfg"] = h.p.astype(str) + "x" + h.threads.astype(str)
h = h.sort_values("p", ascending=False)
plt.figure(figsize=(6, 4))
plt.bar(h.cfg, h.t_compute)
plt.xlabel("MPI ranks x OpenMP threads"); plt.ylabel("compute time (s)")
plt.title(f"Hybrid configurations, n = 4M ({STAT})")
plt.tight_layout(); plt.savefig("results/hybrid.png", dpi=150)

# ---- text summary
sp = spread[spread.experiment == "strong"].sort_values(["method", "p"])
with open("results/summary.txt", "w") as f:
    f.write(f"STATISTIC USED: {STAT} of repeated runs\n\n")
    f.write(f"STRONG SCALING, n=4M (baseline = MPI p=1; sequential binary = {seq:.3f} s)\n")
    f.write(s[["p", "t_compute", "speedup", "efficiency"]].round(3).to_string(index=False))
    f.write("\n\nRUN-TO-RUN SPREAD, strong scaling (min / median / max compute seconds)\n")
    f.write(sp[["method", "p", "min", "median", "max"]].round(3).to_string(index=False))
    f.write("\n\nDATA VOLUME\n")
    f.write(v.sort_values(["method", "p", "n"])[["method", "p", "n", "t_compute"]]
            .round(3).to_string(index=False))
    f.write("\n\nHYBRID, n=4M\n")
    f.write(h[["cfg", "t_compute"]].round(3).to_string(index=False) + "\n")
print(open("results/summary.txt").read())