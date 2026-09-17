import subprocess
import csv
import pandas as pd
import matplotlib.pyplot as plt

buffer_sizes = [2**i for i in range(17)]

with open("results.csv", "w", newline = "") as f:
    writer = csv.writer(f);
    writer.writerow(["bufsize", "real"])
    for b in buffer_sizes:
        print(f"using bufsize {b}")
        with open("testfile2.dat","w") as output:
            result = subprocess.run(
                    [
                        "/usr/bin/time",
                        "-f", "%e",
                        "./kit",
                        "-b", str(b),
                        "test64MB.dat"
                    ],
                    stdout = output,
                    stderr = subprocess.PIPE,
                    text = True
                    )
            
            real = float(result.stderr.strip())
            writer.writerow([b,real])

data = pd.read_csv("results.csv")
data["MB/sec"] = 64/data["real"]

plt.plot(data["bufsize"], data["MB/sec"])
plt.grid()
plt.xscale("log", base=2)
plt.xlabel("Bufsize (bytes)")
plt.ylabel("throughput (MB/sec)")
plt.savefig("throughput relative to Bufsize.png")
               
