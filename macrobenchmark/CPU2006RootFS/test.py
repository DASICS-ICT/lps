import subprocess

def get_spec_int():
  return [
    # "400.perlbench", 
    "401.bzip2",
    # "403.gcc",
    "429.mcf",
    "445.gobmk",
    "456.hmmer",
    "458.sjeng",
    "462.libquantum",
    "464.h264ref",
    # "471.omnetpp",
    "473.astar",
    # "483.xalancbmk"
  ]
  
if __name__ == '__main__':
    for name in get_spec_int():
        subprocess.run(f'./test.sh {name}', check=True, shell=True)