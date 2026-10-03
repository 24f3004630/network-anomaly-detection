import argparse, os
import numpy as np

p = argparse.ArgumentParser()
p.add_argument("n", type=int, help="number of flows (rows)")
p.add_argument("--d", type=int, default=16, help="features per flow")
p.add_argument("--anom", type=float, default=0.01, help="anomaly fraction")
p.add_argument("--seed", type=int, default=42)
p.add_argument("--out", default="data")
a = p.parse_args()

os.makedirs(a.out, exist_ok=True)
rng = np.random.default_rng(a.seed)
means = rng.uniform(-1, 1, size=(3, a.d)).astype(np.float32)  # 3 "normal traffic" modes
base = os.path.join(a.out, f"flows_{a.n}")
CHUNK = 1_000_000  # write in chunks so RAM stays small

with open(base + ".bin", "wb") as fx, open(base + ".labels", "wb") as fl:
    done = 0
    while done < a.n:
        m = min(CHUNK, a.n - done)
        comp = rng.integers(0, 3, size=m)
        x = means[comp] + rng.standard_normal((m, a.d), dtype=np.float32)
        lab = rng.random(m) < a.anom
        for i in np.nonzero(lab)[0]:
            feats = rng.choice(a.d, size=3, replace=False)
            x[i, feats] += rng.choice([-1, 1], size=3) * rng.uniform(10, 14, size=3)
        fx.write(x.astype(np.float32).tobytes())
        fl.write(lab.astype(np.uint8).tobytes())
        done += m

print(f"wrote {base}.bin  ({a.n} x {a.d} float32, {a.n*a.d*4/1e6:.0f} MB) and {base}.labels")