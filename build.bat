@echo off

gcc -Itree-sitter/lib/include -Itree-sitter/lib/src -o tree-sitter/libtree-sitter.dll tree-sitter/lib/src/lib.c -shared
gcc -Itree-sitter/lib/include -Itree-sitter/lib/src -c -o tree-sitter/tree-sitter.o tree-sitter/lib/src/lib.c
ar rcs tree-sitter/tree-sitter.a tree-sitter/tree-sitter.o

gcc -Itree-sitter-javascript/src -o tree-sitter-javascript/libtree-sitter-javascript.dll tree-sitter-javascript/src/parser.c tree-sitter-javascript/src/scanner.c -shared
gcc -Itree-sitter-javascript/src -c -o tree-sitter-javascript/tree-sitter-javascript-parser.o tree-sitter-javascript/src/parser.c
gcc -Itree-sitter-javascript/src -c -o tree-sitter-javascript/tree-sitter-javascript-scanner.o tree-sitter-javascript/src/scanner.c
ar rcs tree-sitter-javascript/tree-sitter-javascript.a tree-sitter-javascript/tree-sitter-javascript-parser.o tree-sitter-javascript/tree-sitter-javascript-scanner.o