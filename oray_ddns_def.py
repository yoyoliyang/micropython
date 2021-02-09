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
    print('network config:', wlan.ifconfig())


# wifi检测，异步函数，用来检测wifi连接是否正常
async def check_connect(ssid, pswd, wlan):
    while True:
        if not wlan.isconnected():
            wlan.connect(ssid, pswd)
            while not wlan.isconnected():
                blink()
                pass


# ddns解析, ddns需要和web_server异步进行，故使用异步任务来调度
async def ddns():
    # user = "your username"
    # pswd = "your pswd"
    # 花生壳的ddns需要Basic认证，故header需要定义认证用户名和密码（base64转换后的）
    headers = {'Authorization': 'Basic #############',
               'User-Agent': 'curl/7.64.0'}
    host = "ddns.oray.com"
    domain = "yourdomain.vicp.net"
    uri = "http://{}/ph/update?hostname={}".format(host, domain)

    # 每隔10分钟去更新一次域名解析
    global ddns_result
    while True:
        try:
            r = urequests.get(uri, headers=headers)
            print(r.text)
            ddns_result = r.text
            await uasyncio.sleep(10 * 60)
        except:
            continue


# web_server 端口为80 用来查看ddns状态
async def web_server(poweron_time):

    s = socket.socket()

    s.bind(("0.0.0.0", 80))
    s.listen(1)

    global ddns_result

    while True:
        cl, addr = s.accept()
        uptime = time.time() - poweron_time
        if (uptime // (60 * 60)) >= 1:
            uptime = uptime // (60 * 60)
            uptime = "{} hours".format(uptime)
        else:
            uptime = "{} seconds".format(uptime)
        try:
            data = cl.recv(1024)
        except:
            pass
        data = data.decode()
        print('client connected from', addr)
        cl.send('HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n')
        cl.send("ddns status: {}</br>".format(ddns_result))
        cl.send("uptime: {}</br>".format(uptime))
        cl.send("{}</br>".format(data[:40]))
        cl.close()

# ddns解析后的response,由ddns函数和web函数来使用
ddns_result = ""
# poweron time开机时的时间戳
poweron_time = time.time()

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

# wifi状态指示灯
Pin(2, Pin.OUT)

# 此处创建多个异步任务，分别处理ddns解析和webserver等任务
event_loop = uasyncio.get_event_loop()
event_loop.create_task(ddns())
event_loop.create_task(web_server(poweron_time))
event_loop.create_task(check_connect(SSID, PSWD, WLAN))

event_loop.run_forever()
