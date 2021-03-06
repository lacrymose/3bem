import sys
import os
# Prepending the parent directory to the path is necessary to force the
# tbempy.* imports to import from this directory rather than a globally installed
# copy of tbempy
this_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(this_dir, os.pardir))

from tbempy.testing import setup_tests
from tbempy.setup import files_in_dir, get_extension_config
from numpy.distutils.misc_util import Configuration

def make_config(path, prefix):
    def configuration(parent_package='',top_path=None):
        config = Configuration(path, parent_package, top_path)

        ext_config = get_extension_config()
        ext_config['sources'] += [
            f for f in files_in_dir(path, 'cpp')
            if f.startswith(os.path.join(path, prefix))
        ]

        config.add_extension('runner', **ext_config)
        return config
    return configuration

if __name__ == "__main__":
    setup_tests(
        os.path.join('unit_tests', 'runner'),
        make_config('unit_tests', 'test_')
    )
