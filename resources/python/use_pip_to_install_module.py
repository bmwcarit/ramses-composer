import sys
import runpy

sys.argv = ['pip', 'install', 'simplejson']
runpy.run_module("pip", run_name="__main__")