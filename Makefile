#Copyright 2026 Spalishe
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#

include objects.mk

define banner

                  __     _  _                               
  _ __  __   __  / /_   | || |           __   __  _ __ ___  
 | '__| \ \ / / | '_ \  | || |_   _____  \ \ / / | '_ ` _ \ 
 | |     \ V /  | (_) | |__   _| |_____|  \ V /  | | | | | |
 |_|      \_/    \___/     |_|             \_/   |_| |_| |_|

endef

export banner

ANSI_GREEN := \x1b[32m
ANSI_BLUE := \x1b[34m
ANSI_RESET := \x1b[0m

define print_info
	@echo "$$banner"
	@echo -e "Branch: $(ANSI_GREEN)$(GIT_BRANCH)-$(GIT_HASH)$(ANSI_RESET)"
	@echo -e "Target arch: $(ANSI_GREEN)$(TRIPLET_ARCH)$(ANSI_RESET)"
	@echo -e "Detected OS: $(ANSI_GREEN)$(OS)$(ANSI_RESET)"
	@echo -e "Detected CXX: $(ANSI_GREEN)$(CXX_VERSION)$(ANSI_RESET)"
	@echo -e "Detected AR: $(ANSI_GREEN)$(AR_VERSION)$(ANSI_RESET)"
	@echo
endef

define print_help
	@echo -e "[$(ANSI_BLUE)INFO$(ANSI_RESET)] Available useflags:"
	@$(foreach var,$(USE_VARS),echo "  $(var)=$($(var))";)

	@echo
	@echo -e "[$(ANSI_BLUE)INFO$(ANSI_RESET)] Available commands:"
	@echo -e "  $(ANSI_GREEN)all$(ANSI_RESET)			Build target"
	@echo -e "  $(ANSI_GREEN)lib$(ANSI_RESET)           Build target to dynamic library"
	@echo -e "  $(ANSI_GREEN)slib$(ANSI_RESET)           Build target to static library"
	@echo -e "  $(ANSI_GREEN)help$(ANSI_RESET)			Shows this menu"
	@echo -e "  $(ANSI_GREEN)clean$(ANSI_RESET)			Clean the build directory"
endef

GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)
GIT_HASH := $(shell git rev-parse HEAD)
GIT_HASH_SHORT := $(shell git rev-parse --short HEAD)

ifeq ($(GIT_BRANCH),)
    GIT_BRANCH := unknown
endif
ifeq ($(GIT_HASH),)
    GIT_HASH := unknown
endif

ifdef CROSS_COMPILE
	CXX := $(CROSS_COMPILE)g++
endif

CXX ?= g++
AR = $(patsubst %g++,%ar,$(CXX))

TRIPLET := $(shell $(CXX) -dumpmachine)

OS = $(shell uname -s)
OS_lower = $(shell echo $(OS) | tr A-Z a-z)
EMPTY := 
SPACE := $(EMPTY) $(EMPTY)
TRIPLET_WORDS := $(subst -,$(SPACE),$(TRIPLET))
TRIPLET_ARCH  := $(word 1,$(TRIPLET_WORDS))
TRIPLET_2     := $(word 2,$(TRIPLET_WORDS))
TRIPLET_3     := $(word 3,$(TRIPLET_WORDS))
TRIPLET_4     := $(word 4,$(TRIPLET_WORDS))

ifeq ($(shell which $(CXX) 2>/dev/null),)
    $(error [FATAL] Compiler '$(CXX)' not found.)
endif
CXX_VERSION := $(shell $(CXX) --version | head -n 1)
AR_VERSION := $(shell $(AR) --version | head -n 1)

#  -fsanitize=address and -fno-omit-frame-pointer for detailed debugging(ASAN)
LIBS := -latomic -pthread 
CXXFLAGS := -std=c++20 -std=gnu++20 $(LIBS) -O3 -g -march=native -flto -MMD -MP -Iinclude

CXXFLAGS += $(foreach v,$(USE_VARS),$(if $(filter-out 0,$($(v))),-D$(v)=$($(v))))

CXXFLAGS += -DRVEM_VERSION='"rv64-vm; git-$(GIT_HASH_SHORT)"'

ifneq ($(findstring mingw,$(TRIPLET_WORDS)),)
    EXE_EXT := .exe
    LIB_EXT := .dll
	STATIC_EXT := .lib
else ifneq ($(findstring darwin,$(TRIPLET_WORDS)),)
    EXE_EXT :=
    LIB_EXT := .dylib
	STATIC_EXT := .a
else
    EXE_EXT :=
    LIB_EXT := .so
	STATIC_EXT := .a
endif

ifdef USE_FRAMEBUFFER
	DISPLAY_SERVER := $(shell \
		if [ "$$XDG_SESSION_TYPE" = "wayland" ]; then echo "wayland"; \
		elif [ "$$XDG_SESSION_TYPE" = "x11" ]; then echo "x11"; \
		elif [ -n "$$WAYLAND_DISPLAY" ]; then echo "wayland"; \
		elif [ -n "$$DISPLAY" ]; then echo "x11"; \
		else echo "unknown"; fi)

	ifeq ($(DISPLAY_SERVER), wayland)
		CXXFLAGS += -D__PKG_WAYLAND
		LIBS += -lwayland-client -lvulkan
	else ifeq ($(DISPLAY_SERVER), x11)
		CXXFLAGS += -D__PKG_X11
		LIBS += -lvulkan -lX11
	endif
endif

BUILD_DIR := build.$(TRIPLET_3).$(TRIPLET_ARCH)
OBJ_DIR_BIN := $(BUILD_DIR)/obj/bin/
OBJ_DIR_SO := $(BUILD_DIR)/obj/so/
OBJ_DIR_A := $(BUILD_DIR)/obj/a/
SRCS := $(shell find src -name '*.cpp')
SRCS_LIB := $(filter-out src/main.cpp, $(SRCS))
OBJS_BIN := $(patsubst src/%.cpp,$(OBJ_DIR_BIN)/%.o,$(SRCS))
OBJS_SO := $(patsubst src/%.cpp,$(OBJ_DIR_SO)/%.o,$(SRCS_LIB))
OBJS_A := $(patsubst src/%.cpp,$(OBJ_DIR_A)/%.o,$(SRCS_LIB))
TARGET_BIN := $(BUILD_DIR)/release_$(TRIPLET_ARCH)$(EXE_EXT)
TARGET_SO := $(BUILD_DIR)/lib_$(TRIPLET_ARCH)$(LIB_EXT)
TARGET_A := $(BUILD_DIR)/slib_$(TRIPLET_ARCH)$(STATIC_EXT)

all:
	$(call print_info)
	
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR_BIN) 
	
	@$(MAKE) --no-print-directory $(TARGET_BIN)

lib:
	$(call print_info)
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR_SO) 

	@$(MAKE) --no-print-directory $(TARGET_SO)

