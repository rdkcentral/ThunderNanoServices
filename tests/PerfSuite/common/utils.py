import subprocess
import logging

from paramiko import ssh_exception
from paramiko.client import SSHClient, AutoAddPolicy
from common import config
import os
import re
from datetime import datetime, timedelta
import time
import platform  # For getting the operating system name
import subprocess  # For executing a shell command


def run_ssh_command(device_type, cmd, wait=2):
    conf = get_config(device_type)
    res = None
    if conf is not None:
        s = SSHClient()
        s.set_missing_host_key_policy(AutoAddPolicy)
        try:
            if "port" in conf["ssh"]:
                port = conf["ssh"]["port"]
            else:
                port = 22
            s.connect(conf["host"], 22, conf["ssh"]["user"], conf["ssh"]["password"], timeout=wait)
            stdin, stdout, stderr = s.exec_command(cmd)
            res = stdout.readlines()
        except Exception as e:
            logging.error(f"Failed connecting to host - {e}")
        finally:
            s.close()
    return res


def get_config(device_type):
    conf = config.Config("DUT_config.json")
    return conf.get_config("type", device_type)

def get_host_port(device_type):
    conf = get_config(device_type)
    h = None
    p = None
    if conf:
        h = conf["host"]
        if conf["thunderPort"]:
            p = conf["thunderPort"]
        else:
            p = 80
    return h,p

def find_pattern(log, time_format):
    match = re.search(time_format, log)
    if match:
        return match.group()
    else:
        return None


def time_diff(end, start=datetime.strptime('00:00:00', '%H:%M:%S')):
    if end:
        return (end - start).total_seconds()
    return 0


def ping(host):
    """
    Returns True if host (str) responds to a ping request.
    Remember that a host may not respond to a ping (ICMP) request even if the host name is valid.
    """

    # Option for the number of packets as a function of
    param = '-n' if platform.system().lower() == 'windows' else '-c'

    # Building the command. Ex: "ping -c 1 google.com"
    command = ['ping', param, '1', host]

    return subprocess.call(command) == 0


def reboot(device_type, wait=2):
    conf = get_config(device_type)
    res = False
    if conf is not None:
        res = True
        s = SSHClient()
        s.set_missing_host_key_policy(AutoAddPolicy)

        try:
            if "port" in conf["ssh"]:
                port = conf["ssh"]["port"]
            else:
                port = 22
            s.connect(conf["host"], 22, conf["ssh"]["user"], conf["ssh"]["password"], timeout=wait)
            timeout = 0.5
            transport = s.get_transport()
            chan = transport.open_session(timeout=timeout)
            chan.settimeout(timeout)
            cmd = "/sbin/reboot -f > /dev/null 2>&1 &"
            stdin, stdout, stderr = s.exec_command(cmd)
        except:
            pass
        finally:
            s.close()
    return res


def ping_wait(device_type, wait_seconds):
    conf = get_config(device_type)
    if conf is not None:
        cur = datetime.now()
        delay = cur + timedelta(wait_seconds)
        while delay > cur:
            if ping(conf["host"]):
                return True
            cur = datetime.now()
            time.sleep(1)
    return False


def ssh_wait(device_type, wait_seconds, retry_interval=1):
    conf = get_config(device_type)
    res = False
    if conf is not None:
        client = SSHClient()
        client.set_missing_host_key_policy(AutoAddPolicy())
        retry_interval = float(retry_interval)
        timeout = int(wait_seconds)
        timeout_start = time.time()
        while time.time() < timeout_start + timeout:
            time.sleep(retry_interval)
            try:
                client.connect(conf["host"], 22, conf["ssh"]["user"], conf["ssh"]["password"])
            except ssh_exception.SSHException as e:
                # socket is open, but not SSH service responded
                logging.error(e)
                if e == 'Error reading SSH protocol banner':
                    logging.error(e)
                    continue
            except ssh_exception.NoValidConnectionsError as e:
                logging.error('SSH transport is not ready...')
                continue
            logging.info('Device Online')
            res = True
            break
    return res
