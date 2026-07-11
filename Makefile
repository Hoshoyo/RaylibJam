web:
	emcc -o bin/index.html src/*.c src/renderer/*.c -Os -Wall ./lib/libraylib.web.a -I. -Iinclude -L. -Llib -s USE_GLFW=3 --shell-file src/minshell.html -DPLATFORM_WEB --preload-file res -s INITIAL_MEMORY=134217728

native:
	gcc -o bin/game src/*.c src/renderer/*.c -Iinclude -L./lib -lraylib -lm

docker:
	mkdir -p bin
	docker run --rm -v $(shell pwd):/app -w /app emscripten/emsdk make
