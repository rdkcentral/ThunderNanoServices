import datetime
import logging
import sys

from common import utils
from device_config import *

conf = utils.get_config(DEVICE_TYPE)


def run_test():
    cmd = "grep -nr 'Startup: EVENT: Network' /var/log/messages"
    if conf is not None:
        if utils.ssh_wait(DEVICE_TYPE, 30):
            log_line = utils.run_ssh_command(DEVICE_TYPE, cmd, 20)
            if log_line:
                ts = utils.find_pattern(log_line[0], TIME_24HMS)
                if ts:
                    logging.info(f"Took {utils.time_diff(datetime.datetime.strptime(ts, '%H:%M:%S'))} seconds to get IP")
            else:
                logging.error(f"Test {sys.modules[__name__]} failed - Command {cmd} failed")
        else:
            logging.error(f"Test {sys.modules[__name__]} failed - Device[{conf['host']}] not online")
    else:
        logging.error(f"Test {sys.modules[__name__]} failed - Device config missing")


def main():
    run_test()


if __name__ == "__main__":
    main()
