# PushService
简单的推送服务

 协议说明：
        (一) 推送采用Post请求，Post定义：
        url: http://ip:port/push?msgid=1&expire_time=50&platform=0 (msgid 为消息id, expire_time有效时长，platform为推送平台，目前为0）
        body 采用Json格式
        ｛
            “client_ids":["", "", ""],
            "msg": {
                "appid":"12345",       
                 "title":"",        
                 "content":""
            }
        ｝
         1. client_ids:推送接收方client_id，使用Json数组；
          2. msg 消息，是一个json格式数据;
          3. appid接收处理的appid;
          4.title：表示该条信息的标题； 
          5. content：表示消息的内容； 
            
    （二）推送数据上报，采用Get请求，
          url: http://ip:port/report?client_id=ab123&msgid=1&action=1  (client_id客户端设备id, msgid消息id，action表示1接收到消息，2点击消息，3卸载应用）
          
客户端接入时需要实现的可以根据自己需要。主要的功能可以参照proto协议文件，push_proto_client.proto
