import sys
from common.thunder_ws import *
from common import utils
from device_config import *
from common.Log import log

conf = utils.get_config(DEVICE_TYPE)

controller_url = "Service/Controller"


def setup():
    if conf is not None:
        if utils.reboot(DEVICE_TYPE):
            if utils.ssh_wait(DEVICE_TYPE, 30):
                host, port = utils.get_host_port(DEVICE_TYPE)
                start_controller_client(host, port, controller_url)
            else:
                log.Error(
                    f"Test {sys.modules[__name__]} failed - Device[{conf['host']}] didn't come online after reboot")
        else:
            log.Error(f"Test {sys.modules[__name__]} failed - Device[{conf['host']}] couldnt be rebooted")
    else:
        log.Error(f"Test {sys.modules[__name__]} failed - Device config missing")


def main():
    setup()


if __name__ == "__main__":
    main()
