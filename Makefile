all:
	emcc -o bin/index.html src/*.c -Os -Wall ./lib/libraylib.web.a -I. -Iinclude -L. -Llib -s USE_GLFW=3 --shell-file src/minshell.html -DPLATFORM_WEB --preload-file res -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=67108864

docker:
	mkdir -p bin
	docker run --rm -v $(pwd):/app -w /app emscripten/emsdk make

native:
	gcc -o bin/game src/*.c -Iinclude -lraylib -lm