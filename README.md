# BluetoothLamp
[![Gitter](https://badges.gitter.im/nephen/BluetoothLamp.svg)](https://gitter.im/nephen/BluetoothLamp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

##环境搭建
开发该蓝牙模块需先构建编译环境，Linux(ubuntu15.10)下环境搭建请查看这篇[博文](http://www.nephen.com/2016/01/BLE%E6%A0%B8%E5%BF%83%E6%A8%A1%E5%9D%97FS-QN9021%E6%A8%A1%E5%9D%97%E5%BC%80%E5%8F%91/#%E7%8E%AF%E5%A2%83%E6%90%AD%E5%BB%BA)，Windows下略同。

##源码获取及贡献

```sh
~ $ cd ~
~ $ git clone https://github.com/nephen/BluetoothLamp.git
# =>开发贡献请先建立独立的分支，稳定后合并
~ $ git checkout -b my-new-feature
~ $ git commit -am 'Add some feature'
~ $ git push origin my-new-feature
# =>合并分支
~ $ git checkout master
~ $ git merge my-new-feature
# =>解决冲突后提交到主分支
~ $ git push origin master
# =>删除分支
~ $ git branch -d my-new-feature
```

更多请参考http://www.liaoxuefeng.com/wiki/0013739516305929606dd18361248578c67b8067c8c017b000/

##说明  
此目录包含FireBLE低功耗开源平台技术案例【蓝牙透传】的固件和APP，通过【蓝牙透传】APP可以控制FireBLE低功耗开源平台或FS_QN9021低功耗蓝牙模组，功能包括：UART、GPIO、SPI、PWM等等     
【蓝牙透传】介绍：http://www.t-firefly.com/zh/firesmart/fireble/case/2015/1008/10.html     

##Firmware 
名称：FireBLE_Passthrough.bin      
说明：完整固件，通过串口方式下载固件，配套技术案例的【蓝牙透传】APP进行操作     

名称：FireBLE_Passthrough_OTA.bin      
说明：OTA固件，通过OTA方式进行空中固件升级，配套技术案例的【蓝牙透传】APP进行操作         

##APP    
【蓝牙透传】APP，包括Android和iOS版本.      

##联系方式
官方网站：http://www.t-firefly.com      
维基教程：http://wiki.t-firefly.com/index.php/FireBLE     
开发者社区：http://developer.t-firefly.com     
购买方式：http://www.t-firefly.com/zh/buy      
技术交流群（QQ）：431139193     
