# MU

By Stefan Hedman, steffe@cs.umu.se

An HTML5/Node.js based FMU co-simulation environment. In practise it is just a web-based GUI for fmilib-master command-line interface.

## Install
First you need Node.js and NPM. On Ubuntu:
```
sudo apt-get install nodejs
```
Then install needed NPM packages:
```
npm install
```
You will also need the ```fmu-master``` from UMIT Research Lab.

Now create a config file:
```
cp config-sample.json config.json;
```
...and edit it so it fits your setup.

## Run
To start the server, run
```
node app.js
```