ifeq "$(MOD_NAME)" ""
include "you must specify a MOD_NAME"
endif

ifndef SOURCES
	SOURCES := $(wildcard src/modules/$(MOD_NAME)/*.c)
	SOURCES += $(wildcard src/modules/$(MOD_NAME)/*.cpp)
	SOURCES += $(wildcard src/modules/$(MOD_NAME)/*.s)
endif

ifeq "$(SOURCES)" ""
include "you must specify a SOURCES list"
endif

MOD_SRC_D	:= src/modules/$(MOD_NAME)
MOD_BUILD_D	:= $(BUILD_D)/d.$(MOD_NAME)
MOD_LIBNAME	:= $(BUILD_D)/lib$(MOD_NAME).a
MOD_TARGETS	+= $(MOD_LIBNAME)

OBJS_$(MOD_NAME) := $(patsubst %,build/d.%.o,$(patsubst src/modules/%,%,$(SOURCES)))

$(MOD_LIBNAME): $(OBJS_$(MOD_NAME))
	$(AR) cr $@ $(OBJS_$(MOD_NAME))

$(MOD_BUILD_D)/%.c.o: $(MOD_SRC_D)/%.c $(INIT_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

$(MOD_BUILD_D)/%.cpp.o: $(MOD_SRC_D)/%.cpp $(INIT_DEP)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(MOD_BUILD_D)/%.s.o: $(MOD_SRC_D)/%.s $(BUILD_D)/versions.s $(INIT_DEP)
	$(AS) $(ASFLAGS) -o $@ $<

DEPS += $(patsubst %.o,%.d,$(OBJS_$(MOD_NAME)))

