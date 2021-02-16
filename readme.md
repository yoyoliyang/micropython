## 利用esp8266(esp32)来实现解析花生壳免费二级域名

# 该脚本存在问题，因为socket的webserver会阻塞，导致异步任务ddns函数无法重复执行。目前的解决方式是用arduino IDE写的c程序实现的。

主要还是用到了花生壳自带的web解析方法

这里用的是micropython中的urequest包来进行的web请求

另外用到了uasyncio来实现的多个协程任务同步执行
