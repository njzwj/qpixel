clang -Iinclude -c ./src/demo0.c -o ./bin/demo0.o -O2
clang -Iinclude -c ./src/qmath.c -o ./bin/qmath.o -O2
clang -Iinclude -c ./src/qpixel.c -o ./bin/qpixel.o -O2
clang -Iinclude -c ./src/qmesh.c -o ./bin/qmesh.o -O2
clang ./bin/demo0.o ./bin/qmath.o ./bin/qpixel.o ./bin/qmesh.o -o demo0.exe
