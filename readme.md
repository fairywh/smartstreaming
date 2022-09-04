## 1. Introduction
Smartstreaming is a high-performance and scalable streaming media server.

## 2. design
    https://user-images.githubusercontent.com/5512308/147847659-d402fafd-6cf0-4a96-9121-2d5624f70530.png![image](https://user-images.githubusercontent.com/5512308/188296425-291d2746-b6a3-4a29-9de1-c13897767079.png)

## 3. How to Use

```Build
git submodule init && git submodule update
./make.sh
```Start
./tmss -p{$port}
```Test
Initiate an http request to the server, with the source address filled in the Http Host. The server receives the request, requests the source reversely, and distributes it to the client. Multiple client requests will converge back to the source.

curl -v "http://127.0.0.1:{$port}/{$path}/{$stream}.{$ext}" -H "Host: XXX"
