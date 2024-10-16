# Playback plugin

WIP plugin to record and play back inputs from your gamepad.

## Installation
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Copy the file `input_playback.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.  
2. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.
3. Requires the [NotificationModule](https://github.com/wiiu-env/NotificationModule) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

## Usage
- Connect a Gamepad and Pro Controller
- Press the **left stick** on the Pro Controller to start the recording. The Game will restart.
- Press the **right stick** on the Pro Controller to stop the recording
- Press **L** on the Pro Controller to playback the latest recording

Recording are not persistent. Recording to include full Gamepad input data (including touch, gyro, communication errors etc.)

## Buildflags

### Logging
Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it'll fallback to UDP (Port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging.

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t playback_plugin-builder

# make 
docker run -it --rm -v ${PWD}:/project playback_plugin-builder make

# make clean
docker run -it --rm -v ${PWD}:/project playback_plugin-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./src -i`