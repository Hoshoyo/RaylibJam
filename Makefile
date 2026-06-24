all:
	emcc -o bin/game.html src/*.c -Os -Wall ./lib/libraylib.web.a -I. -Iinclude -L. -Llib -s USE_GLFW=3 --shell-file src/minshell.html -DPLATFORM_WEB --preload-file res -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=67108864

native:
	gcc -o bin/game src/*.c -Iinclude -lraylib -lm