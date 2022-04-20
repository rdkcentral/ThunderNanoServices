import importlib
import os
import sys
from common import utils, config
from common.Log import *


def load_exec():
    cwd = os.getcwd()
    conf = config.Config("DUT_config.json")
    types = conf.get_types()
    for t in types:
        try:
            test_module = __import__(t)
            if hasattr(test_module, "bootstrap"):
                for module in test_module.bootstrap:
                    sys.path.insert(1, cwd + '/' + t)
                    m = importlib.import_module(module)
                    m.main()
            for module in test_module.__all__:
                sys.path.insert(1, cwd + '/' + t)
                m = importlib.import_module(module)
                m.main()
        except Exception as exception:
            log.Error(f"{exception} - found in {cwd}",)
            continue



if __name__ == "__main__":
    load_exec()
