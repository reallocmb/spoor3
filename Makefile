program = spoor3

extern_lib = `pkg-config --cflags --libs freetype2` -lX11
#extern_lib = -lraylibw -lopengl32 -lgdi32 -lwinmm

compiler = gcc $(develop_flags)

version = gnu89

test_flags = -Wall -Wextra -std=$(version) -g
develop_flags = -Wall -Wextra -std=$(version) -g
release_flags = -std=$(version) -O3 -DRELEASE

source_dir = src
object_dir = obj
binary_dir = bin

source_sub = $(wildcard src/*/*.c)
object_sub = $(patsubst $(source_dir)/%.c,$(object_dir)/%.o,$(source_sub))

source = $(wildcard $(source_dir)/*.c)
object = $(patsubst $(source_dir)/%.c,$(object_dir)/%.o,$(source))

object_all = $(object) $(object_sub)

binary = $(binary_dir)/$(program)

sub_dirs = $(filter-out $(wildcard src/*.c) $(wildcard src/*.h),$(wildcard src/*))
objdirs = $(patsubst src/%,obj/%,$(sub_dirs))

all: $(objdirs) $(object_all) $(binary)

$(binary): $(object_all)
	$(compiler) -Llib -o $(binary) $(object_all) $(extern_lib) 

$(object_dir)/%.o: $(source_dir)/%.c
	$(compiler) -Iinc `pkg-config --cflags freetype2` -c -o $@ $<

$(objdirs):
	@mkdir -p $@

run: all
	./$(binary)

runt: all
	./$(binary) -t

gdb:
	gdb ./$(binary)

clean:
	rm $(object_dir)/* -rf
	rm $(binary) -f

init:
	mkdir -p $(source_dir) $(object_dir) $(binary_dir)

install_dir = /usr/bin

install: compiler := gcc $(release_flags)
install: clean all
	sudo cp $(binary) $(install_dir)

tests: compiler := gcc $(test_flags)
tests: all
	gcc -Isrc tests/test.c -o bin/tests tests/eenheid/*.c $(extern_lib)
	bin/tests

test:
	gcc -Isrc tests/test.c -o bin/test tests/eenheid/*.c $(extern_lib)
	./bin/test

release: 
	compiler = gcc $(release_flags)

remove:
	sudo rm $(install_dir)/$(program)

bear: clean
	bear -- make
