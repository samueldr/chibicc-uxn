import sys
from collections import Counter
lines = sys.stdin.read().split("\n")
lines = ['  lit' if '#' in l else l for l in lines]

c = Counter()
for size in range(4, 10):
    for i in range(len(lines)-size+1):
        if 'STH2kr' not in str(lines[i:i+size]):
            c[tuple(lines[i:i+size])] += size

for x,n in c.most_common(50):
    print('%4d'%n, ''.join(x))
