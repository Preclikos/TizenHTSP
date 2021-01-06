import sys
import argparse
import os, fnmatch

def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                yield filename

def main(args):
    print('Clean started')

    parser = argparse.ArgumentParser(usage='nacl_clean.py PATERNS',
                                   description="Files you want to delete")
    parser.add_argument('files', nargs='+',)
    options = parser.parse_args(args)
    for pattern in options.files:
        for filename in find_files('../', pattern):
            print 'Found file: ', filename
            os.remove(filename)

    print('Clean completed')

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
