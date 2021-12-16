## 1. introduction
Smartstreaming is a high-performance and scalable streaming media server.

## 2. design

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
  
