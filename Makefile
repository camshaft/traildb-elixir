MIX = mix
CFLAGS = -std=c99 -g -O3 -pedantic -Wall -Wextra -Wno-unused-parameter

ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS += -I$(ERLANG_PATH)
# TODO resolve this in a better way
LIBS = -L/usr/local/lib -ltraildb

ifneq ($(OS),Windows_NT)
	CFLAGS += -fPIC

	ifeq ($(shell uname),Darwin)
		LDFLAGS += -dynamiclib -undefined dynamic_lookup
	endif
endif

.PHONY: all traildb clean

all: traildb

traildb:
	@$(MIX) compile

priv/traildb_cons.so: src/traildb_cons.c
	@mkdir -p priv
	@$(CC) $(CFLAGS) -shared $(LDFLAGS) -o $@ src/traildb_cons.c $(LIBS)

clean:
	$(MIX) clean
	$(RM) priv/traildb_cons.so
