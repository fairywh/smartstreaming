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
```
curl -v "http://127.0.0.1:{$port}/{$path}/{$stream}.{$ext}" -H "Host: XXX"
```
