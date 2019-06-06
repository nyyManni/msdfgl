
lib: src/libmsdfgl.so

src/libmsdfgl.so: src/msdfgl.o src/msdfgl_serializer.o
	gcc -shared $^ -o $@

src/msdfgl.o: src/msdfgl.c src/_msdfgl_shaders.h src/msdfgl_serializer.h
	gcc -Isrc -I/usr/include/freetype2 -O3 -Wall -fPIC -DPIC -c $< -o $@ -Isrc

src/msdfgl_serializer.o: src/msdfgl_serializer.c src/msdfgl_serializer.h
	gcc -Isrc -I/usr/include/freetype2 -O3 -Wall -fPIC -DPIC -c $< -o $@ -Isrc

# Generate a C-header containing char arrays of the shader files.
src/_msdfgl_shaders.h: src/msdf_vertex.glsl \
                       src/msdf_fragment.glsl \
                       src/font_vertex.glsl \
                       src/font_geometry.glsl \
                       src/font_fragment.glsl
	echo '#ifndef _MSDF_SHADERS_H\n#define _MSDF_SHADERS_H' >$@
	for f in $^; do \
	        fname=$${f##*/}; \
	        sed -e '1 i\\nconst char * _'$${fname%%.glsl}' =' \
	            -e 's/\\/\\\\/' \
	            -e 's/^\(.*\)$$/"\1\\n"/' \
	            -e '$$ a;' $$f >>$@; \
	done
	echo '#endif /* _MSDF_SHADERS_H */' >>$@

install:  install-lib install-header

install-lib: lib
	cp src/libmsdfgl.so /usr/local/lib/

install-header: src/msdfgl.h
	cp $< /usr/local/include/


clean:
	rm -f src/libmsdfgl.so || true
	rm -f src/msdfgl.o || true
	rm -f src/_msdfgl_shaders.h || true

