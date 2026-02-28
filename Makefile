PLUGIN_NAME = hypr-columns

HYPRLAND_HEADERS = $(shell pkg-config --cflags hyprland pixman-1 libdrm)
CXXFLAGS = -shared -fPIC --no-gnu-unique -std=c++2b -g $(HYPRLAND_HEADERS)

SRCS = main.cpp ColumnsAlgorithm.cpp
OUT = $(PLUGIN_NAME).so

all: $(OUT)

$(OUT): $(SRCS) ColumnsAlgorithm.hpp globals.hpp
	g++ $(CXXFLAGS) -o $(OUT) $(SRCS)

clean:
	rm -f $(OUT)

install: $(OUT)
	hyprctl plugin load $(CURDIR)/$(OUT)

uninstall:
	hyprctl plugin unload $(CURDIR)/$(OUT)

.PHONY: all clean install uninstall
