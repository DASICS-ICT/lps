#!/usr/bin/env python3

import argparse
import os
import subprocess
import spec_copy
# usage: gen_rootfs.py [src] [boot]
# boot/build.sh
# boot/rootfs/rootfsimg

def main(src, boot):
    dest = os.path.join(boot, "rootfs", "rootfsimg")
    
    for name in spec_copy.get_spec_int():
        # spec copy
        print(f'spec copy {name} from {src} to {dest}...')
        spec_copy.spec_copy(name, src, dest)
        # get rootfs size
        print(f'check rootfs size...')
        result = subprocess.run(f'du -s {dest}', shell=True, capture_output=True, text=True)
        rootfs_size = int(result.stdout.split()[0])
        print(f'{name} rootfs size is {rootfs_size}B')
        if rootfs_size > 64000:
            print(f'WARNING: {name} rootfs size is {rootfs_size}B, maybe TO BIG!')
        # gen boot bin && rename
        board_type = "vcu128"
        print(f'gen {boot}/images/{board_type}/{name}.bin...')
        log_dir = os.path.join(boot, 'logs')
        os.makedirs(log_dir, exist_ok=True)
        log_file_path = os.path.join(log_dir, f'{name}.log')
        
        image_path = os.path.join(boot, "images", board_type)
        os.makedirs(image_path, exist_ok=True)
        
        rootfs_bin = os.path.join(image_path, f'{board_type}-rootfs.bin')
        target_bin = os.path.join(image_path, f'{name}.bin')
        
        with open(log_file_path, 'a') as log_file:
            flag = "-fpga" if board_type == "vcu128" else ""
            subprocess.run(f'cd {boot} && ./build.sh {flag} images/{board_type}/', shell=True, stdout=log_file, stderr=log_file)
            subprocess.run(f'mv {rootfs_bin} {target_bin}', shell=True)
            
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='gen spec int rootfs')
    parser.add_argument('src')
    parser.add_argument('boot')
    args = parser.parse_args()
    
    main(args.src, args.boot)
    