slib: LIBS += -static
slib: CXXFLAGS += -static
slib:
	$(call print_info)
	@mkdir -p $(BUILD_DIR) 
	@mkdir -p $(OBJ_DIR_SO) 

	@$(MAKE) --no-print-directory $(TARGET_A)

help:
	$(call print_info)
	$(call print_help)
clean:
	$(call print_info)
	@rm -rf $(BUILD_DIR)
	
$(OBJ_DIR_BIN)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR_SO)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@
	
$(OBJ_DIR_A)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[$(ANSI_GREEN)CXX$(ANSI_RESET)] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(OBJS_BIN:.o=.d)
-include $(OBJS_SO:.o=.d)
-include $(OBJS_A:.o=.d)

$(TARGET_BIN): $(OBJS_BIN)
	@echo -e "[$(ANSI_BLUE)LD$(ANSI_RESET)] $@"
	@$(CXX) $(OBJS_BIN) -o $@ $(LIBS) $(LDFLAGS)

$(TARGET_SO): $(OBJS_SO)
	@echo -e "[$(ANSI_BLUE)LD$(ANSI_RESET)] $@"
	@$(CXX) $(OBJS_SO) -o $@ $(LIBS) -shared
	
$(TARGET_A): $(OBJS_A)
	@echo -e "[$(ANSI_BLUE)AR$(ANSI_RESET)] $@"
	@$(AR) rcs  $@ $(OBJS_A) 
