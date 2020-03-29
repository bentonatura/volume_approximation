import subprocess

n = 2
m = 10

subprocess.call(['./generate', '-n', str(n), '-m', str(m)])
filename = "sdp_prob_" + str(n) + "_" + str(m) + ".txt"

subprocess.call(['./vol', '-file', filename, '-sample'])
subprocess.call(['./vol', '-file', filename, '-sample', '-boundary'])





