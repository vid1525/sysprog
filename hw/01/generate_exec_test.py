import os
import sys
import argparse

parser = argparse.ArgumentParser(description="generate required count of tests")
parser.add_argument("-n", type=int, required=True, help="number of tests")
parser.add_argument("-c", type=int, required=True, help="cnt of numbers in each test")
parser.add_argument("-m", type=int, required=True, help="maximal number")
args = parser.parse_args()

exec_str = "./main "

for i in range(1, args.n + 1):
    os.system("python3 generator.py -f test{} -c {} -m {}".format(i, args.c, args.m))
    exec_str = exec_str + "test{} ".format(i)
    print("test{} -- generated".format(i))

print("./main execution launched")
os.system(exec_str)

print("Testing launched")
os.system("python3 checker.py -f test1")
