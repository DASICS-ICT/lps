#!/usr/bin/env python3

import argparse
import os
import subprocess
# usage: spec_copy.py [name] [src] [dest]

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
  
is_dry_run = False
is_no_clean = False

def dry_run(cmd_list):
    if is_dry_run:
        print(cmd_list)
    else:
        subprocess.run(cmd_list, check=True, shell=True)
        
def clean_other(dest):
    for root, dirs, _ in os.walk(dest):
        for dir in dirs:
            if dir in get_spec_int():
                dir_path = os.path.join(root, dir)
                dry_run(f'rm -r {dir_path}')
    

def spec_copy(name, src, dest):
    # clean other spec
    if not is_no_clean:
        clean_other(dest)
    
    dest_path = os.path.join(dest, name)
    if os.path.exists(dest_path):
        subprocess.run(['rm', '-r', f'{dest_path}'])
    dry_run(f'mkdir -p {dest_path}')
    src_path = os.path.join(src, name)
    
    # copy elf
    elf_path = os.path.join(src_path, 'build', name)
    if not os.path.exists(elf_path):
        print(f'elf [{elf_path}] not exists!')
        exit()
    dry_run(f'cp {elf_path} {dest_path}')
    
    # copy data
    data_all_path = os.path.join(src_path, 'data', 'all', 'input')
    data_test_path = os.path.join(src_path, 'data', 'test', 'input')
    
    if os.path.exists(data_all_path):
        dry_run(f'cp -r {data_all_path}/* {dest_path}')
    
    if os.path.exists(data_test_path):
        dry_run(f'cp -r {data_test_path}/* {dest_path}')
    
    # copy sh
    sh_path = os.path.join(src_path, 'run-test.sh')
    if not os.path.exists(sh_path):
        print(f'sh [{os.path.basename(sh_path)}] not exists!')
        exit()
    dry_run(f'cp {sh_path} {dest_path}')
    
        
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='copy spec test to rootfs')
    parser.add_argument('--dryrun', '-d', default=False, action='store_true', help='dry run')
    parser.add_argument('--noclean', '-nc', default=False, action='store_true', help='do not clean other spec test')
    parser.add_argument('src', help='src path')
    parser.add_argument('dest', help='dest path')
    parser.add_argument('name', help='test item name')
    
    args = parser.parse_args()
    
    is_dry_run = args.dryrun
    is_no_clean = args.noclean
    
    if not args.name in get_spec_int():
        print(f'unknown benchmark: {args.name}')
        exit()
        
    if not os.path.exists(args.src):
        print(f'invalid src path: {args.src}')
        exit()
    
    if not os.path.exists(args.dest):
        print(f'invalid dest path: {args.dest}')
        exit()
    
    spec_copy(args.name, args.src, args.dest)
    
    
    
    