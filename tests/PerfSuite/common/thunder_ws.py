import json
import struct

import websocket
from common.Log import log
from threading import Thread


class WSClient:
    def __init__(self):
        self.thread = None
        self.url = ""
        self.ws = None
        websocket.enableTrace(True)

    def connect(self, url):
        self.ws = websocket.WebSocket()
        try:
            self.ws.connect(url)
        except Exception as e:
            log.Error(u'connect Exception:{}'.format(str(e)))

    def send_request(self, method, params=None):
        msg = None
        if self.ws:
            d = {'method': method}
            if params is not None:
                d['parameters'] = params
            j = json.dumps(d)
            try:
                self.ws.send(j)
                resp_opcode, msg = self.ws.recv_data()
                log.Info("Response opcode: " + str(resp_opcode))
                if resp_opcode == 8 and len(msg) >= 2:
                    log.Info("Response close code: " + str(struct.unpack("!H", msg[0:2])[0]))
                    log.Info("Response message: " + str(msg[2:]))
                else:
                    log.Info("Response message: " + str(msg))
            except Exception as e:
                log.Error(u'send_request Exception:{}'.format(str(e)))
        else:
            log.Error("Websocket not connected")
        return msg

    def close(self):
        if self.ws:
            self.ws.close()
            self.ws = None

class ControllerClient:
    def __init__(self):
        self.thread = None
        self.url = ""
        self.ws = None

    def start(self, url):
        self.url = url
        self.ws = None
        self.ws = websocket.WebSocketApp(self.url,
                                         on_message=self.on_message,
                                         on_error=self.on_error,
                                         on_close=self.on_close,
                                         on_open=self.on_open,
                                         subprotocols=["notification"])
        self.thread = Thread(target=self.ws.run_forever)
        self.thread.start()

    def stop(self):
        if self.thread and self.thread.isAlive():
            self.ws.close()
            self.thread.join()

    def on_message(self, ws, message):
        log.Info(message)

    def on_open(self, ws):
        log.Info("WebSocket Opened")

    def on_error(self, ws, error):
        log.Info(f"WebSocket Error - {error}")

    def on_close(self, ws, close_status_code, close_msg):
        # Because on_close was triggered, we know the opcode = 8
        log.Info("WebSocket Closed: ")
        if close_status_code or close_msg:
            log.Info("close status code: " + str(close_status_code))
            log.Info("close message: " + str(close_msg))



def start_controller_client(host, port, path):
    wsc = ControllerClient()
    wsc.start("ws://" + host + ":" + str(port) + "/" + path)
    return wsc

