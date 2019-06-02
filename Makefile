


lib: src/libmsdfgl.so

src/libmsdfgl.so: src/msdfgl.o
	gcc -shared $^ -o $@

src/msdfgl.o: src/msdfgl.c src/_msdfgl_shaders.h
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

clean:
	rm src/libmsdfgl.so || true
	rm src/msdfgl.o || true
	rm src/_msdfgl_shaders.h || true

