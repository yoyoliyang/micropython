# micropython for esp8266 ddns解析脚本
# 实现的功能： 花生壳ddns解析、简易的状态查看(web)方式、wifi状态检测
# 2021.2.06
# yoyo

import uasyncio
import urequests
import socket
import network
import time
from machine import Pin


# 闪烁wifi指示灯
def blink():
    # 此处让wifi指示灯闪烁表示正在等待连接
    Pin(2, Pin.OUT)
    time.sleep(0.1)
    Pin(2, Pin.IN)
    time.sleep(0.1)


# wifi连接
def do_connect(ssid, pswd, wlan):
    if not wlan.isconnected():
        print("connecting to wireless...")
        wlan.connect(ssid, pswd)
        while not wlan.isconnected():
            blink()
            pass

    # wifi状态指示灯
    Pin(2, Pin.OUT)
    print('network config:', wlan.ifconfig())


# ddns解析, ddns需要和web_server异步进行，故使用异步任务来调度
async def ddns():
    # user = "your username"
    # pswd = "your pswd"
    # 花生壳的ddns需要Basic认证，故header需要定义认证用户名和密码（base64转换后的）
    headers = {'Authorization': 'Basic #################',
               'User-Agent': 'curl/7.64.0'}
    host = "ddns.oray.com"
    domain = "xxxxxx.vicp.net"
    uri = "http://{}/ph/update?hostname={}".format(host, domain)

    # 每隔10分钟去更新一次域名解析
    global ddns_result
    while True:
        try:
            r = urequests.get(uri, headers=headers)
            print(r.text)
            ddns_result = r.text
            await uasyncio.sleep(10 * 60)
        except OSError as e:
            ddns_result = str(e)
            continue


# web_server 端口为80 用来查看ddns状态
async def web_server():

    s = socket.socket()

    s.bind(("0.0.0.0", 80))
    s.listen(1)

    global ddns_result

    while True:
        cl, addr = s.accept()
        try:
            data = cl.recv(1024)
        except OSError:
            cl.close()
            continue
        print('client connected from', addr, data)
        data = data.decode()
        response = ['HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n',
                    "ddns status: {}".format(ddns_result)
                    ]
        cl.send("".join(response))
        cl.close()

# ddns解析后的response,由ddns函数和web函数来使用
ddns_result = ""

# WIFI
SSID = 'SSID'
PSWD = 'PSWD'
# 固定ip定义
TCPIP = ('192.168.1.131',
         '255.255.255.0',
         '192.168.1.1',
         '192.168.1.1')
# wlan初始化
WLAN = network.WLAN(network.STA_IF)
WLAN.active(True)
WLAN.ifconfig(TCPIP)

# 开机后的网络连接
do_connect(SSID, PSWD, WLAN)

# 此处创建多个异步任务，分别处理ddns解析和webserver等任务
event_loop = uasyncio.get_event_loop()
event_loop.create_task(ddns())
event_loop.create_task(web_server())

event_loop.run_forever()
