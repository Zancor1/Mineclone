CXX = g++
PKG_CONFIG ?= pkg-config
TARGET ?= minecraft_clone
BUILD_DIR ?= build

CPPFLAGS += -Iinclude $(shell $(PKG_CONFIG) --cflags raylib)
CXXFLAGS ?= -O3 -DNDEBUG
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --libs raylib) -lm -lz -lresolv -ldl -pthread

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all clean test test-renderer test-touch

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.cpp Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

test: test-skywars test-touch test-progression test-anticheat test-host-migration $(BUILD_DIR)/platform_test $(BUILD_DIR)/bedwars_test $(BUILD_DIR)/chat_text_test $(BUILD_DIR)/network_test $(BUILD_DIR)/items_test $(BUILD_DIR)/content18_test $(BUILD_DIR)/migration_test $(BUILD_DIR)/survival_test $(BUILD_DIR)/water_test $(BUILD_DIR)/water_mesh_test $(BUILD_DIR)/lighting_test $(BUILD_DIR)/settings_test $(BUILD_DIR)/lava_test $(BUILD_DIR)/streaming_test $(BUILD_DIR)/chest_test $(BUILD_DIR)/crafting_test
	./$(BUILD_DIR)/platform_test
	./$(BUILD_DIR)/bedwars_test
	./$(BUILD_DIR)/water_test
	./$(BUILD_DIR)/water_test lava
	./$(BUILD_DIR)/water_mesh_test
	./$(BUILD_DIR)/lighting_test
	./$(BUILD_DIR)/settings_test
	./$(BUILD_DIR)/lava_test
	./$(BUILD_DIR)/streaming_test
	./$(BUILD_DIR)/chest_test
	./$(BUILD_DIR)/crafting_test
	./$(BUILD_DIR)/survival_test
	./$(BUILD_DIR)/content18_test
	./$(BUILD_DIR)/items_test
	./$(BUILD_DIR)/network_test
	./$(BUILD_DIR)/chat_text_test
	./$(BUILD_DIR)/migration_test

$(BUILD_DIR)/water_test: tests/water_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/water_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/water_mesh_test: tests/water_mesh_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/water_mesh_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/lighting_test: tests/lighting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/lighting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/settings_test: tests/settings_test.cpp $(BUILD_DIR)/settings.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/settings_test.cpp $(BUILD_DIR)/settings.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/lava_test: tests/lava_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/lava_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/streaming_test: tests/streaming_test.cpp $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/streaming_test.cpp $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/chest_test: tests/chest_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/chest_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/crafting_test: tests/crafting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/crafting_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

test-renderer: $(BUILD_DIR)/bedwars_test $(BUILD_DIR)/renderer_smoke_test $(BUILD_DIR)/network_test
	./$(BUILD_DIR)/bedwars_test --render
	./$(BUILD_DIR)/renderer_smoke_test
	./$(BUILD_DIR)/network_test --render

$(BUILD_DIR)/renderer_smoke_test: tests/renderer_smoke_test.cpp $(BUILD_DIR)/ui.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) $(BUILD_DIR)/touch_platform.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/renderer_smoke_test.cpp $(BUILD_DIR)/ui.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/lighting.o $(BUILD_DIR)/touch_platform.o -o $@ $(LDLIBS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPENDENCIES)

