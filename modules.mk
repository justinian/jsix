ifeq "$(MOD_NAME)" ""
include "you must specify a MOD_NAME"
endif

ifndef SOURCES
	SOURCES := $(wildcard src/modules/$(MOD_NAME)/*.c)
	SOURCES += $(wildcard src/modules/$(MOD_NAME)/*.s)
endif

ifeq "$(SOURCES)" ""
include "you must specify a SOURCES list"
endif

MOD_SRC_D	:= src/modules/$(MOD_NAME)
MOD_BUILD_D	:= $(BUILD_D)/d.$(MOD_NAME)
MOD_LIBNAME	:= $(BUILD_D)/lib$(MOD_NAME).a
MOD_TARGETS	+= $(MOD_LIBNAME)

OBJS_$(MOD_NAME) := $(patsubst %.c,build/d.%.o,$(patsubst src/modules/%,%,$(SOURCES)))

$(MOD_LIBNAME): $(OBJS_$(MOD_NAME))
	$(AR) cr $@ $(OBJS_$(MOD_NAME))

$(MOD_BUILD_D)/%.o: $(MOD_SRC_D)/%.c $(INIT_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

DEPS += $(patsubst %.o,%.d,$(OBJS_$(MOD_NAME)))

