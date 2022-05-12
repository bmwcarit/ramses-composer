import sys
import runpy

sys.argv = ['pip', 'uninstall', '-y', 'simplejson']
runpy.run_module("pip", run_name="__main__")
