﻿#
# This file performs the installation step once the build is complete.  Installation
# primarily involves copying the build results to the IronPython site-packages
# directory.
#

import os
import sys
import shutil
import tempfile
from os.path import dirname, isdir, isfile, join

src_dir = os.getcwd()

def install(bin_dir):
    print "INSTALLING ..."
    sp_dir = join(sys.prefix, r'Lib\site-packages')
    numpy_dir = join(src_dir, '..', '..', '..')
    dll_dir = join(sys.prefix, 'DLLs')
    if not isdir(dll_dir):
        os.mkdir(dll_dir)
    for fn in ['ndarray.dll', 'NpyAccessLib.dll', 'NumpyDotNet.dll']:
        src = join(bin_dir, fn)
        dst = join(dll_dir, fn)
        if isfile(dst):
            tmp_dir = tempfile.mkdtemp()
            os.rename(dst, join(tmp_dir, fn))
        shutil.copyfile(src, dst)
        print "Installed %s" % dst

    for root, dirs, files in os.walk(numpy_dir):
        for fn in files:
            if not fn.endswith('.py'):
                continue
            abs_path = join(root, fn)
            rel_path = abs_path[len(numpy_dir) + 1:]
            dst_dir = dirname(join(sp_dir, 'numpy', rel_path))
            if not isdir(dst_dir):
                 os.makedirs(dst_dir)
            shutil.copy(abs_path, dst_dir)
    write_config(join(sp_dir, r'numpy\__config__.py'))

def write_config(path):
    fo = open(path, 'w')
    fo.write("""# this file is generated by ironsetup.py
__all__ = ["show"]
_config = {}
def show():
    print 'Numpy for IronPython'
""")
    fo.close()



if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Usage: %s <path_to_build_binaries>" % sys.argv[0]

    # For some reason Visual Studio appears to leave a trailing double-quote char
    bin_dir = sys.argv[1]
    if bin_dir[-1] == '"':
        bin_dir = bin_dir[:-1]

    install(bin_dir)
