#!python3

import sys

def process(file):
    with open(file, 'r') as f:
        time_list = [
            float(item.split('#')[0].strip())
            for line in f
            for item in line.split('|')
            if "elapsed in second" in item
        ]
    print(f'{sum(time_list):.2f}')
            
if __name__ == '__main__':
    process(sys.argv[1])