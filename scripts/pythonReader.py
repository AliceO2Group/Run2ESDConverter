#!/usr/bin/env python
import pyarrow as pa
import matplotlib.pyplot as plt
import sys

tables = {}

try:
  while True:
    reader = pa.ipc.open_stream(pa.PythonFile(sys.stdin))
    t = reader.read_all()
    print t.schema.metadata
    tables[t.schema.metadata["description"]] = t
except Exception,e:
  pass

# A couple of plots to 
df = tables["TRACKPAR"].to_pandas(zero_copy_only=True)
h1 = df.hist(column='fSigned1Pt', bins=100, range=[-30,30])
plt.savefig('figure.pdf')
df2 = tables["CALO"].to_pandas(zero_copy_only=True)
h2 = df2.hist(column='fAmplitude', bins=100, range=[0, 0.7])
plt.savefig('figure2.pdf')
