# BOOM ONLINE 2

## Build
### Server
#### Prerequisites
- Protobuf
```sh
apt install protobuf-compiler
```

#### Build steps
```sh
cd ./Server
make all
make tools
```

## Usage
### Server
```sh
cd ./Server/bin
./mapmaker
./server
```

Get server local ip address
```sh
ip a
```
or
```sh
hostname -I
```

### Client (windows only)
Run two or more instances of client app
```sh
cd ./Unity-Client/Builds
./BoomOnline2.exe
```
Fill in server's ip address and press validate button to try to connect

![Screenshot](docs/unity-client-connect-screen.png?raw=true)