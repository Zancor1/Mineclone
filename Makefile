CXX = g++
PKG_CONFIG ?= pkg-config
TARGET := minecraft_clone
BUILD_DIR := build

CPPFLAGS += -Iinclude $(shell $(PKG_CONFIG) --cflags raylib)
CXXFLAGS ?= -O3 -DNDEBUG
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --libs raylib) -lm

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all clean test test-renderer

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.cpp Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

test: $(BUILD_DIR)/water_test $(BUILD_DIR)/water_mesh_test $(BUILD_DIR)/lighting_test $(BUILD_DIR)/settings_test
	./$(BUILD_DIR)/water_test
	./$(BUILD_DIR)/water_mesh_test
	./$(BUILD_DIR)/lighting_test
	./$(BUILD_DIR)/settings_test

$(BUILD_DIR)/water_test: tests/water_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/water_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/water_mesh_test: tests/water_mesh_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/water_mesh_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/lighting_test: tests/lighting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/lighting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/settings_test: tests/settings_test.cpp $(BUILD_DIR)/settings.o $(BUILD_DIR)/player.o $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/settings_test.cpp $(BUILD_DIR)/settings.o $(BUILD_DIR)/player.o $(BUILD_DIR)/world.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

test-renderer: $(BUILD_DIR)/renderer_smoke_test
	./$(BUILD_DIR)/renderer_smoke_test

$(BUILD_DIR)/renderer_smoke_test: tests/renderer_smoke_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/renderer_smoke_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPENDENCIES)
