#!/usr/bin/env python
import pyarrow as pa
import matplotlib.pyplot as plt
import sys

reader = pa.ipc.open_stream(pa.PythonFile(sys.stdin))
t = reader.read_next_batch()
df = t.to_pandas(zero_copy_only=True)
h1 = df.hist(column='fSigned1Pt', bins=100)
plt.savefig('figure.pdf')