$(BUILD_DIR)/survival_test: tests/survival_test.cpp $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/survival_test.cpp $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/content18_test: tests/content18_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h) $(wildcard include/*.inc)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/content18_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/migration_test: tests/migration_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/migration_test.cpp $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/items_test: tests/items_test.cpp $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/items_test.cpp $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/network_test: tests/network_test.cpp $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/items.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/player.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/network_test.cpp $(BUILD_DIR)/renderer.o $(BUILD_DIR)/bedwars.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/items.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/player.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o -o $@ $(LDLIBS)

$(BUILD_DIR)/chat_text_test: tests/chat_text_test.cpp $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o include/chat_text.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/chat_text_test.cpp $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o -o $@ $(LDLIBS)

$(BUILD_DIR)/bedwars_test: tests/bedwars_test.cpp $(BUILD_DIR)/bedwars_fixture.o $(BUILD_DIR)/progression.o $(BUILD_DIR)/cosmetics.o $(BUILD_DIR)/skywars.o $(BUILD_DIR)/bedwars_maps.o $(BUILD_DIR)/external_arenas.o $(BUILD_DIR)/lan_transport.o $(BUILD_DIR)/ui.o $(BUILD_DIR)/renderer.o $(BUILD_DIR)/items.o $(BUILD_DIR)/player.o $(BUILD_DIR)/network.o $(BUILD_DIR)/chat_text.o $(BUILD_DIR)/dns_srv.o $(BUILD_DIR)/settings.o $(BUILD_DIR)/world.o $(BUILD_DIR)/map_blocks.o $(BUILD_DIR)/chest.o $(BUILD_DIR)/crafting.o $(BUILD_DIR)/survival.o $(BUILD_DIR)/lighting.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: windows
windows:
	sh tools/build-windows.sh

$(BUILD_DIR)/platform_test: tests/platform_test.cpp src/lan_transport.cpp src/dns_srv.cpp include/platform_socket.h include/file_replace.h include/lan_transport.h include/dns_srv.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/platform_test.cpp src/lan_transport.cpp src/dns_srv.cpp -o $@ $(LDLIBS)

.PHONY: test-platform test-windows-platform
test-platform: $(BUILD_DIR)/platform_test
	./$(BUILD_DIR)/platform_test

test-windows-platform:
	mkdir -p build/windows
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -D_WIN32_WINNT=0x0601 -Iinclude tests/platform_test.cpp src/lan_transport.cpp src/dns_srv.cpp -static -lws2_32 -ldnsapi -liphlpapi -pthread -o build/windows/platform_test.exe
	WINEPREFIX="$(CURDIR)/build/windows/wine-test" WINEDEBUG=-all WINEDLLOVERRIDES="mscoree,mshtml=" wine build/windows/platform_test.exe

$(BUILD_DIR)/external_maps_test: tests/external_maps_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/external_maps_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS)) -o $@ $(LDLIBS)

.PHONY: test-maps
test-maps: $(BUILD_DIR)/external_maps_test
	./$(BUILD_DIR)/external_maps_test

.PHONY: test-progression
test-progression: $(BUILD_DIR)/progression_test
	./$(BUILD_DIR)/progression_test
$(BUILD_DIR)/progression_test: tests/progression_test.cpp $(BUILD_DIR)/progression.o include/progression.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/progression_test.cpp $(BUILD_DIR)/progression.o -o $@ $(LDLIBS)

.PHONY: test-cosmetics
test-cosmetics: $(BUILD_DIR)/cosmetics_test
	./$(BUILD_DIR)/cosmetics_test
$(BUILD_DIR)/cosmetics_test: tests/cosmetics_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: test-edit-frames
test-edit-frames: $(BUILD_DIR)/edit_frame_test
	./$(BUILD_DIR)/edit_frame_test
$(BUILD_DIR)/edit_frame_test: tests/edit_frame_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: test-loading
test-loading: $(BUILD_DIR)/loading_test
	./$(BUILD_DIR)/loading_test
$(BUILD_DIR)/loading_test: tests/loading_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/touch_test: tests/touch_test.cpp include/touch_controls.h include/settings.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/touch_test.cpp -o $@ $(LDLIBS)

test-touch: $(BUILD_DIR)/touch_test
	./$(BUILD_DIR)/touch_test

.PHONY: android
android:
	./tools/build-android.sh

$(BUILD_DIR)/renderer_packed.o: src/renderer.cpp $(wildcard include/*.h) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DMINECLONE_PACKED_ATLAS -c $< -o $@

$(BUILD_DIR)/renderer_mobile_test: tests/renderer_smoke_test.cpp $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/game.o $(BUILD_DIR)/renderer.o,$(OBJECTS)) $(BUILD_DIR)/renderer_packed.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/anticheat_test: tests/anticheat_test.cpp include/anticheat.h | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/anticheat_test.cpp -o $@

$(BUILD_DIR)/anticheat_lan_test: tests/anticheat_lan_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: test-anticheat
test-anticheat: $(BUILD_DIR)/anticheat_test $(BUILD_DIR)/anticheat_lan_test
	./$(BUILD_DIR)/anticheat_test
	./$(BUILD_DIR)/anticheat_lan_test

# Legacy gameplay fixtures directly reposition the local player to arrange scenes.
# Keep that setup helper out of production and the anti-cheat/migration tests.
$(BUILD_DIR)/bedwars_fixture.o: src/bedwars.cpp src/bedwars_migration.inc $(wildcard include/*.h) Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DMINECLONE_LEGACY_FIXTURE -MMD -MP -c $< -o $@

$(BUILD_DIR)/host_migration_test: tests/host_migration_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: test-host-migration
test-host-migration: $(BUILD_DIR)/host_migration_test
	./$(BUILD_DIR)/host_migration_test

.PHONY: test-skywars
test-skywars: $(BUILD_DIR)/skywars_test
	./$(BUILD_DIR)/skywars_test

$(BUILD_DIR)/skywars_test: tests/skywars_test.cpp $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)
