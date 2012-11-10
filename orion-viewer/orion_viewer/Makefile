VERSION=0.38.5
TIME=$(shell date +%Y-%m-%d_%H)

all:
	ant debug && adb install -r $(shell pwd)/bin/orion_viewer-debug.apk
release:
	ant debug && cp $(shell pwd)/bin/orion_viewer-debug.apk \
		../../releases/orion_viewer-$(VERSION)-$(TIME).apk
