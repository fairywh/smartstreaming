## 1. introduction
Smartstreaming is a high-performance and scalable streaming media server.

## 2. design
![image](https://user-images.githubusercontent.com/5512308/147847659-d402fafd-6cf0-4a96-9121-2d5624f70530.png)
   -----------------------
   | io |      Coroutine     |
   ----------------------------
   | transport | tcp/udp/srt/quic  |
   ---------------------------
  | protocol | http/rtmp/rtp      |
   -----------------------------------
  | format | rtmp/flv/ts/hls/rtp/dash  |
  ------------------------------------------
  | action | push, pull, file delivery  |
  -------------------------------------------
  
## 3. How to use

Build
```
git submodule init && git submodule update
./make.sh
```
