## 1. introduction
Smartstreaming is a high-performance and scalable streaming media server.

## 2. design
![image](https://user-images.githubusercontent.com/5512308/147847659-d402fafd-6cf0-4a96-9121-2d5624f70530.png)
  
## 3. How to use

Build
```
git submodule init && git submodule update
./make.sh
```

start
```
./tmss -p{$port}
```

test
向server发起http请求，Host带上源地址，server收到请求，反向代理请求源地址，并分发给client，多个client请求会收敛回源.

Initiate an http request to the server, with the source address filled in the Http Host. The server receives the request, requests the source address reversely, and distributes it to the client. Multiple client requests will converge back to the source.
```
curl -v "http://127.0.0.1:{$port}/{$path}/{$stream}.{$ext}" -H "Host: XXX"
```
